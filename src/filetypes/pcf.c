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
    PCF_EncodingList* encodings = allocate(encodingsSize, "pcf  | enco | all ", "encodings list");
    if (encodings == NULL) return NULL;
    encodings->characterAmount = characterAmount;
    encodings->defaultCharacter = defaultCharacter;

    // 2nd scan
    if (!seekFile(file, encodingListOffset, SEEK_SET, "pcf  | enco | all ", "encoding data")) return NULL;

    uint16_t characterI = 0;
    for (uint16_t byte1 = minByte1; byte1 <= maxByte1; byte1++) {
        for (uint16_t byte2 = minByte2; byte2 <= maxByte2; byte2++) {
            uint16_t characterIndex;
            // store a character if it has a defined endpoint
            if (!readU16(&characterIndex, 1, file, "pcf  | enco | all ", "character index", bswap)) return NULL;
            if (characterIndex != 0xFFFF) {
                encodings->encodings[characterI].codepoint = ((uint16_t)byte1 * 0x100) + byte2;
                encodings->encodings[characterI].index = characterIndex;
                characterI++;
            }
        }
    }

    return encodings;
}

static PCF_EncodingList* readEncodingsPCFSome(FILE* file, uint16_t minByte1, uint16_t maxByte1, uint16_t minByte2, uint16_t maxByte2, bool bswap, uint16_t defaultCharacter, char32_t* include) { // Error TAG: pcf  | enco | some
    // Get length of max length 65536 for amount of unicode characters in the BMP(basic multilinguial plane 0x0-0xffff)
    uint32_t len = 0;
    for (uint32_t i = 0; i <= 0xffff; i++) {
        if (include[i] == 0)
            break;
        else if (include[i] <= 0xffff)
            len++;
        if (i == 0xffff)
            len = 0xffffffff; // impossible value for error checking
    }

    // error check for no null at end
    if (len == 0xffffffff) {
        printf("pcf  | enco | some | include length to large / not null terminated");
        return NULL;
    }

    // allocation
    uint32_t encodingsSize = sizeof(PCF_EncodingList) + len * sizeof(PCF_Encoding);
    PCF_EncodingList* encodings = allocate(encodingsSize, "pcf  | enco | some", "encodings list");
    if (encodings == NULL) return NULL;
    encodings->characterAmount = len;
    encodings->defaultCharacter = defaultCharacter;

    // remember position offseting into list
    long location;
    if (!tellFile(file, &location, "pcf  | enco | some", "encoding data")) return NULL;

    // set data in encodings
    for (uint32_t i = 0; i < len; i++) {
        // fill codepoint only if valid (in BMP)
        if (include[i] > 0xffff)
            continue;
        encodings->encodings[i].codepoint = (uint16_t)include[i];

        // calculation taken from source (source is in comment near file top)
        uint8_t  enc1 = (uint8_t)(include[i] >> 8);
        uint8_t  enc2 = (uint8_t)(include[i]);
        uint16_t index = (enc1-minByte1)*(maxByte2-minByte2+1)+enc2-minByte2;

        // goto and read from index
        if (!seekFile(file, location + index * sizeof(uint16_t), SEEK_SET, "pcf  | enco | some", "index")) return NULL;
        uint16_t temp;
        if (!readU16(&temp, 1, file, "pcf  | enco", "minimum of byte 2", bswap)) return NULL;
        encodings->encodings[i].index = temp;
    }

    return encodings;
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
    PCF_EncodingList* encodings;
    if (include == NULL) {
        // scans everything but is less efficient
        encodings =  readEncodingsPCFAll(file, minByte1, maxByte1, minByte2, maxByte2, bswap, defaultCharacter);
    } else {
        // only scans selected encodings and more efficient
        encodings = readEncodingsPCFSome(file, minByte1, maxByte1, minByte2, maxByte2, bswap, defaultCharacter, include);
    }

    // DEBUG print encodings
    /*for (uint16_t i = 0; i < encodings->characterAmount; i++) {
        printf("%lc: %i\n", encodings->encodings[i].codepoint, encodings->encodings[i].index);
    }*/

    return encodings;
}

