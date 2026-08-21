#include "ttf.h"

#define TTF_VERSION_CODE      0x00010000
#define CFF_VERSION_CODE      0x4F54544F

#define CMAP_TAG              0x70616D63
#define CMAP_VERSION          0x0000
#define CMAP_UNICODE_CODE     0x0000
#define CMAP_UNICODE_IDV2_BMP 0x0003

#define HEAD_TAG              0x64616568
#define HEAD_MAJOR_VERSION    0x0001
#define HEAD_MINOR_VERSION    0x0000
#define HEAD_MAGIC_NUMBER     0x5F0F3CF5

#define MAXP_TAG              0x7078616D
#define MAXP_TTF_VERSION      0x00010000

#define LOCA_TAG              0x61636F6C

#define GLYF_TAG              0x66796C67
#define GLYF_X_SHORT          0x02
#define GLYF_Y_SHORT          0x04
#define GLYF_X_SAME_SHORT_POS 0x10
#define GLYF_Y_SAME_SHORT_POS 0x20

#define GDEF_TAG              0x46454447

static inline bool seekFile(FILE* file, long offset, int origin, const char* errorType, const char* locationName) {
    // Helper fseek with error checking
    int e = fseek(file, offset, origin);
    if (e != 0) {
        printf("%s | Failed to seek to %s.\n", errorType, locationName);
        return false;
    }
    return true;
}

static inline bool tellFile(FILE* file, long* buffer, const char* errorType, const char* locationName) {
    // Helper fseek with error checking
    *buffer = ftell(file);
    if (*buffer == -1L) {
        printf("%s | Failed to tell location of %s.\n", errorType, locationName);
        return false;
    }
    return true;
}

static inline bool readU8(uint8_t* buffer, size_t count, FILE* file, const char* errorType, const char* intName) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(uint8_t), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, intName);
        return false;
    }
    return true;
}

static inline bool readU16(uint16_t* buffer, size_t count, FILE* file, const char* errorType, const char* intName) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(uint16_t), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, intName);
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        *(buffer+i) = __builtin_bswap16(*(buffer+i));
    }
    return true;
}

static inline bool readU32(uint32_t* buffer, size_t count, FILE* file, const char* errorType, const char* intName) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(uint32_t), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, intName);
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        *(buffer+i) = __builtin_bswap32(*(buffer+i));
    }
    return true;
}

static inline bool read16(int16_t* buffer, size_t count, FILE* file, const char* errorType, const char* intName) {
    // Helper fread with error checking
    size_t size = fread(buffer, sizeof(int16_t), count, file);
    if (size != count) {
        printf("%s | File cuts off at %s.\n", errorType, intName);
        return false;
    }
    for (size_t i = 0; i < count; i++) {
        *(buffer+i) = __builtin_bswap16(*(buffer+i));
    }
    return true;
}

static inline bool readList() {return true;} // TODO make this function

