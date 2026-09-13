#ifndef IRON_RENDER_IMAGE

#define IRON_RENDER_IMAGE

typedef struct ImageAtlasPackedImages {
    uint32_t options;
    /*
    amount of sizes indexed (n&0xFF)
    amount of color indexed ((n>>8)&0xFF)
    size of x               ((n>>16)&0x1F)
    size of y               ((n>>21)&0x1F)
    channel amount          ((n>>26)&0x3)
    size of channel         ((n>>28)&0xF)
    channel:
        1-g (grayscale)
        2-ga
        3-rgb
        4-rgba
    */
    uint8_t bytes[]; // size then color, pallette then main data
} ImageAtlasPackedImages;

#endif
