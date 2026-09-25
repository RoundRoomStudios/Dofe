#include "pcf.h"

#define PCF_HEADER             0x70636601

#define PCF_METRICS            0x00000004
#define PCF_ENCODINGS          0x00000020
#define PCF_BITMAPS            0x00000008

#define PCF_COMPRESSED_METRICS 0x00000100

#define PCF_BITMAP_PAD         0x00000003
#define PCF_BYTE_MASK          0x00000004
#define PCF_BIT_MASK           0x00000008
#define PCF_BITMAP_UNIT        0x00000030

#define PCF_BYTE  0
#define PCF_SHORT 1
#define PCF_INT   2

static const uint8_t padList[3] = {1, 2, 4};

// Source: https://fontforge.org/docs/techref/pcf-format.html#

static PCF_Tables* readTableDirectory(FILE* file) { // Error TAG: pcf  | tdir
    // check header
    uint32_t header;
    if (!readU32(&header, 1, file, "pcf  | tdir", "header", false)) return NULL;
    if (header != PCF_HEADER) {
        printf("pcf  | tdir | pcf header is wrong");
        return NULL;
    }

    // get table count
    uint32_t tableCount;
    if (!readU32(&tableCount, 1, file, "pcf  | tdir", "table count", false)) return NULL;

    // allocate directory
    uint32_t tableSize = sizeof(PCF_Tables) + tableCount * sizeof(PCF_TableEntry);
    PCF_Tables* tables = allocate(tableSize, "pcf  | tdir", "table directory");
    if (tables == NULL) return NULL;
    tables->tableCount = tableCount;

    // read tables
    for (uint32_t i = 0; i < tableCount; i++) {
        if (!readU32(&tables->tables[i].type, 1, file,   "pcf  | tdir", "table entry type",   false)) return NULL;
        if (!readU32(&tables->tables[i].format, 1, file, "pcf  | tdir", "table entry format", false)) return NULL;
        if (!readU32(&tables->tables[i].size, 1, file,   "pcf  | tdir", "table entry size",   false)) return NULL;
        if (!readU32(&tables->tables[i].offset, 1, file, "pcf  | tdir", "table entry offset", false)) return NULL;
    }

    #if DEBUG || DEBUG_PCF || DEBUG_PCF_TDIR
    printf("\npcf  | tdir\n");
    for (uint8_t i = 0; i < tableCount; i++) {
        switch (tables->tables[i].type) {
            case PCF_METRICS:
                printf("%i: Metrics   table, sized %i\n", i, tables->tables[i].size); break;
            case PCF_ENCODINGS:
                printf("%i: Encodings table, sized %i\n", i, tables->tables[i].size); break;
            case PCF_BITMAPS:
                printf("%i: Bitmaps   table, sized %i\n", i, tables->tables[i].size); break;
            default:
                printf("%i: Unknown   table, sized %i\n", i, tables->tables[i].size); break;
        }
    }
    #endif

    return tables;
}

static inline uint32_t getTableOffset(PCF_Tables* tables, uint32_t type) {
    // Helper for read____PCF functions
    for (uint16_t i = 0; i < tables->tableCount; i++) {
        if (tables->tables[i].type == type) {
            // Return offset
            return  tables->tables[i].offset;
        }
    }
    return 0;
}

