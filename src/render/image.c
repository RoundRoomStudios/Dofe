#include "image.h"

AtlasPIGrid atlasPIGridFrom(AtlasPI* atlas, IronWindow* window, bool shift, uint32_t width, uint32_t height, uint16_t scale) { // Error TAG: imge | grid | pitr
    // for rror check
    AtlasPIGrid nullAtlasGrid = {NULL};

    // setup
    AtlasPIGrid atlasGrid = {atlas->pointerList, atlas->packedImages};
    AtlasPointer* pointers = atlas->pointerList->pointers;

    // get size type
    uint32_t options = atlas->packedImages->options;
    uint8_t sizeAmount = options & 0xFF;
    uint8_t sizeX = ((options >> 16) & 0x1F) + 1;
    uint8_t sizeY = ((options >> 21) & 0x1F) + 1;

    // get grid size
    uint32_t gridX;
    uint32_t gridY;
    bool wide = false;
    if (sizeAmount) {
        // indexed size
        BitReader r = {atlas->packedImages->bytes, 0};
        readBitsU32(&r, &gridX, sizeX); gridX++;
        readBitsU32(&r, &gridY, sizeY); gridY++;

        for (uint16_t i = 0; i < sizeAmount - 1; i++) {
            // scan rest
            uint32_t tempX; readBitsU32(&r, &tempX, sizeX); tempX++;
            uint32_t tempY; readBitsU32(&r, &tempY, sizeY); tempY++;
            if (tempX != gridX || tempY != gridY) wide = true;
            gridX = gcdU32(gridX, tempX);
            gridY = gcdU32(gridY, tempY);
        }
    } else {
        // non-indexed size
        printf("imge | grid | pitr | non-indexed size not implemented for grids\n");
        return nullAtlasGrid;
    }

    // error checks TODO allow wide and (color) shift grids
    if (!wide) {printf( "imge | grid | pitr | non-wide  not supported\n"); return nullAtlasGrid;}
    if (!shift) {printf("imge | grid | pitr | non-shift not supported\n"); return nullAtlasGrid;}

    // get grid width, height if not set
    if (width == 0 || height == 0) {
        int screenW, screenH; SDL_GetWindowSize(window->window, &screenW, &screenH);
        width  = screenW / gridX / scale;
        height = screenH / gridY / scale;
    }

    // allocate grid
    uint32_t gridSize = sizeof(WideShiftGrid) + width * height * sizeof(CellWideShift);
    WideShiftGrid* grid = allocate(gridSize, "imge | grid | pitr", "grid");
    if (grid == NULL) return nullAtlasGrid;

    // set grid properties
    grid->cellWidth =  gridX * scale;
    grid->cellHeight = gridY * scale;
    grid->width  = width;
    grid->height = height;
    grid->x = 0;
    grid->y = 0;

    // fill grid
    for (uint32_t i = 0; i < grid->width * grid->height; i++) {
        grid->cells[i].index = 0;
        grid->cells[i].shift = 0;
        grid->cells[i].partX = 0;
        grid->cells[i].partY = 0;
    }

    atlasGrid.gridType = 0b11;
    atlasGrid.grid = grid;

    #if DEBUG || DEBUG_IMGE || DEBUG_IMGE_GRID || DEBUG_IMGE_GRID_PITR
    printf("\nimge | from\n");
    printf("grid width: %i\n", grid->width);
    printf("grid heigh: %i\n", grid->height);
    printf("cell width: %i\n", grid->cellWidth);
    printf("cell heigh: %i\n", grid->cellHeight);
    printf("grid pos x: %i\n", grid->x);
    printf("grid pos y: %i\n", grid->y);
    #endif

    return atlasGrid;
}

AtlasPIGridRenderer setupAtlasPIGridRenderer(AtlasPIGrid* atlasGrid, IronWindow* window) { // Error TAG: imge | grid | setp
    // TODO TODO TODO
}