static TTF_Tables* readTableDirectory(FILE* file) { // Error TAG: ttf  | tdir
    /* STEP 1: Read Table Directory
     *   uint32 sfntVersion:
     *       TTF - 0x00010000
     *       CFF - 0x4F54544F
     *   uint16 numTables     - number of tables
     *   uint16 searchRange   = 16 * 2**log2(numTables)
     *   uint16 entrySelector = log2(numTables)
     *   uint16 rangeShift    = numTables * 16 - searchRange
     *
     *   record tableRecords[numTables]
     */

    // Table Directory is at 0 in the file
    if (!seekFile(file, 0, SEEK_SET, "ttf  | tdir", "0")) return NULL;

    // Read sfntVersion: 0x00010000 = TTF, 0x4F54544F = CFF
    uint32_t sfntVersion;
    if (!readU32(&sfntVersion, 1, file, "ttf  | tdir", "sfntVersion")) return NULL;

    // Error check for codes
    if (sfntVersion == CFF_VERSION_CODE) {
        printf("ttf  | tdir | Not accepting CCF for sfntVersion.\n");
        return NULL;
    } else if (sfntVersion != TTF_VERSION_CODE) {
        printf("ttf  | tdir | Not a valid sfntVersion.\n");
        return NULL;
    }

    // Reads and checks number of tables
    uint16_t numTables;
    if (!readU16(&numTables, 1, file, "ttf  | tdir", "numTables")) return NULL;

    // Skips irrelevant Table Directory parts
    if (!seekFile(file, 6, SEEK_CUR, "ttf  | tdir", "past searchRange, entrySelector, rangeShift")) return NULL;

    // Mallocs based on amount of records
    uint32_t tablesSize = sizeof(TTF_Tables) + sizeof(TTF_TableRecord) * numTables;
    TTF_Tables* tables;
    if (!(tables = allocate(tablesSize, "ttf  | tdir", "Table Directory"))) return NULL;

    // Fill in tables
    tables->numTables = numTables;

    // TODO add helper function for this
    // Write after numTables in tables, the Table Directory
    size_t size = fread(tables->tables, sizeof(TTF_TableRecord), numTables, file);
    if (size == 0) {
        printf("ttf  | tdir | Failed to read Table Directory.\n");
        return NULL;
    } else if (size != numTables) {
        printf("ttf  | tdir | Failed to read all items in Table Directory.\n");
        return NULL;
    }

    // Byte swap all of tables numbers
    for (uint16_t i = 0; i < numTables; i++) {
        // Dont swap tag as it is a string
        tables->tables[i].checksum = __builtin_bswap32(tables->tables[i].checksum);
        tables->tables[i].offset   = __builtin_bswap32(tables->tables[i].offset);
        tables->tables[i].length   = __builtin_bswap32(tables->tables[i].length);
    }

    return tables;
}

static inline uint32_t getTableOffset(TTF_Tables* tables, uint32_t tag) {
    // Helper for readTable____ functions
    for (uint16_t i = 0; i < tables->numTables; i++) {
        if (tables->tables[i].tableTag == tag) {
            // Goto cmap
            return  tables->tables[i].offset;
        }
    }
    return 0;
}