static PCF_EncodingList* readEncodingsPCFAll(FILE* file, uint16_t minByte1, uint16_t maxByte1, uint16_t minByte2, uint16_t maxByte2, bool bswap, uint16_t defaultCharacter) { // Error TAG: pcf  | enco | all
    // remember position for 2nd scan
    long encodingListOffset;
    if (!tellFile(file, &encodingListOffset, "pcf  | enco | all ", "encoding data")) return NULL;

    // split into 2, 2D scans (1 to get list size, other to fill list)
    uint16_t characterAmount = 0;
    for (uint16_t byte1 = minByte1; byte1 <= maxByte1; byte1++) {
        for (uint16_t byte2 = minByte2; byte2 <= maxByte2; byte2++) {
            // add a character if it has a defined endpoint
            uint16_t characterIndex;
            if (!readU16(&characterIndex, 1, file, "pcf  | enco | all ", "character index", bswap)) return NULL;
            if (characterIndex != 0xFFFF) characterAmount++;
        }
    }

    // allocation
    uint32_t encodingsSize = sizeof(PCF_EncodingList) + characterAmount * sizeof(PCF_Encoding);
    PCF_EncodingList* encodingList = allocate(encodingsSize, "pcf  | enco | all ", "encodings list");
    if (encodingList == NULL) return NULL;
    encodingList->characterAmount = characterAmount;
    encodingList->defaultCharacter = defaultCharacter;

    // 2nd scan
    if (!seekFile(file, encodingListOffset, SEEK_SET, "pcf  | enco | all ", "encoding data")) return NULL;

    uint16_t characterI = 0;
    for (uint16_t byte1 = minByte1; byte1 <= maxByte1; byte1++) {
        for (uint16_t byte2 = minByte2; byte2 <= maxByte2; byte2++) {
            uint16_t characterIndex;
            // store a character if it has a defined endpoint
            if (!readU16(&characterIndex, 1, file, "pcf  | enco | all ", "character index", bswap)) return NULL;
            if (characterIndex != 0xFFFF) {

                // store character
                PCF_Encoding* character = &encodingList->encodings[characterI];
                character->codepoint = ((uint16_t)byte1 * 0x100) + byte2;
                character->index = characterIndex;
                characterI++;
            }
        }
    }

    return encodingList;
}

static PCF_EncodingList* readEncodingsPCFSome(FILE* file, uint16_t minByte1, uint16_t maxByte1, uint16_t minByte2, uint16_t maxByte2, bool bswap, uint16_t defaultCharacter, char32_t* include) { // Error TAG: pcf  | enco | some
    // remember position for offseting into list
    long location;
    if (!tellFile(file, &location, "pcf  | enco | some", "encoding data")) return NULL;

    // allocate valid characters list
    uint32_t length = 0;
    while (include[length] != 0) {
        length++;
    }
    char32_t* valid = allocate(length * sizeof(char32_t), "pcf  | enco | some", "valid list");
    uint32_t  validLength = 0;

    // check valid characters
    for (uint32_t i = 0; i < length; i++) {
        // check if in bounds
        uint8_t byte1 = include[i] >> 8;
        uint8_t byte2 = include[i] & 0xFF;
        if (byte1 > maxByte1 || byte1 < minByte1) continue;
        if (byte2 > maxByte2 || byte2 < minByte2) continue;
        if (include[i] > 0xFFFF) continue;

        // check if valid character
        uint16_t index = (byte1-minByte1)*(maxByte2-minByte2+1)+byte2-minByte2;
        if (!seekFile(file, location + index * sizeof(uint16_t), SEEK_SET, "pcf  | enco | some", "index")) return NULL;

        uint16_t characterIndex;
        if (!readU16(&characterIndex, 1, file, "pcf  | enco | some", "character index", bswap)) return NULL;

        if (characterIndex == 0xFFFF) continue;

        // check if repeat character
        bool skip = false;
        for (uint16_t j = 0; j < validLength; j++) {
            if (valid[j] == include[i]) {skip = true; break;}
        }
        if (skip) continue;

        // set into valid list
        valid[validLength] = include[i];
        validLength++;
    }

    // allocation
    uint32_t encodingsSize = sizeof(PCF_EncodingList) + validLength * sizeof(PCF_Encoding);
    PCF_EncodingList* encodingList = allocate(encodingsSize, "pcf  | enco | some", "encodings list");
    if (encodingList == NULL) return NULL;
    encodingList->characterAmount = validLength;
    encodingList->defaultCharacter = defaultCharacter;

    // set data in encodings
    for (uint32_t i = 0; i < validLength; i++) {
        // fill codepoint
        encodingList->encodings[i].codepoint = (uint16_t)valid[i];

        // calculation taken from source (source is in comment near file top)
        uint8_t  byte1 = (uint8_t)(valid[i] >> 8);
        uint8_t  byte2 = (uint8_t)(valid[i]);
        uint16_t index = (byte1-minByte1)*(maxByte2-minByte2+1)+byte2-minByte2;

        // goto and read from index
        if (!seekFile(file, location + index * sizeof(uint16_t), SEEK_SET, "pcf  | enco | some", "index")) return NULL;
        if (!readU16(&encodingList->encodings[i].index, 1, file, "pcf  | enco | some", "character index", bswap)) return NULL;
    }

    return encodingList;
}