static PCF_MetricList* readMetricsPCFFull(FILE* file, bool bswap, PCF_EncodingList* encoding) { // Error TAG: pcf  | metr | full
    // skip metric count and get array location
    if (!seekFile(file, sizeof(int32_t), SEEK_CUR, "pcf  | metr | all ", "past metric count")) return NULL;
    long arrayLocation;
    if (!tellFile(file, &arrayLocation, "pcf  | metr | all ", "metric data location")) return NULL;

    // allocate metric array
    uint32_t metricsSize = sizeof(PCF_MetricList) + encoding->characterAmount * sizeof(PCF_Metric);
    PCF_MetricList* metrics = allocate(metricsSize, "pcf  | metr | all ", "metrics list");
    if (metrics == NULL) return NULL;

    // loop over encodings for each metric
    for (uint16_t i = 0; i < encoding->characterAmount; i++) {
        // goto metrics for each encoding
        uint16_t index = (uint16_t)encoding->encodings[i].index;
        long metricLocation = arrayLocation + index * sizeof(PCF_Metric);
        if (!seekFile(file, metricLocation, SEEK_SET, "pcf  | metr | all ", "to metric")) return NULL;

        // get metrics
        PCF_Metric* metric = &metrics->metrics[i];
        if (!read16( &metric->leftSidedBearing, 1, file, "pcf  | metr | all ", "left sided bearing",  bswap)) return NULL;
        if (!read16( &metric->rightSideBearing, 1, file, "pcf  | metr | all ", "right side bearing",  bswap)) return NULL;
        if (!read16( &metric->characterWidth,   1, file, "pcf  | metr | all ", "character width",     bswap)) return NULL;
        if (!read16( &metric->characterAscent,  1, file, "pcf  | metr | all ", "character ascent",    bswap)) return NULL;
        if (!read16( &metric->characterDescent, 1, file, "pcf  | metr | all ", "character descent",   bswap)) return NULL;
        if (!readU16(&metric->characterAttrib,  1, file, "pcf  | metr | all ", "character attribute", bswap)) return NULL;
    }

    return metrics;
}

static PCF_MetricList* readMetricsPCFComp(FILE* file, bool bswap, PCF_EncodingList* encoding) { // Error TAG: pcf  | metr | comp
    // skip metric count and get array location
    if (!seekFile(file, sizeof(int16_t), SEEK_CUR, "pcf  | metr | comp", "past metric count")) return NULL;
    long arrayLocation;
    if (!tellFile(file, &arrayLocation, "pcf  | metr | comp", "metric data location")) return NULL;

    // allocate metric array
    uint32_t metricsSize = sizeof(PCF_MetricList) + encoding->characterAmount * sizeof(PCF_Metric);
    PCF_MetricList* metrics = allocate(metricsSize, "pcf  | metr | comp", "metrics list");
    if (metrics == NULL) return NULL;

    // loop over encodings for each metric
    for (uint16_t i = 0; i < encoding->characterAmount; i++) {
        // goto metrics for each encoding
        uint16_t index = (uint16_t)encoding->encodings[i].index;
        long metricLocation = arrayLocation +
        index * 5 * sizeof(uint8_t);
        if (!seekFile(file, metricLocation, SEEK_SET, "pcf  | metr | comp", "to metric")) return NULL;

        // get metrics
        PCF_Metric* metric = &metrics->metrics[i];
        uint8_t temp;
        if (!readU8(&temp, 1, file, "pcf  | metr | comp", "left sided bearing")) return NULL; metric->leftSidedBearing = (int16_t)temp - 0x80;
        if (!readU8(&temp, 1, file, "pcf  | metr | comp", "right side bearing")) return NULL; metric->rightSideBearing = (int16_t)temp - 0x80;
        if (!readU8(&temp, 1, file, "pcf  | metr | comp", "character width"   )) return NULL; metric->characterWidth   = (int16_t)temp - 0x80;
        if (!readU8(&temp, 1, file, "pcf  | metr | comp", "character ascent"  )) return NULL; metric->characterAscent  = (int16_t)temp - 0x80;
        if (!readU8(&temp, 1, file, "pcf  | metr | comp", "character descent" )) return NULL; metric->characterDescent = (int16_t)temp - 0x80;
        metric->characterAttrib  = 0;
    }

    return metrics;
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
    PCF_MetricList* metrics;
    if (format & PCF_COMPRESSED_METRICS) {
        metrics = readMetricsPCFComp(file, bswap, encoding);
    } else {
        metrics = readMetricsPCFFull(file, bswap, encoding);
    }

    // DEBUG display metrics
    /*for (uint16_t i = 0; i < encoding->characterAmount; i++) {
        uint16_t codepoint = encoding->encodings[i].codepoint;
        uint16_t width = metrics->metrics[i].rightSideBearing - metrics->metrics[i].leftSidedBearing;
        uint16_t height = metrics->metrics[i].characterAscent + metrics->metrics[i].characterDescent;
        printf("%lc: %i x %i\n", codepoint, width, height); // display character width and height
    }*/

    return metrics;
}