// TODO document steps & data in basically everything but table dictionary
static TTF_SimplifiedCMAP* readTableCMAP(FILE* file, TTF_Tables* tables) { // Error TAG: ttf  | cmap
    // Find cmap in table directory and goto it
    uint32_t offset = getTableOffset(tables, CMAP_TAG);
    if (!seekFile(file, offset, SEEK_SET, "ttf  | cmap", "cmap")) return NULL;

    // Check if found an item
    if (offset == 0) {
        printf("ttf  | cmap | Couldnt find cmap in Table Directory.\n");
        return NULL;
    }

    // Error checking version number
    uint16_t version;
    if (!readU16(&version, 1, file, "ttf  | cmap", "version")) return NULL;

    // Check if correct version
    if (version != CMAP_VERSION) {
        printf("ttf  | cmap | Wrong cmap version.\n");
        return NULL;
    }

    // Reads and bswaps the number of subtables
    uint16_t numTables;
    if (!readU16(&numTables, 1, file, "ttf  | cmap", "numTables")) return NULL;

    // TODO add windows support
    // Get platform and encoding
    uint32_t subOffset = 0;
    for (uint16_t i = 0; i < numTables; i++) {

        // Read and error check platformID
        uint16_t platformID;
        if (!readU16(&platformID, 1, file, "ttf  | cmap", "platformID")) return NULL;

        // Read and error check encodingID
        uint16_t encodingID;
        if (!readU16(&encodingID, 1, file, "ttf  | cmap", "encodingID")) return NULL;

        // TODO support other encodings for unicode
        // Check the encodings
        if (platformID == CMAP_UNICODE_CODE && encodingID == CMAP_UNICODE_IDV2_BMP) {
            // Read offset
            if (!readU32(&subOffset, 1, file, "ttf  | cmap", "subOffset")) return NULL;
            break;
        }

        // Skip offset if invalid IDs
        if (!seekFile(file, offset, SEEK_SET, "ttf  | cmap", "past offset")) return NULL;
    }

    // Check if found a usable item
    if (subOffset == 0) {
        printf("ttf  | cmap | Failed to find suitable subtable.");
        return NULL;
    }

    // Goto subtable at the sum of the offsets
    if (!seekFile(file, offset + subOffset, SEEK_SET, "ttf  | cmap", "subtable")) return NULL;

    // Read the format
    uint16_t format;
    if (!readU16(&format, 1, file, "ttf  | cmap", "format")) return NULL;

    // TODO add more formats
    // Check if it is a usable formatcharacterCode
    if (format != 4) {
        printf("ttf  | cmap | This format is not implemented.\n");
        return NULL;
    }

    // Read length of subtable
    uint16_t length;
    if (!readU16(&length, 1, file, "ttf  | cmap", "length")) return NULL;

    // Error check language
    uint16_t language;
    if (!readU16(&language, 1, file, "ttf  | cmap", "language")) return NULL;
    if (language != 0) { // Only 0 in Macintosh systems
        printf("ttf  | cmap | Subtable uses Macintosh somehow.\n");
        return NULL;
    }

    // Read segCount
    uint16_t segCountX2;
    if (!readU16(&segCountX2, 1, file, "ttf  | cmap", "segCountX2")) return NULL;
    uint16_t segCount = segCountX2 / 2;

    // Skip irrelevant items
    if (!seekFile(file, 6, SEEK_CUR, "ttf  | cmap", "past subtable items")) return NULL;

    // Allocate character segment list
    uint16_t segmentListSize = sizeof(TTF_SegmentsCMAP) + sizeof(TTF_SegmentCMAP) * segCount;
    TTF_SegmentsCMAP* segmentsList;
    if (!(segmentsList = allocate(segmentListSize, "ttf  | cmap", "Segments"))) return NULL;

    // Fill segment list endpoints
    for (uint16_t i = 0; i < segCount; i++) {
        uint16_t* location = &segmentsList->segments[i].endCodepoint;
        if (!readU16(location, 1, file, "ttf  | cmap", "endCodepoint")) return NULL;
    }

    // Error check reservedPad
    uint16_t reservedPad;
    if (!readU16(&reservedPad, 1, file, "ttf  | cmap", "reservedPad")) return NULL;
    if (reservedPad != 0) {
        printf("ttf  | cmap | The reservedPad in the cmap encoding is not 0.\n");
        return NULL;
    }

    // Fill segment list startpoints
    for (uint16_t i = 0; i < segCount; i++) {
        uint16_t* location = &segmentsList->segments[i].startCodepoint;
        if (!readU16(location, 1, file, "ttf  | cmap", "startCodepoint")) return NULL;
    }

    // Fill segment list deltas
    for (uint16_t i = 0; i < segCount; i++) {
        int16_t* location = &segmentsList->segments[i].idDelta;
        if (!read16(location, 1, file, "ttf  | cmap", "idDelta")) return NULL;
    }

    // Fill segment list range offsets
    for (uint16_t i = 0; i < segCount; i++) {
        uint16_t* location = &segmentsList->segments[i].idRangeOffset;
        if (!readU16(location, 1, file, "ttf  | cmap", "idRangeOffset")) return NULL;
    }

    // Find codepoint amount
    uint16_t charCount = 0;
    for (uint16_t i = 0; i < segCount; i++) {
        TTF_SegmentCMAP segment = segmentsList->segments[i];
        charCount += segment.endCodepoint - segment.startCodepoint + 1;
    }

    // Allocate cmap
    uint16_t cmapSize = sizeof(TTF_SimplifiedCMAP) + sizeof(TTF_RecordCMAP) * charCount;
    TTF_SimplifiedCMAP* charMap;
    if (!(charMap = allocate(cmapSize, "ttf  | cmap", "charMap"))) return NULL;
    charMap->numCodepoints = charCount;

    // Store location of glyphIdArray
    long glyphIdArrayPos;
    if (!tellFile(file, &glyphIdArrayPos, "ttf  | cmap", "glyphIdArray")) return NULL;

    // Fill cmap
    uint16_t id = 0;
    for (uint16_t i = 0; i < segCount; i++) {
        // Gets segment data
        uint16_t startPoint    = segmentsList->segments[i].startCodepoint;
        uint16_t endPoint      = segmentsList->segments[i].endCodepoint;
        int16_t  idDelta       = segmentsList->segments[i].idDelta;
        uint16_t idRangeOffset = segmentsList->segments[i].idRangeOffset;

        // Last codepoint is 65535 and will loop forever if not skipped
        if (endPoint == 0xFFFF) break;

        for (uint16_t c = startPoint; c <= endPoint; c++) {
            id++;

            // Logic for codepoints to glyphs
            charMap->codepoints[id].codepoint = c;

            // If 0 use idDelta
            if (idRangeOffset == 0)
                charMap->codepoints[id].glyphID = c + idDelta;
            else {
                // TODO HACK errrr I dont really understand this code
                long filePos = idRangeOffset + 2 * ((c - startPoint) - (segCount - i));
                // Goes to filePos and reads it
                if (!seekFile(file, glyphIdArrayPos, SEEK_SET, "ttf  | cmap", "start of glyphIdArray")) return NULL;
                if (!seekFile(file, filePos, SEEK_CUR, "ttf  | cmap", "into glyphIdArray")) return NULL;
                if (!readU16(&charMap->codepoints[id].glyphID, 1, file, "ttf  | cmap", "glyphID")) return NULL;
            }
        }
    }

    // Cleanup
    free(segmentsList);

    return charMap;
}

