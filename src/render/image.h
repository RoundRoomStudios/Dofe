#ifndef IRON_RENDER_IMAGE

#define IRON_RENDER_IMAGE

typedef struct AtlasPackedImages {
    uint32_t size;
    uint32_t options;
    /*
    amount of sizes indexed (n&0xFF)
    amount of color indexed ((n>>8)&0xFF)
    size of x               ((n>>16)&0x1F)+1
    size of y               ((n>>21)&0x1F)+1
    channel amount          ((n>>26)&0x3)+1
    size of channel         ((n>>28)&0xF)+1
    channel:
        1-g (grayscale)
        2-ga
        3-rgb
        4-rgba
    */
    uint8_t bytes[]; // size then color, pallette then main data
} AtlasPackedImages;

typedef struct AtlasPackedPointers {
    uint32_t size;
    uint16_t options;
    /*
    size of identifier (n&0x1F)+1
    size of index      ((n>>5)&0x1F)+1
    */
    uint8_t bytes[];
} AtlasPackedPointers;

typedef struct AtlasPointer {
    uint32_t ID;
    uint32_t index;
} AtlasPointer;

typedef struct AtlasPointers {
    uint32_t pointerAmount;
    AtlasPointer pointers[];
} AtlasPointers;

typedef struct AtlasPI {
    AtlasPointer*      pointerList;
    AtlasPackedImages* packedImages;
} AtlasPI;

#endif