static PCF_AtlasInfo getAtlasInfo(PCF_MetricList* metrics, PCF_EncodingList* encoding) { // Error TAG: pcf  | bitm | size // TODO only use indexed sizes mode if there is a certain amount of duplicates
    PCF_AtlasInfo info = {0};

    // check if size is indexed by counting unique sizes
    uint16_t length = 0;
    for (uint32_t i = 0; i < encoding->characterAmount; i++) {
        uint16_t width = metrics->metrics[i].rightSideBearing - metrics->metrics[i].leftSidedBearing;
        uint16_t height = metrics->metrics[i].characterAscent + metrics->metrics[i].characterDescent;

        // check if size already exists
        bool included = false;
        for (uint16_t j = 0; j < length; j++) {
            if (info.sizes[2*j] == width && info.sizes[2*j+1] == height) {
                included = true;
                break;
            }
        }

        if (!included) {
            // if too many sizes break
            if (length == 0xFF)
                break;

            // if it doesnt exist add it to the list
            info.sizes[length*2]   = width;
            info.sizes[length*2+1] = height;
            length++;
        }
    }

    // actual variable
    bool indexedSize = length <= 0xFF;
    info.sizeAmount = length;

    // calculate size and options
    info.size = 0;
    info.options = 0x00000000;

    // set options for size indexed
    if (indexedSize)
        info.options |= length;

    // set options for color indexed
    info.options |= 0x2 << 8;

    // get bit size of x and y (__builtin_stdc_bit_width gets amount of bits, much quicker then log2)
    uint8_t sizeX = 0;
    uint8_t sizeY = 0;
    if (indexedSize) {
        for (uint16_t i = 0; i < length; i++) {
            sizeX = maxU8(sizeX, __builtin_stdc_bit_width((uint8_t)(info.sizes[i*2]-1)));
            sizeY = maxU8(sizeY, __builtin_stdc_bit_width((uint8_t)(info.sizes[i*2+1]-1)));
        }
    } else {
        for (uint32_t i = 0; i < encoding->characterAmount; i++) {
            uint16_t width = metrics->metrics[i].rightSideBearing - metrics->metrics[i].leftSidedBearing;
            uint16_t height = metrics->metrics[i].characterAscent + metrics->metrics[i].characterDescent;
            sizeX = maxU8(sizeX, __builtin_stdc_bit_width((uint8_t)(width-1)));
            sizeY = maxU8(sizeY, __builtin_stdc_bit_width((uint8_t)(height-1)));
        }
    }

    // fill bit sizes of size
    info.options |= sizeX << 16;
    info.options |= sizeY << 21;

    // fill color options
    info.options |= 0x1 << 26; // channel amount (2 for black and see-through)
    info.options |= 0x0 << 28; // channel size (1 for absolute values)

    // now fill actual size of data (in bytes)
    uint32_t bitsize = 0;

    // calculate pallette sizes
    bitsize += (info.options & 0xFF) * (sizeX + sizeY); // sizes
    bitsize += 4; // colors, precalculated

    // now calculate bitmap portion
    for (uint32_t i = 0; i < encoding->characterAmount; i++) {
        uint16_t width = metrics->metrics[i].rightSideBearing - metrics->metrics[i].leftSidedBearing;
        uint16_t height = metrics->metrics[i].characterAscent + metrics->metrics[i].characterDescent;
        bitsize += width * height;
    }

    // add sizes per images
    if (indexedSize) {
        bitsize += __builtin_stdc_bit_width((uint8_t)(length-1)) * encoding->characterAmount;
    } else {
        bitsize += (sizeX + sizeY) * encoding->characterAmount;
    }

    // final size calculation
    info.size = (bitsize & 0x7) ? bitsize / 8 + 1 : bitsize / 8;
    info.size += sizeof(uint32_t) * 2; // for options and size

    // DEBUG check info
    /*printf("Size: %i.\n", info.size);
    printf("Options: %X.\n", info.options);
    printf("Size of index for size: %X.\n", info.sizeAmount);
    for (uint16_t i = 0; i < info.sizeAmount; i++) {
        printf("(%i, %i)\n", info.sizes[i*2], info.sizes[i*2+1]);
    }*/

    return info;
}

