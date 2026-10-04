#ifndef IRON_RENDER_IMAGE

#define IRON_RENDER_IMAGE

#include <glad/gl.h>
#include <SDL3/SDL.h>

#include "window.h"
#include "./renderBasic.h"
#include "../basic.h"
#include "../filetypes/txt.h"

typedef struct AtlasPackedImages {
    uint32_t size; // size of bytes only
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
    AtlasPointers*     pointerList;
    AtlasPackedImages* packedImages;
} AtlasPI;

typedef struct Grid {
    uint16_t width;
    uint16_t height;
    uint16_t cellWidth;
    uint16_t cellHeight;
    uint16_t x;
    uint16_t y;
    uint32_t cells[];
} Grid;

typedef struct CellWide {
    uint32_t index;
    uint16_t partX;
    uint16_t partY;
} CellWide;

typedef struct WideGrid {
    uint16_t width;
    uint16_t height;
    uint16_t cellWidth;
    uint16_t cellHeight;
    uint16_t x;
    uint16_t y;
    CellWide cells[];
} WideGrid;

typedef struct CellShift {
    uint32_t index;
    uint32_t shift;
} CellShift;

typedef struct ShiftGrid {
    uint16_t width;
    uint16_t height;
    uint16_t cellWidth;
    uint16_t cellHeight;
    uint16_t x;
    uint16_t y;
    CellShift cells[];
} ShiftGrid;

typedef struct CellWideShift {
    uint32_t index;
    uint32_t shift;
    uint16_t partX;
    uint16_t partY;
} CellWideShift;

typedef struct WideShiftGrid {
    uint16_t width;
    uint16_t height;
    uint16_t cellWidth;
    uint16_t cellHeight;
    uint16_t x;
    uint16_t y;
    CellWideShift cells[];
} WideShiftGrid;

typedef struct AtlasPIGrid {
    AtlasPointers*     pointerList;
    AtlasPackedImages* packedImages;
    uint8_t            gridType; // 0 - Grid | 1 - Wide Grid | 2 - Shift Grid | 3 - Wide Shift Grid
    void*              grid; // depends on gridtype
} AtlasPIGrid;

typedef struct AtlasPIGridRenderer {
    // copied from AtlasPIGrid for no extra allocations
    AtlasPointers*     pointerList;
    AtlasPackedImages* packedImages;
    uint8_t            gridType;
    void*              grid;

    // renderer specific
    GLuint       program;
    GLuint       bitmapSSBO;
    GLuint       offsetSSBO;
    GLuint       VAO; // unused but opengl requires it
} AtlasPIGridRenderer;

AtlasPIGrid atlasPIGridFrom(AtlasPI* atlas, IronWindow* window, bool shift, uint32_t width, uint32_t height, uint16_t scale);

AtlasPIGridRenderer setupAtlasPIGridRenderer(AtlasPIGrid* atlasGrid, IronWindow* window);

bool renderAtlasPIGridRenderer(AtlasPIGridRenderer* renderer, IronWindow* window);

#endif