static PCF_EncodingList* readEncodingsPCF(FILE* file, PCF_Tables* tables, char32_t* include) { // Error TAG: pcf  | enco
    // goto table
    uint32_t offset = getTableOffset(tables, PCF_ENCODINGS);
    if (!seekFile(file, offset, SEEK_SET, "pcf  | enco", "encoding")) return NULL;

    // check format
    uint32_t format;
    if (!readU32(&format, 1, file, "pcf  | enco", "format", false)) return NULL;
    bool bswap = (format & PCF_BYTE_MASK) != 0;

    // read range of encodings
    uint16_t minByte2;
    if (!readU16(&minByte2, 1, file, "pcf  | enco", "minimum of byte 2", bswap)) return NULL;
    uint16_t maxByte2;
    if (!readU16(&maxByte2, 1, file, "pcf  | enco", "maximum of byte 2", bswap)) return NULL;
    uint16_t minByte1;
    if (!readU16(&minByte1, 1, file, "pcf  | enco", "minimum of byte 1", bswap)) return NULL;
    uint16_t maxByte1;
    if (!readU16(&maxByte1, 1, file, "pcf  | enco", "maximum of byte 1", bswap)) return NULL;

    // character for when a invalid one is displayed (needs to be always scanned)
    uint16_t defaultCharacter;
    if (!readU16(&defaultCharacter, 1, file, "pcf  | enco", "default character", bswap)) return NULL;

    // switch depending on include
    PCF_EncodingList* encodingList;
    if (include == NULL) {
        // scans everything but is less efficient
        encodingList =  readEncodingsPCFAll(file, minByte1, maxByte1, minByte2, maxByte2, bswap, defaultCharacter);
    } else {
        // only scans selected encodings and more efficient
        encodingList = readEncodingsPCFSome(file, minByte1, maxByte1, minByte2, maxByte2, bswap, defaultCharacter, include);
    }

    #if DEBUG || DEBUG_PCF || DEBUG_PCF_ENCO
    printf("\npcf  | enco\n");
    for (uint16_t i = 0; i < encodingList->characterAmount; i++) {
        if (printf("%lc: %i\n", encodingList->encodings[i].codepoint, encodingList->encodings[i].index) < 0)
            printf(" : %i\n", encodingList->encodings[i].index);
    }
    #endif

    return encodingList;
}

static PCF_MetricList* readMetricsPCFFull(FILE* file, bool bswap, PCF_EncodingList* encoding) { // Error TAG: pcf  | metr | full
    // skip metric count and get array location
    if (!seekFile(file, sizeof(int32_t), SEEK_CUR, "pcf  | metr | all ", "past metric count")) return NULL;
    long arrayLocation;
    if (!tellFile(file, &arrayLocation, "pcf  | metr | all ", "metric data location")) return NULL;

    // allocate metric array
    uint32_t metricsSize = sizeof(PCF_MetricList) + encoding->characterAmount * sizeof(PCF_Metric);
    PCF_MetricList* metricList = allocate(metricsSize, "pcf  | metr | all ", "metrics list");
    if (metricList == NULL) return NULL;
    PCF_Metric* metrics = (void*)metricList->metrics;
    metricList->compressed = false;

    // loop over encodings for each metric
    for (uint16_t i = 0; i < encoding->characterAmount; i++) {
        // goto metrics for each encoding
        uint16_t index = (uint16_t)encoding->encodings[i].index;
        long metricLocation = arrayLocation + index * sizeof(PCF_Metric);
        if (!seekFile(file, metricLocation, SEEK_SET, "pcf  | metr | all ", "to metric")) return NULL;

        // get metrics
        PCF_Metric* metric = &metrics[i];
        if (!read16( &metric->leftSidedBearing, 1, file, "pcf  | metr | all ", "left sided bearing",  bswap)) return NULL;
        if (!read16( &metric->rightSideBearing, 1, file, "pcf  | metr | all ", "right side bearing",  bswap)) return NULL;
        if (!read16( &metric->characterWidth,   1, file, "pcf  | metr | all ", "character width",     bswap)) return NULL;
        if (!read16( &metric->characterAscent,  1, file, "pcf  | metr | all ", "character ascent",    bswap)) return NULL;
        if (!read16( &metric->characterDescent, 1, file, "pcf  | metr | all ", "character descent",   bswap)) return NULL;
        if (!readU16(&metric->characterAttrib,  1, file, "pcf  | metr | all ", "character attribute", bswap)) return NULL;
    }

    return metricList;
}

