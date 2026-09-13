#ifndef IRON_FILETYPES_PCF

#define IRON_FILETYPES_PCF

#include <stdio.h>
#include <stdint.h>
#include <uchar.h>

#include "./filetypesBasic.h"
#include "../basic.h"

#include "../render/image.h"

typedef struct PCF_Encoding {
    uint16_t codepoint;
    uint32_t index;
    uint32_t bit;
} PCF_Encoding;

typedef struct PCF_EncodingList {
    uint16_t     characterAmount;
    uint16_t     defaultCharacter;
    PCF_Encoding encodings[];
} PCF_EncodingList;

typedef struct PCF_Metric {
    int16_t leftSidedBearing;
    int16_t rightSideBearing;
    int16_t characterWidth;
    int16_t characterAscent;
    int16_t characterDescent;
    uint16_t characterAttrib;
} PCF_Metric;

typedef struct PCF_MetricList {
    PCF_Metric metrics[];
} PCF_MetricList;

typedef struct PCF_TableEntry {
    uint32_t type;
    uint32_t format;
    uint32_t size;
    uint32_t offset;
} PCF_TableEntry;

typedef struct PCF_Tables {
    uint32_t        tableCount;
    PCF_TableEntry  tables[];
} PCF_Tables;

typedef struct PCF_CharacterAtlas {
    ImageAtlasPackedImages* images;
    PCF_EncodingList* pointers;
} PCF_CharacterAtlas;

typedef struct PCF_AtlasInfo {
    uint32_t size;
    uint32_t options; //for atlas
    uint16_t sizeAmount;
    uint16_t sizes[0xFF*2];
} PCF_AtlasInfo;

PCF_CharacterAtlas loadFilePCF(char* fileName, char32_t* include);

#endif