static TTF_GlyphMetadata* readTableHEAD(FILE* file, TTF_Tables* tables, TTF_GlyphMetadata* metadata) { // Error TAG: ttf  | head
    // Find head in table directory and goto it
    uint32_t offset = getTableOffset(tables, HEAD_TAG);
    if (!seekFile(file, offset, SEEK_SET, "ttf  | head", "head")) return NULL;

    // Check if found an item
    if (offset == 0) {
        printf("ttf  | head | Couldnt find head in Table Directory.\n");
        return NULL;
    }

    // Error checking version number
    uint16_t majorVersion;
    if (!readU16(&majorVersion, 1, file, "ttf  | head", "majorVersion")) return NULL;
    uint16_t minorVersion;
    if (!readU16(&minorVersion, 1, file, "ttf  | head", "minorVersion")) return NULL;

    if (!(majorVersion == HEAD_MAJOR_VERSION && minorVersion == HEAD_MINOR_VERSION)) {
        printf("ttf  | head | Wrong version code.\n");
        return NULL;
    }

    // Skip to magic number
    if (!seekFile(file, sizeof(uint32_t) * 2, SEEK_CUR, "ttf  | head", "magic number")) return NULL;

    // Error checking magic number
    uint32_t magicNumber;
    if (!readU32(&magicNumber, 1, file, "ttf  | head", "magic number")) return NULL;

    if (magicNumber != HEAD_MAGIC_NUMBER) {
        printf("ttf  | head | Wrong magic number.\n");
        return NULL;
    }

    // Skip to indexToLocFormat
    if (!seekFile(file, sizeof(uint16_t) * 9 + sizeof(uint64_t) * 2, SEEK_CUR, "ttf  | head", "indexToLocFormat")) return NULL;

    // Read indexToLocFormat
    if (!read16(&metadata->indexToLocFormat, 1, file, "ttf  | head", "indexToLocFormat")) return NULL;

    // Error check indexToLocFormat
    if (metadata->indexToLocFormat != 1 && metadata->indexToLocFormat != 0) {
        printf("ttf  | head | No valid data for indexToLocFormat.\n");
        return NULL;
    }

    return metadata;
}

static TTF_GlyphMetadata* readTableMAXP(FILE* file, TTF_Tables* tables, TTF_GlyphMetadata* metadata) { // Error TAG: ttf  | maxp
    // Find maxp in table directory and goto it
    uint32_t offset = getTableOffset(tables, MAXP_TAG);
    if (!seekFile(file, offset, SEEK_SET, "ttf  | maxp", "maxp")) return NULL;

    // Check if found an item
    if (offset == 0) {
        printf("ttf  | maxp | Couldnt find maxp in Table Directory.\n");
        return NULL;
    }

    // Error check version
    uint32_t version;
    if (!readU32(&version, 1, file, "ttf  | maxp", "version")) return NULL;

    if (version != MAXP_TTF_VERSION) {
        printf("ttf  | maxp | Invalid version number.\n");
        return NULL;
    }

    // Find number of glyphs
    if (!read16(&metadata->numGlyphs, 1, file, "ttf  | maxp", "number of glyphs")) return NULL;

    return metadata;
}

