#ifndef IRON_RENDER_GRID

#define IRON_RENDER_GRID

#include <glad/gl.h>
#include <SDL3/SDL.h>

#include "window.h"
#include "./renderBasic.h"
#include "../basic.h"
#include "../general/types.h"
#include "../filetypes/txt.h"
#include "./image.h"
#include "../ui/ui.h"

typedef struct Grid {
    UVec2_16 gridSize;
    UVec2_16 cellSize;
    UVec2_16 pos;
    uint32_t cells[];
} Grid;

typedef struct CellWide {
    uint32_t index;
    UVec2_16 part;
} CellWide;

typedef struct WideGrid {
    UVec2_16 gridSize;
    UVec2_16 cellSize;
    UVec2_16 pos;
    CellWide cells[];
} WideGrid;

typedef struct CellShift {
    uint32_t index;
    uint32_t shift;
} CellShift;

typedef struct ShiftGrid {
    UVec2_16 gridSize;
    UVec2_16 cellSize;
    UVec2_16 pos;
    CellShift cells[];
} ShiftGrid;

typedef struct CellWideShift {
    uint32_t index;
    uint32_t shift;
    UVec2_16 part;
} CellWideShift;

typedef struct WideShiftGrid {
    UVec2_16 gridSize;
    UVec2_16 cellSize;
    UVec2_16 pos;
    CellWideShift cells[];
} WideShiftGrid;

typedef enum AtlasGridType { // TODO implement everywhere
    NORMAL_GRID     = 0,
    WIDE_GRID       = 1,
    SHIFT_GRID      = 2,
    WIDE_SHIFT_GRID = 3,
} AtlasGridType;

typedef struct AtlasPIGrid {
    AtlasPointers*     pointerList;
    AtlasPackedImages* packedImages;
    AtlasGridType      gridType;
    void*              grid; // depends on gridtype
} AtlasPIGrid;

typedef struct AtlasPIGridRenderer {
    // copied from AtlasPIGrid for no extra allocations
    AtlasPointers*     pointerList;
    AtlasPackedImages* packedImages;
    AtlasGridType      gridType;
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

bool insertUI(AtlasPIGridRenderer* renderer, IronUI* ui, uiGrid grid);

#endif