static PCF_MetricList* readMetricsPCFComp(FILE* file, bool bswap, PCF_EncodingList* encoding) { // Error TAG: pcf  | metr | comp
    // skip metric count and get array location
    if (!seekFile(file, sizeof(int16_t), SEEK_CUR, "pcf  | metr | comp", "past metric count")) return NULL;
    long arrayLocation;
    if (!tellFile(file, &arrayLocation, "pcf  | metr | comp", "metric data location")) return NULL;

    // allocate metric array
    uint32_t metricsSize = sizeof(PCF_MetricList) + encoding->characterAmount * sizeof(PCF_MetricCompressed);
    PCF_MetricList* metricList = allocate(metricsSize, "pcf  | metr | comp", "metrics list compressed");
    if (metricList == NULL) return NULL;
    PCF_MetricCompressed* metrics = (void*)metricList->metrics;
    metricList->compressed = true;

    // loop over encodings for each metric
    for (uint16_t i = 0; i < encoding->characterAmount; i++) {
        // goto metrics for each encoding
        uint16_t index = (uint16_t)encoding->encodings[i].index;
        long metricLocation = arrayLocation + index * sizeof(PCF_MetricCompressed);
        if (!seekFile(file, metricLocation, SEEK_SET, "pcf  | metr | comp", "to metric")) return NULL;

        // get metrics
        PCF_MetricCompressed* metric = &metrics[i];
        if (!readU8(&metric->leftSidedBearing, 1, file, "pcf  | metr | comp", "left sided bearing")) return NULL;
        if (!readU8(&metric->rightSideBearing, 1, file, "pcf  | metr | comp", "right side bearing")) return NULL;
        if (!readU8(&metric->characterWidth,   1, file, "pcf  | metr | comp", "character width"   )) return NULL;
        if (!readU8(&metric->characterAscent,  1, file, "pcf  | metr | comp", "character ascent"  )) return NULL;
        if (!readU8(&metric->characterDescent, 1, file, "pcf  | metr | comp", "character descent" )) return NULL;
    }

    return metricList;
}

static PCF_MetricList* readMetricsPCF(FILE* file, PCF_Tables* tables, PCF_EncodingList* encoding) { // Error TAG: pcf  | metr
    // goto table
    uint32_t offset = getTableOffset(tables, PCF_METRICS);
    if (!seekFile(file, offset, SEEK_SET, "pcf  | metr", "metrics")) return NULL;

    // check format
    uint32_t format;
    if (!readU32(&format, 1, file, "pcf  | metr", "format", false)) return NULL;
    bool bswap = (format & PCF_BYTE_MASK) != 0;

    // change reading depending if its compressed or not
    PCF_MetricList* metricList;
    if (format & PCF_COMPRESSED_METRICS) {
        metricList = readMetricsPCFComp(file, bswap, encoding);
    } else {
        metricList = readMetricsPCFFull(file, bswap, encoding);
    }

    #if DEBUG || DEBUG_PCF || DEBUG_PCF_METR
    printf("\npcf  | metr\n");
    if (metricList->compressed) {
        for (uint16_t i = 0; i < encoding->characterAmount; i++) {
            uint16_t codepoint = encoding->encodings[i].codepoint;

            PCF_MetricCompressed* metrics = (void*)metricList->metrics;

            int16_t left    = metrics[i].leftSidedBearing - 0x80;
            int16_t right   = metrics[i].rightSideBearing - 0x80;
            int16_t ascent  = metrics[i].characterAscent  - 0x80;
            int16_t descent = metrics[i].characterDescent - 0x80;

            int16_t width = right - left;
            int16_t height = ascent + descent;

            if (printf("%lc: %i x %i\n", codepoint, width, height) < 0)
                printf(" : %i x %i\n", width, height);
        }
    } else {
        for (uint16_t i = 0; i < encoding->characterAmount; i++) {
            uint16_t codepoint = encoding->encodings[i].codepoint;

            PCF_Metric* metrics = (void*)metricList->metrics;

            int16_t left    = metrics[i].leftSidedBearing;
            int16_t right   = metrics[i].rightSideBearing;
            int16_t ascent  = metrics[i].characterAscent;
            int16_t descent = metrics[i].characterDescent;

            int32_t width = right - left;
            int32_t height = ascent + descent;

            if (printf("%lc: %i x %i\n", codepoint, width, height) < 0)
                printf(" : %i x %i\n", width, height);
        }
    }
    #endif

    return metricList;
}