static uint32_t* readTableLOCA(FILE* file, TTF_Tables* tables, TTF_GlyphMetadata metadata) { // Error TAG: ttf  | loca
    // Find loca in table directory and goto it
    uint32_t offset = getTableOffset(tables, LOCA_TAG);
    if (!seekFile(file, offset, SEEK_SET, "ttf  | loca", "loca")) return NULL;

    // Check if found an item
    if (offset == 0) {
        printf("ttf  | loca | Couldnt find loca in Table Directory.\n");
        return NULL;
    }

    // Allocate storage needed for offsets
    uint32_t* list;
    if (!(list = allocate(sizeof(uint32_t) * (metadata.numGlyphs + 1), "ttf  | loca", "index to glpyh list"))) return NULL;

    // Branch depending on loca format
    if (metadata.indexToLocFormat) {
        // Normal uint32_t types
        if (!readU32(list, metadata.numGlyphs + 1, file, "ttf  | loca", "index to glyph list")) return NULL;
    } else {
        // TODO Halved uint16_t types
    }

    return list;
}

static TTF_CharsAndGlyphs* readTableGLYF(FILE* file, TTF_Tables* tables, TTF_SimplifiedCMAP* charMap, uint32_t* offsets, char* include) { // Error TAG: ttf  | glyf
    // Find glyf in table directory and goto it
    uint32_t offset = getTableOffset(tables, GLYF_TAG);
    if (!seekFile(file, offset, SEEK_SET, "ttf  | glyf", "glyf")) return NULL;

    // Check if found an item
    if (offset == 0) {
        printf("ttf  | glyf | Couldnt find glyf in Table Directory.\n");
        return NULL;
    }

    // Allocate the character to glyph map TODO check include for repeated characters
    uint32_t charToGlyphSize = sizeof(TTF_CharToGlyph) + sizeof(TTF_Character) * strlen(include);
    if (include == NULL) charToGlyphSize = sizeof(TTF_CharToGlyph) + sizeof(TTF_Character) * charMap->numCodepoints;
    TTF_CharToGlyph* charToGlyph;
    if (!(charToGlyph = allocate(charToGlyphSize, "ttf  | glyf", "charToGlyph"))) return NULL;
    charToGlyph->characterAmount = 0;

    uint16_t m = 0;
    // Loop over characters
    uint16_t realI = 0;
    uint32_t p = 0;
    for (uint16_t i = 0; i < charMap->numCodepoints; i++) {
        // conversion // TODO when using 4 byte characters have full conversion
        char charUTF8[4] = {0, 0, 0, 0};
        uint16_t codepoint = charMap->codepoints[i].codepoint;
        if (codepoint <= 0x7F) {
            charUTF8[0] = (uint8_t)codepoint;
        } else if (codepoint <= 0x7FF) {
            charUTF8[0] = 0b11000000;
            charUTF8[0] |= (codepoint & 0x0700) >> 6;
            charUTF8[0] |= (codepoint & 0x00C0) >> 6;
            charUTF8[1] = 0b10000000;
            charUTF8[1] |= codepoint & 0x003F;
        } else {
            charUTF8[0] = 0b11100000;
            charUTF8[0] |= (codepoint & 0xF000) >> 12;
            charUTF8[1] = 0b10000000;
            charUTF8[1] |= (codepoint & 0x0F00) >> 6;
            charUTF8[1] |= (codepoint & 0x00C0) >> 6;
            charUTF8[2] = 0b10000000;
            charUTF8[2] |= codepoint & 0x003F;
        }

        // Only use include characters
        if (include != NULL && strstr(include, charUTF8) == NULL) continue;

        // printf("%s%lc\n", charUTF8, codepoint); // checking what gets scanned
        // Add 1 for each character and fill charToGlyph
        charToGlyph->characterAmount++;
        charToGlyph->characters[realI].codepoint = codepoint;

        // Seek to location using loca and cmap
        uint32_t location = offsets[charMap->codepoints[i].glyphID] + offset;
        if (!seekFile(file, location, SEEK_SET, "ttf  | glyf", "character")) return NULL;

        // temp set charToGlyph index to file offset
        charToGlyph->characters[realI].index = location;

        // Read number of Contours
        int16_t numOfContours;
        if (!read16(&numOfContours, 1, file, "ttf  | glyf", "contour number")) return NULL;

        // Full Contour amount
        charToGlyph->characters[realI].contourAmount = numOfContours;

        // TODO add compound glyphs
        // Count number of missed compound glyphs
        if (numOfContours <= 0) {m++; continue;}

        // Seek after x/y min/max
        if (!seekFile(file, sizeof(int16_t) * 4, SEEK_CUR, "ttf  | glyf", "after x/y metadata")) return NULL;

        // Goto Last contour
        if (!seekFile(file, sizeof(uint16_t) * (numOfContours - 1), SEEK_CUR, "ttf  | glyf", "last point in contour")) return NULL;

        // Read amount of points
        uint16_t numOfPoints;
        if (!readU16(&numOfPoints, 1, file, "ttf  | glyf", "point number")) return NULL;

        // Sum total points
        p += numOfPoints + 1;
        realI++;
    }

    // Make the array for points
    uint32_t glyphPointsSize = sizeof(TTF_GlyphPointList) + sizeof(TTF_GlyphPoint) * (p - 1);
    TTF_GlyphPointList* glyphPoints;
    if (!(glyphPoints = allocate(glyphPointsSize, "ttf  | glyf", "glyphPoints"))) return NULL;
    glyphPoints->pointAmount = p;

    // Set all flags to 0
    for (uint32_t i = 0; i < p; i++)
        glyphPoints->points[i].flag = 0;

    // Fill glyphPoints flags
    p = 0;
    for (uint32_t i = 0; i < charToGlyph->characterAmount; i++) {
        // Go to character location
        uint32_t location = charToGlyph->characters[i].index;
        if (!seekFile(file, location, SEEK_SET, "ttf  | glyf", "character (2)")) return NULL;

        // Seek to contour points
        if (!seekFile(file, sizeof(int16_t) * 5, SEEK_CUR, "ttf  | glyf", "after metadata")) return NULL;

        // Place last point in contour flags
        uint16_t endPoint;
        for (uint16_t j = 0; j < charToGlyph->characters[i].contourAmount; j++) {
            if (!readU16(&endPoint, 1, file, "ttf  | glyf", "contour end point")) return NULL;
            glyphPoints->points[endPoint + p].flag = 0x80;
        }

        uint16_t pointAmount = endPoint + 1;

        // Seek to flags
        uint16_t instructionLength;
        if (!readU16(&instructionLength, 1, file, "ttf  | glyf", "instructionLength")) return NULL;
        if (!seekFile(file, sizeof(uint8_t) * instructionLength, SEEK_CUR, "ttf  | glyf", "after instructions")) return NULL;

        // Account for repeating flags
        uint8_t flagToRepeat;
        uint8_t amountToRepeat = 0;

        // Set index
        charToGlyph->characters[i].index = p;

        // Read flags
        uint32_t tempP = p;
        for (uint32_t j = 0; j < pointAmount; j++) {

            // Check for repeats
            if (amountToRepeat > 0) {
                amountToRepeat--;
                glyphPoints->points[p].flag = flagToRepeat;
                p++;
                continue;
            }

            // Read flag
            uint8_t flag;
            if (!readU8(&flag, 1, file, "ttf  | glyf", "point flag")) return NULL;

            // Repeat if needed
            if (flag & 0x08) {
                flagToRepeat = flag;
                if (!readU8(&amountToRepeat, 1, file, "ttf  | glyf", "repeat amount")) return NULL;
            }

            // actually write the flag
            glyphPoints->points[p].flag |= flag;

            p++;
        }

        // Read x
        p = tempP;
        int16_t prevCoord = 0;
        int16_t delta = 0;
        for (uint32_t j = 0; j < pointAmount; j++) {
            // Depends on flag
            uint8_t flagX = glyphPoints->points[p].flag & (GLYF_X_SHORT | GLYF_X_SAME_SHORT_POS);

            // Switch depending on flag
            uint8_t basicDelta;
            switch (flagX) {
                case 0x00:
                    if (!read16(&delta, 1, file, "ttf  | glyf", "x delta (16 bit)")) return NULL;
                    break;
                case GLYF_X_SHORT:
                    if (!readU8(&basicDelta, 1, file, "ttf  | glyf", "x delta (8 bit -)")) return NULL;
                    delta = -basicDelta;
                    break;
                case GLYF_X_SAME_SHORT_POS:
                    delta = 0;
                    break;
                case GLYF_X_SHORT | GLYF_X_SAME_SHORT_POS:
                    if (!readU8(&basicDelta, 1, file, "ttf  | glyf", "x delta (8 bit +)")) return NULL;
                    delta = basicDelta;
                    break;
            }

            // Set x
            prevCoord += delta;
            glyphPoints->points[p].x = prevCoord;

            p++;
        }

        // Read y (same as x with minimal changes)
        p = tempP;
        prevCoord = 0;
        for (uint32_t j = 0; j < pointAmount; j++) {
            // Depends on flag
            uint8_t flagY = glyphPoints->points[p].flag & (GLYF_Y_SHORT | GLYF_Y_SAME_SHORT_POS);

            // Switch depending on flag
            uint8_t basicDelta;
            switch (flagY) {
                case 0x00:
                    if (!read16(&delta, 1, file, "ttf  | glyf", "y delta (16 bit)")) return NULL;
                    break;
                case GLYF_Y_SHORT:
                    if (!readU8(&basicDelta, 1, file, "ttf  | glyf", "y delta (8 bit -)")) return NULL;
                    delta = -basicDelta;
                break;
                case GLYF_Y_SAME_SHORT_POS:
                    delta = 0;
                    break;
                case GLYF_Y_SHORT | GLYF_Y_SAME_SHORT_POS:
                    if (!readU8(&basicDelta, 1, file, "ttf  | glyf", "y delta (8 bit +)")) return NULL;
                    delta = basicDelta;
                break;
            }

            // // Set y
            prevCoord += delta;
            glyphPoints->points[p].y = prevCoord;

            p++;
        }
    }

    // TODO miss no characters printf("Missed characters: %i\n", m);
    // Allocate final struct
    TTF_CharsAndGlyphs* charAndGlyph;
    if (!(charAndGlyph = allocate(sizeof(TTF_CharsAndGlyphs), "ttf  | glyf", "charAndGlyph"))) return NULL;

    // Fill final struct
    charAndGlyph->charToGlyph = charToGlyph;
    charAndGlyph->glyphPoints = glyphPoints;

    return charAndGlyph;
}