static PCF_CharacterAtlas readBitmapsPCF(FILE* file, PCF_Tables* tables, PCF_MetricList* metrics, PCF_EncodingList* encoding) { // Error TAG: pcf  | bitm
    // for errors
    PCF_CharacterAtlas nullAtlas = {NULL, NULL};

    // goto table
    uint32_t offset = getTableOffset(tables, PCF_BITMAPS);
    if (!seekFile(file, offset, SEEK_SET, "pcf  | bitm", "metrics")) return nullAtlas;

    // check format
    uint32_t format;
    if (!readU32(&format, 1, file, "pcf  | bitm", "format", false)) return nullAtlas;

    // various values derived from the format
    bool bswap   = (format & PCF_BYTE_MASK)   != 0;
    bool bitSwap = (format & PCF_BIT_MASK)    != 0;
    uint8_t unit = (format & PCF_BITMAP_UNIT) >> 4; // for how to bswap
    uint8_t pad  = format & PCF_BITMAP_PAD;         // for how its stored

    // error checking
    if (unit == 3 || pad == 3) {
        printf("pcf  | bitm | impossible unit or padding (could be a uint64_t).\n");
    }

    if (unit > pad) {
        printf("pcf  | bitm | REALLY impossible padding or unit.\n");
    }

    // get count of glyphs offseting
    uint32_t glyphCount;
    if (!readU32(&glyphCount, 1, file, "pcf  | bitm", "glyph count", bswap)) return nullAtlas;

    // offset list location saved
    long offsetList;
    if (!tellFile(file, &offsetList, "pcf  | bitm", "offset list location")) return nullAtlas;

    // get to main data
    if (!seekFile(file, (glyphCount + 4) * sizeof(uint32_t), SEEK_CUR, "pcf  | bitm", "bitmap data")) return nullAtlas;

    // save data location
    long offsetData;
    if (!tellFile(file, &offsetData, "pcf  | bitm", "bitmap data location")) return nullAtlas;

    // get info of what the atlas should abide to
    PCF_AtlasInfo info = getAtlasInfo(metrics, encoding);

    // atlas allocation and basic creation
    uint32_t atlasSize = info.size; // size variable
    atlasSize += sizeof(uint32_t) - atlasSize % sizeof(uint32_t); // round to uint32_t for shader
    ImageAtlasPackedImages* atlas = allocate(atlasSize, "pcf  | bitm", "atlas");
    if (atlas == NULL) return nullAtlas;
    atlas->options = info.options;
    atlas->size = info.size;

    // set bytes to 0
    for (uint32_t i = 0; i < info.size - sizeof(uint32_t); i++)
        atlas->bytes[i] = 0;

    // fill atlas size index (if can)
    BitWriter writer = {atlas->bytes};
    uint8_t indexSizeAmount = info.options & 0xFF;
    if (indexSizeAmount) {
        uint8_t sizeX = (info.options >> 16) & 0x1F;
        uint8_t sizeY = (info.options >> 21) & 0x1F;

        // fill
        for (uint16_t i = 0; i < indexSizeAmount; i++) {
            pushBits(&writer, info.sizes[i*2]-1, sizeX);
            pushBits(&writer, info.sizes[i*2+1]-1, sizeY);
        }
    }

    // preprocessed, push color pallette
    pushBits(&writer, 0b0001, 4); // 0 transparent, 1 filled

    // bitmaps
    for (uint32_t i = 0; i < encoding->characterAmount; i++) {
        // get offset
        long offset = offsetList + encoding->encodings[i].index * sizeof(uint32_t);
        if (!seekFile(file, offset, SEEK_SET, "pcf  | bitm", "offset of glyph")) return nullAtlas;
        uint32_t bitmapOffset;
        if (!readU32(&bitmapOffset, 1, file, "pcf  | bitm", "offset into bitmap", bswap)) return nullAtlas;

        // goto image
        offset = offsetData + bitmapOffset;
        if (!seekFile(file, offset, SEEK_SET, "pcf  | bitm", "data of glyph")) return nullAtlas;

        // set bit index
        encoding->encodings[i].bit = getBitPos(&writer);

        // store size
        uint16_t width = metrics->metrics[i].rightSideBearing - metrics->metrics[i].leftSidedBearing;
        uint16_t height = metrics->metrics[i].characterAscent + metrics->metrics[i].characterDescent;
        if (indexSizeAmount) {
            // if has indexed list
            for (uint16_t j = 0; j < indexSizeAmount; j++) {
                // search for index
                if (info.sizes[j*2] == width && info.sizes[j*2+1] == height) {
                    pushBits(&writer, j, __builtin_stdc_bit_width((uint8_t)(indexSizeAmount-1)));
                    break;
                }
            }
        } else {
            pushBits(&writer, width-1, (info.options >> 16) & 0x1F);
            pushBits(&writer, height-1, (info.options >> 21) & 0x1F);
        }

        // read bitmap
        for (uint32_t j = 0; j < height; j++) {
            // scan row
            uint32_t scannedBits = 0;
            while (scannedBits < width) {
                switch (unit) {
                    // read based on typeof value
                    case PCF_BYTE:
                        uint8_t byte;
                        if (!readU8(&byte, 1, file, "pcf  | bitm", "bitmap byte")) return nullAtlas;
                        if (!bitSwap) byte=bitSwap8(byte);
                        pushBits(&writer, byte, min16(width - scannedBits, 8));
                        scannedBits += 8;
                        break;
                    case PCF_SHORT:
                        uint16_t shortVal;
                        if (!readU16(&shortVal, 1, file, "pcf  | bitm", "bitmap short", bswap)) return nullAtlas;
                        if (!bitSwap) shortVal=bitSwap16(shortVal);
                        pushBits(&writer, shortVal, min16(width - scannedBits, 16));
                        scannedBits += 16;
                        break;
                    case PCF_INT:
                        uint32_t intVal;
                        if (!readU32(&intVal, 1, file, "pcf  | bitm", "bitmap int", bswap)) return nullAtlas;
                        if (!bitSwap) intVal=bitSwap32(intVal);
                        pushBits(&writer, intVal, min16(width - scannedBits, 32));
                        scannedBits += 32;
                        break;
                    default:
                        printf("pcf  | bitm | impossible branch reached.\n");
                        return nullAtlas;
                }
            }
            // use padding
            long location;
            switch (pad) {
                // pad based on typeof value
                case PCF_BYTE:
                    break;
                case PCF_SHORT:
                    if (!tellFile(file, &location, "pcf  | bitm", "location for padding")) return nullAtlas;
                    if (!seekFile(file, (location+1)/2*2, SEEK_SET, "pcf  | bitm", "padding")) return nullAtlas;
                    break;
                case PCF_INT:
                    if (!tellFile(file, &location, "pcf  | bitm", "location for padding")) return nullAtlas;
                    if (!seekFile(file, (location+3)/4*4, SEEK_SET, "pcf  | bitm", "padding")) return nullAtlas;
                    break;
                default:
                    printf("pcf  | bitm | impossible branch reached.\n");
                    return nullAtlas;
            }

        }
    }


    // push final bits
    pushBufferBits(&writer);

    //HACK render letter 8*16
    /*uint16_t bit = encoding->encodings[0].bit;
    for (uint16_t i = 0; i < 128; i++) {
        int byte = (i + bit - 1) / 8;
        int bitN = (i + bit - 1) % 8;
        bool ink = 0x80 & (atlas->bytes[byte] << bitN);

        if (ink) {
            printf("█");
        } else {
            printf(" ");
        }
        if ((i%8)==7)
            printf("\n");
    }*/

    //DEBUG
    /*for (uint32_t i = 0; i < info.size - sizeof(uint32_t); i++) {
        printf("%.2x", atlas->bytes[i]);
    }
    printf("\n%i\n", writer.byte);*/

    PCF_CharacterAtlas finalAtlas = {atlas, encoding};
    return finalAtlas;
}