static PCF_Metric getMetric(PCF_MetricList* metricList, uint32_t i) { // Error TAG: pcf  | metr | get
    // helper function to get metrics
    PCF_Metric metric;
    if (metricList->compressed) {
        // resize from list
        PCF_MetricCompressed* metrics = (void*)metricList->metrics;
        metric.leftSidedBearing = metrics[i].leftSidedBearing - 0x80;
        metric.rightSideBearing = metrics[i].rightSideBearing - 0x80;
        metric.characterWidth   = metrics[i].characterWidth   - 0x80;
        metric.characterAscent  = metrics[i].characterAscent  - 0x80;
        metric.characterDescent = metrics[i].characterDescent - 0x80;
        metric.characterAttrib  = 0;
    } else {
        // simply output from list
        PCF_Metric* metrics = (void*)metricList->metrics;
        metric = metrics[i];
    }

    return metric;
}

static PCF_AtlasInfo getAtlasInfo(PCF_MetricList* metricList, PCF_EncodingList* encodingList) { // Error TAG: pcf  | bitm | info
    // for errors
    PCF_AtlasInfo nullInfo = {0};

    // info values (initial values from colors)
    PCF_AtlasInfo info = {0, 0x04000200};
    uint32_t bitSize = 4;

    // determine size
    uint8_t xBits = 0;
    uint8_t yBits = 0;
    for (uint32_t i = 0; i < encodingList->characterAmount; i++) {
        // get size of character
        PCF_Metric metric = getMetric(metricList, i);
        uint16_t width = metric.rightSideBearing - metric.leftSidedBearing;
        uint16_t height = metric.characterAscent + metric.characterDescent;
        bitSize += width * height;

        // set bit size
        xBits = maxU8(xBits, __builtin_stdc_bit_width((uint16_t)(width  - 1)));
        yBits = maxU8(yBits, __builtin_stdc_bit_width((uint16_t)(height - 1)));

        // skip check if invalid
        if (info.sizeAmount == 0xFFFF) continue;

        // check if indexed
        bool indexed = false;
        for (uint16_t j = 0; j < info.sizeAmount; j++) {
            if (info.w[j] + 1 == width && info.h[j] + 1 == height) {indexed = true; break;}
        }

        // if not index it
        if (!indexed) {
            info.w[info.sizeAmount] = width  - 1;
            info.h[info.sizeAmount] = height - 1;
            info.sizeAmount++;
        }

        // if too many leave
        if (info.sizeAmount > 0xFF) {
            info.sizeAmount = 0xFFFF;
        }
    }

    if (info.sizeAmount == 0xFFFF) info.sizeAmount = 0;

    // set options
    info.options |= (uint8_t)info.sizeAmount;
    info.options |= xBits << 16;
    info.options |= yBits << 21;

    // set bitsize
    if (info.sizeAmount == 0)
        bitSize += encodingList->characterAmount * (xBits + yBits);
    else {
        bitSize += info.sizeAmount * (xBits + yBits);
        uint8_t indexWidth = __builtin_stdc_bit_width((uint16_t)(info.sizeAmount - 1));
        bitSize += encodingList->characterAmount * indexWidth;
    }
    info.size = bitSize / 8;

    #if DEBUG || DEBUG_PCF || DEBUG_PCF_BITM || DEBUG_PCF_BITM_INFO
    printf("\npcf  | bitm | info\n");
    printf("Sizes: %i\n", info.sizeAmount);
    for (uint32_t i = 0; i < info.sizeAmount; i++) {
        printf("%i: (%i, %i)\n", i, info.w[i], info.h[i]);
    }
    printf("Size in bits: %i\n\n", bitSize);
    printf("Option sizes indexed:  %i\n", ( info.options        & 0xFF));
    printf("Option colors indexed: %i\n", ((info.options >> 8)  & 0xFF));
    printf("Option size of x:      %i\n", ((info.options >> 16) & 0x1F) + 1);
    printf("Option size of y:      %i\n", ((info.options >> 21) & 0x1F) + 1);
    printf("Option channel amount: %i\n", ((info.options >> 26) & 0x3)  + 1);
    printf("Option channel size:   %i\n", ((info.options >> 28) & 0xF)  + 1);
    #endif

    return info;
}