void* loadFileTTF(char* fileName, char* include) { // Error TAG: ttf
    FILE* file = fopen(fileName, "rb");

    if (file == NULL) {
        printf("ttf  | File failed loading.\n");
        return NULL;
    }

    // Reading table directory
    TTF_Tables* tables = readTableDirectory(file);
    if (tables == NULL) return NULL;

    TTF_SimplifiedCMAP* charMap = readTableCMAP(file, tables);
    if (charMap == NULL) return NULL;

    TTF_GlyphMetadata metadata;
    TTF_GlyphMetadata* glyphMetadata = readTableHEAD(file, tables, &metadata);
    if (glyphMetadata == NULL) return NULL;

    glyphMetadata = readTableMAXP(file, tables, &metadata);
    if (glyphMetadata == NULL) return NULL;

    uint32_t* indexToGlyph = readTableLOCA(file, tables, metadata);
    if (indexToGlyph == NULL) return NULL;

    TTF_CharsAndGlyphs* charAndGlyph = readTableGLYF(file, tables, charMap, indexToGlyph, include);
    if (charAndGlyph == NULL) return NULL;

    //HACK test code
    /*printf("number of tables: %i\n", tables->numTables);
    for (uint16_t i = 0; i < tables->numTables; i++) {
        printf("table %i: %.4s\n", i + 1, &tables->tables[i].tableTag);
    }*/
    // NOTE bdat and bloc not defined in specs

    // Cleanup
    fclose(file);
    free(tables);
    free(charMap);
    free(indexToGlyph);

    return charAndGlyph; // TODO return actual font data
}
