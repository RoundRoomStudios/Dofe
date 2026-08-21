#ifndef IRON_FILETYPES_TTF

#define IRON_FILETYPES_TTF

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "../basic.h"

// TODO move some typedefs to ttf.c

void* loadFileTTF(char* fileName, char* include);

typedef uint32_t TTF_Tag;

typedef struct TTF_TableRecord {
    TTF_Tag  tableTag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
} TTF_TableRecord;

typedef struct TTF_Tables {
    uint16_t        numTables;
    TTF_TableRecord tables[];
} TTF_Tables;

typedef struct TTF_RecordCMAP {
    uint16_t codepoint;
    uint16_t glyphID;
} TTF_RecordCMAP;

typedef struct TTF_SimplifiedCMAP {
    uint16_t       numCodepoints;
    TTF_RecordCMAP codepoints[];
} TTF_SimplifiedCMAP;

typedef struct TTF_SegmentCMAP {
    uint16_t startCodepoint;
    uint16_t endCodepoint;
    uint16_t idRangeOffset;
    int16_t  idDelta;
} TTF_SegmentCMAP;

typedef struct TTF_SegmentsCMAP {
    TTF_SegmentCMAP segments[];
} TTF_SegmentsCMAP;

typedef struct TTF_GlyphMetadata {
    uint16_t numGlyphs;
    int16_t  indexToLocFormat;
} TTF_GlyphMetadata;

typedef struct TTF_Character {
    uint16_t codepoint;
    uint16_t contourAmount;
    uint32_t index;
} TTF_Character;

typedef struct TTF_CharToGlyph {
    uint16_t      characterAmount;
    TTF_Character characters[];
} TTF_CharToGlyph;

typedef struct TTF_GlyphPoint {
    uint8_t flag;
    int16_t x;
    int16_t y;
} TTF_GlyphPoint;

typedef struct TTF_GlyphPointList {
    uint32_t       pointAmount;
    TTF_GlyphPoint points[];
} TTF_GlyphPointList;

typedef struct TTF_CharsAndGlyphs {
    TTF_CharToGlyph*    charToGlyph;
    TTF_GlyphPointList* glyphPoints;
} TTF_CharsAndGlyphs;

#endif