static bool readBitmapGlyph(FILE* file, uint32_t i, PCF_MetricList* metricList, PCF_EncodingList* encodingList, BitWriter* writer, long offsetData, long bitmapData, PCF_AtlasInfo info, uint32_t format) { // Error TAG: pcf  | bitm | glyf
    // format values
    bool bswap   = (format & PCF_BYTE_MASK)   != 0;
    bool bitSwap = (format & PCF_BIT_MASK)    != 0;
    uint8_t unit = (format & PCF_BITMAP_UNIT) >> 4;
    uint8_t pad  = format & PCF_BITMAP_PAD;

    // error checking
    if (unit > pad || unit == 3 || pad == 3) {
        printf("pcf  | bitm | impossible unit or padding (could be a uint64_t).\n"); return false;
    }

    // get offset
    long offset = offsetData + encodingList->encodings[i].index * sizeof(uint32_t);
    if (!seekFile(file, offset, SEEK_SET, "pcf  | bitm | glyf", "offset of glyph")) return false;
    uint32_t bitmapOffset;
    if (!readU32(&bitmapOffset, 1, file, "pcf  | bitm | glyf", "offset into bitmap", bswap)) return false;

    // goto image
    if (!seekFile(file, bitmapData + bitmapOffset, SEEK_SET, "pcf  | bitm | glyf", "glyph data")) return false;

    // set bit index
    encodingList->encodings[i].bit = getBitPos(writer);

    // store size
    PCF_Metric metric = getMetric(metricList, i);
    uint16_t width = metric.rightSideBearing - metric.leftSidedBearing - 1;
    uint16_t height = metric.characterAscent + metric.characterDescent - 1;

    // if has indexed list
    if (info.sizeAmount) {
        for (uint16_t j = 0; j < info.sizeAmount; j++) {
            // search for index
            if (info.w[j] == width && info.h[j] == height) {
                pushBits(writer, j, __builtin_stdc_bit_width((uint8_t)(info.sizeAmount-1)));
                break;
            }
        }
    } else {
        pushBits(writer, width,  (info.options >> 16) & 0x1F);
        pushBits(writer, height, (info.options >> 21) & 0x1F);
    }

    // read bitmap
    for (uint32_t j = 0; j < height + 1; j++) {
        // scan row
        uint32_t scannedBits = 0;
        while (scannedBits < width + 1) {
            uint8_t currentScan; uint32_t val;
            switch (unit) {
                // read based on typeof value
                case PCF_BYTE:
                    // code for byte format
                    uint8_t byte; if (!readU8(&byte, 1, file, "pcf  | bitm | glyf", "bitmap byte")) return false;
                    if (!bitSwap) byte = bitSwap8(byte);
                    val = byte; currentScan = 8;
                    break;
                case PCF_SHORT:
                    // code for short format
                    uint16_t shortVal; if (!readU16(&shortVal, 1, file, "pcf  | bitm | glyf", "bitmap short", bswap)) return false;
                    if (!bitSwap) shortVal = bitSwap16(shortVal);
                    val = shortVal; currentScan = 16; break;
                case PCF_INT:
                    // code for int format
                    uint32_t intVal; if (!readU32(&intVal, 1, file, "pcf  | bitm | glyf", "bitmap int", bswap)) return false;
                    if (!bitSwap) intVal = bitSwap32(intVal);
                    val = intVal; currentScan = 32; break;
            }
            // finalize bits
            pushBits(writer, val, min16(width - scannedBits + 1, currentScan));
            scannedBits += currentScan;
        }

        // use padding
        long location;
        if (!tellFile(file, &location, "pcf  | bitm | glyf", "location for padding")) return false;
        if ((location - bitmapData) % padList[pad] != 0) {
            long relativeOffset = padList[pad] - (location - bitmapData) % padList[pad];
            if (!seekFile(file, relativeOffset, SEEK_CUR, "pcf  | bitm | glyf", "padding")) return false;
        }
    }

    #if DEBUG || DEBUG_PCF || DEBUG_PCF_BITM || DEBUG_PCF_BITM_GLYF
    printf("\nDEBUG_PCF_BITM_GLYF unimplimented\n");
    #endif

    return true;
}