PCF_CharacterAtlas loadFilePCF(char* fileName, char32_t* include) { // Error TAG: pcf
    FILE* file = fopen(fileName, "rb");

    PCF_CharacterAtlas nullAtlas = {NULL, NULL};

    PCF_Tables* tables = readTableDirectory(file);
    if (tables == NULL) return nullAtlas;

    PCF_EncodingList* encoding = readEncodingsPCF(file, tables, include);
    if (encoding == NULL) return nullAtlas;

    PCF_MetricList* metrics = readMetricsPCF(file, tables, encoding);
    if (metrics == NULL) return nullAtlas;

    PCF_CharacterAtlas atlas = readBitmapsPCF(file, tables, metrics, encoding);
    if (atlas.images == NULL) return nullAtlas;

    //cleanup
    free(tables);
    free(metrics);

    // HACK show a letter
    /*uint16_t i;
    for (i = 0; i < bitmaps.pointers->characterAmount; i++) {
        if (bitmaps.pointers->encodings[i].codepoint == 0x4e3a)
            break;
    }
    printf("%i\n", bitmaps.pointers->encodings[i].index);
    for (uint16_t j = 0; j < 256; j++) {
        if (bitmaps.images->bytes[j + bitmaps.pointers->encodings[i].index] == 0x1) {
            printf("0");
        } else {
            printf(" ");
        }
        if (j / 16 * 16 == j)
            printf("\n");
    }*/

    return atlas;
}
