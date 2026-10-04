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
    if (!wide) {printf( "imge | grid | pitr | non-wide  grid not supported\n"); return nullAtlasGrid;}
    if (!shift) {printf("imge | grid | pitr | non-shift grid not supported\n"); return nullAtlasGrid;}

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
        grid->cells[i].index = atlas->pointerList->pointers[0].index;
        grid->cells[i].shift = 0;
        grid->cells[i].partX = 0;
        grid->cells[i].partY = 0;
    }

    atlasGrid.gridType = 0b11;
    atlasGrid.grid = grid;

    #if DEBUG || DEBUG_IMGE || DEBUG_IMGE_GRID || DEBUG_IMGE_GRID_PITR
    printf("\nimge | grid | pitr\n");
    printf("grid width: %i\n", grid->width);
    printf("grid heigh: %i\n", grid->height);
    printf("cell width: %i\n", grid->cellWidth);
    printf("cell heigh: %i\n", grid->cellHeight);
    printf("grid pos x: %i\n", grid->x);
    printf("grid pos y: %i\n", grid->y);
    #endif

    return atlasGrid;
}

uint8_t gridMults[4] = {sizeof(uint32_t), sizeof(CellWide), sizeof(CellShift), sizeof(CellWideShift)};
AtlasPIGridRenderer setupAtlasPIGridRenderer(AtlasPIGrid* atlasGrid, IronWindow* window) { // Error TAG: imge | grid | setp
    // copy pointers into renderer
    AtlasPIGridRenderer renderer = {atlasGrid->pointerList, atlasGrid->packedImages, atlasGrid->gridType, atlasGrid->grid};
    AtlasPIGridRenderer nullRenderer = {NULL};

    // make vertex shader
    TXT_File* vertFile = loadFileTXT("src/shaders/grid.vert", true);
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    const char* textVert = vertFile->text;
    glShaderSource(vertShader, 1, &textVert, NULL);
    glCompileShader(vertShader);
    getShaderError(vertShader, "imge | grid | setp", "vertex shader");
    getError("imge | grid | setp", "compiled vertex shader");

    // make fragment shader
    TXT_File* fragFile = loadFileTXT("src/shaders/grid.frag", true);
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    const char* textFrag = fragFile->text;
    glShaderSource(fragShader, 1, &textFrag, NULL);
    glCompileShader(fragShader);
    getShaderError(fragShader, "imge | grid | setp", "fragment shader");
    getError("imge | grid | setp", "compiled fragment shader");

    // make program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glUseProgram(program);
    getError("imge | grid | setp", "made program");

    // reallocate bitmap data to be multiple of 4
    size_t size = atlasGrid->packedImages->size;
    if (size & 3) {size = ((size | 3) + 1);}
    void* temp = realloc(atlasGrid->packedImages, size);
    if (temp == NULL) {printf("imge | grid | setp | realloc failed\n"); return nullRenderer;}
    // setting bitmap data for consistency
    atlasGrid->packedImages = temp;
    atlasGrid->packedImages->size = size;

    // make bitmap SSBO
    GLuint bitmapSSBO;
    glCreateBuffers(1, &bitmapSSBO);
    glNamedBufferData(bitmapSSBO, size + sizeof(AtlasPackedImages), atlasGrid->packedImages, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_GRID_IMGE, bitmapSSBO);
    getError("imge | grid | setp", "made bitmap SSBO");

    // make offset SSBO
    Grid* grid = atlasGrid->grid;
    size = gridMults[atlasGrid->gridType] * grid->width * grid->height + sizeof(Grid);
    GLuint offsetSSBO;
    glCreateBuffers(1, &offsetSSBO);
    glNamedBufferData(offsetSSBO, size, grid, GL_STATIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INDEX_SSBO_GRID_INDX, offsetSSBO);
    getError("imge | grid | setp", "made offset SSBO");

    // make VAO
    GLuint VAO;
    glCreateVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    getError("imge | grid | setp", "made VAO");

    // input grid type and screen size
    glUniform1ui(INDEX_UNIF_GRID_TYPE, atlasGrid->gridType);

    // renderer data
    renderer.packedImages = atlasGrid->packedImages;
    renderer.program      = program;
    renderer.bitmapSSBO   = bitmapSSBO;
    renderer.offsetSSBO   = offsetSSBO;
    renderer.VAO          = VAO;

    // cleanup
    glDeleteShader(vertShader);
    free(vertFile);
    glDeleteShader(fragShader);
    free(fragFile);
    getError("imge | grid | setp", "deleted shaders");

    #if DEBUG || DEBUG_IMGE || DEBUG_IMGE_GRID || DEBUG_IMGE_GRID_SETP
    printf("\nimge | grid | setp\n");
    printf("pointerList:  %p\n", atlasGrid->pointerList);
    printf("packedImages: %p\n", atlasGrid->packedImages);
    printf("gridType:     %i\n", atlasGrid->gridType);
    printf("grid:         %p\n", atlasGrid->grid);
    printf("program:      %i\n", renderer.program);
    printf("bitmapSSBO:   %i\n", renderer.bitmapSSBO);
    printf("offsetSSBO:   %i\n", renderer.offsetSSBO);
    printf("VAO:          %i\n", renderer.VAO);
    for (uint16_t i = 0; i < 4; i++) {
        printf("mult %i: %i\n", i, gridMults[i]);
    }
    #endif

    return renderer;
}

bool renderAtlasPIGridRenderer(AtlasPIGridRenderer* renderer, IronWindow* window) { // Error TAG: imge | grid | rend
    int w, h;
    SDL_GetWindowSize(window->window, &w, &h);
    glUniform2i(INDEX_UNIF_GLBL_VIEW, w, h);
    glViewport(0, 0, w, h);

    Grid* grid = renderer->grid;
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, grid->width * grid->height);
    if (!getError("imge | grid | rend", "render grid")) return false;

    #if DEBUG || DEBUG_IMGE || DEBUG_IMGE_GRID || DEBUG_IMGE_GRID_REND
    printf("\nimge | grid | rend\n");
    #endif

    return true;
}