static PCF_CharacterAtlas readBitmapsPCF(FILE* file, PCF_Tables* tables, PCF_MetricList* metricList, PCF_EncodingList* encodingList) { // Error TAG: pcf  | bitm
    // for errors
    PCF_CharacterAtlas nullAtlas = {NULL};

    // get info of atlas
    PCF_AtlasInfo info = getAtlasInfo(metricList, encodingList);
    if (info.options == 0) return nullAtlas;

    // goto table
    uint32_t offset = getTableOffset(tables, PCF_BITMAPS);
    if (!seekFile(file, offset, SEEK_SET, "pcf  | bitm", "bitmap table")) return nullAtlas;

    // check format
    uint32_t format;
    if (!readU32(&format, 1, file, "pcf  | bitm", "format", false)) return nullAtlas;
    bool bswap = (format & PCF_BYTE_MASK) != 0;

    // get glyphCount and offset table location
    uint32_t glyphCount;
    if (!readU32(&glyphCount, 1, file, "pcf  | bitm", "glyph count", bswap)) return nullAtlas;
    long offsetData;
    if (!tellFile(file, &offsetData, "pcf  | bitm", "offset data location")) return nullAtlas;

    // skip table and get data location
    if (!seekFile(file, (glyphCount + 4) * sizeof(uint32_t), SEEK_CUR, "pcf  | bitm", "bitmap data")) return nullAtlas;
    long bitmapData;
    if (!tellFile(file, &bitmapData, "pcf  | bitm", "bitmap data location")) return nullAtlas;

    // allocation
    uint32_t imagesSize = info.size + sizeof(AtlasPackedImages);
    AtlasPackedImages* images = allocate(imagesSize, "pcf  | bitm", "bitmap images");
    if (images == NULL) return nullAtlas;
    images->options = info.options;
    images->size = info.size;

    // bit writer preparation
    for (uint32_t i = 0; i < info.size; i++)
        images->bytes[i] = 0;
    BitWriter writer = {images->bytes};

    // fill index
    if (info.sizeAmount) {
        uint8_t sizeX = (info.options >> 16) & 0x1F;
        uint8_t sizeY = (info.options >> 21) & 0x1F;

        // fill
        for (uint16_t i = 0; i < info.sizeAmount; i++) {
            pushBits(&writer, info.w[i], sizeX);
            pushBits(&writer, info.h[i], sizeY);
        }
    }
    pushBits(&writer, 0b0001, 4); // preprocessed: 0 transparent, 1 filled

    // loop for getting data
    for (uint32_t i = 0; i < encodingList->characterAmount; i++) {
        readBitmapGlyph(file, i, metricList, encodingList, &writer, offsetData, bitmapData, info, format);
    }

    pushBufferBits(&writer);

    #if DEBUG || DEBUG_PCF || DEBUG_PCF_BITM || DEBUG_PCF_BITM_MAIN
    printf("\npcf  | bitm\n");
    for (uint32_t i = 0; i < encodingList->characterAmount; i++) {
        if (printf("%.4x %lc\n", i, encodingList->encodings[i].codepoint) < 0)
            printf("%.4x\n", i, encodingList->encodings[i].codepoint);
        BitReader r = {images->bytes, encodingList->encodings[i].bit};
        r.bit += __builtin_stdc_bit_width((uint8_t)(info.sizeAmount-1));
        PCF_Metric metric = getMetric(metricList, i);
        uint16_t width = metric.rightSideBearing - metric.leftSidedBearing;
        uint16_t height = metric.characterAscent + metric.characterDescent;
        for (uint16_t j = 0; j < height; j++) {
            printf("    %.2x|", j);
            for (uint16_t k = 0; k < width; k++) {
                uint64_t n = 0;
                readBits(&r, &n, 1);
                if (n) {
                    printf("█");
                } else {
                    printf(" ");
                }
            }
            printf("|\n");
        }
    }
    #endif
}

PCF_CharacterAtlas loadFilePCF(char* fileName, char32_t* include) { // Error TAG: pcf
    FILE* file = fopen(fileName, "rb");

    PCF_CharacterAtlas nullAtlas = {NULL};

    PCF_Tables* tables = readTableDirectory(file);
    if (tables == NULL) return nullAtlas;

    PCF_EncodingList* encodingList = readEncodingsPCF(file, tables, include);
    if (encodingList == NULL) return nullAtlas;

    PCF_MetricList* metricList = readMetricsPCF(file, tables, encodingList);
    if (metricList == NULL) return nullAtlas;

    // TODO refactor with new atlas and metrics
    PCF_CharacterAtlas atlas = readBitmapsPCF(file, tables, metricList, encodingList);
    if (atlas.images == NULL) return nullAtlas;

    //cleanup
    //free(tables);
    //free(metrics);

    return nullAtlas;
    //return atlas;
}
