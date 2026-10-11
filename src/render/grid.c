#include "grid.h"

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
            uint32_t tempX = 0; readBitsU32(&r, &tempX, sizeX); tempX++;
            uint32_t tempY = 0; readBitsU32(&r, &tempY, sizeY); tempY++;
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
    grid->gridSize.x = width;
    grid->gridSize.y = height;
    grid->cellSize.x = gridX * scale;
    grid->cellSize.y = gridY * scale;
    grid->pos.x = 0;
    grid->pos.y = 0;


    // fill grid
    for (uint32_t i = 0; i < grid->gridSize.x * grid->gridSize.y; i++) {
        grid->cells[i].index = atlas->pointerList->pointers[0].index;
        grid->cells[i].shift = 0xFFFFFFFF;
        grid->cells[i].part.x = 0;
        grid->cells[i].part.y = 0;
    }

    atlasGrid.gridType = 0b11;
    atlasGrid.grid = grid;

    #if DEBUG || DEBUG_IMGE || DEBUG_IMGE_GRID || DEBUG_IMGE_GRID_PITR
    printf("\nimge | grid | pitr\n");
    printf("grid width: %i\n", grid->gridSize.x);
    printf("grid heigh: %i\n", grid->gridSize.y);
    printf("cell width: %i\n", grid->cellSize.x);
    printf("cell heigh: %i\n", grid->cellSize.y);
    printf("grid pos x: %i\n", grid->pos.x);
    printf("grid pos y: %i\n", grid->pos.y);
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
    size = gridMults[atlasGrid->gridType] * grid->gridSize.x * grid->gridSize.y + sizeof(Grid);
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
    // set input size
    int w, h;
    SDL_GetWindowSize(window->window, &w, &h);
    glUniform2i(INDEX_UNIF_GLBL_VIEW, w, h);
    glViewport(0, 0, w, h);

    // set input grid
    Grid* grid = (Grid*)renderer->grid;
    size_t size = gridMults[renderer->gridType] * grid->gridSize.x * grid->gridSize.y + sizeof(Grid);
    glNamedBufferData(renderer->offsetSSBO, size, renderer->grid, GL_STATIC_DRAW);
    getError("imge | grid | setp", "made offset SSBO");

    // render
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, grid->gridSize.x * grid->gridSize.y);
    if (!getError("imge | grid | rend", "render grid")) return false;

    #if DEBUG || DEBUG_IMGE || DEBUG_IMGE_GRID || DEBUG_IMGE_GRID_REND
    printf("\nimge | grid | rend\n");
    #endif

    return true;
}

static inline bool insert(AtlasPIGridRenderer* renderer, Vec2 pos, Vec2 size, uint32_t id) { // return cell size TODO use rect for pos and size
    // switch depending on the grid type
    switch (renderer->gridType) {
        case NORMAL_GRID: // TODO
            break;
        case WIDE_GRID: // TODO
            break;
        case SHIFT_GRID: // TODO
            break;
        case WIDE_SHIFT_GRID:
            WideShiftGrid* grid = renderer->grid;
            for (uint16_t x = 0; x < size.x; x++)
                for (uint16_t y = 0; y < size.y; y++) {
                    uint32_t index = x + pos.x + (y + pos.y) * grid->gridSize.x;
                    uint32_t offset; getOffsetFromID(*(AtlasPI*)renderer, id, &offset);
                    grid->cells[index].index = offset;
                    grid->cells[index].part.x = x;
                    grid->cells[index].part.y = y;
                }
            break;
    }
}

static inline bool insertBlock(AtlasPIGridRenderer* renderer, Rect2 rect, Vec2 size, uint32_t id) {
    // switch depending on the grid type
    switch (renderer->gridType) {
        case NORMAL_GRID: // TODO
            break;
        case WIDE_GRID: // TODO
            break;
        case SHIFT_GRID: // TODO
            break;
        case WIDE_SHIFT_GRID:
            WideShiftGrid* grid = renderer->grid;
            Vec2 pen = {0, 0};
            for (pen.x = rect.x; pen.x < rect.x + rect.w; pen.x += size.x) {
                for (pen.y = rect.y; pen.y < rect.y + rect.h; pen.y += size.y) {
                    insert(renderer, pen, size, id);
                }
            }
            break;
    }
}

static inline bool insertBackground(AtlasPIGridRenderer* renderer, uint32_t background) {
    Grid* grid = (Grid*)renderer->grid; // only used for info
    Rect2 rect = {0, 0, grid->gridSize.x, grid->gridSize.y};
    UVec2_16 cellSize = grid->cellSize;
    Vec2 size; getSizeFromID(*(AtlasPI*)renderer, background, &size);
    size.x /= cellSize.x; size.y /= cellSize.y;
    insertBlock(renderer, rect, size, background);
}

static inline bool insertRect(AtlasPIGridRenderer* renderer, Rect2 rect, uiGridRect rectSpec) {
    // find size of each cell
    Grid* grid = (Grid*)renderer->grid; // only used for info
    UVec2_16 cellSize = grid->cellSize;
    Vec2 topLeft;      getSizeFromID(*(AtlasPI*)renderer, rectSpec.topLeft,      &topLeft     ); topLeft.x      /= cellSize.x; topLeft.y      /= cellSize.y; //printf("%i, %i\n", topLeft.x     , topLeft.y     );
    Vec2 topMiddle;    getSizeFromID(*(AtlasPI*)renderer, rectSpec.topMiddle,    &topMiddle   ); topMiddle.x    /= cellSize.x; topMiddle.y    /= cellSize.y; //printf("%i, %i\n", topMiddle.x   , topMiddle.y   );
    Vec2 topRight;     getSizeFromID(*(AtlasPI*)renderer, rectSpec.topRight,     &topRight    ); topRight.x     /= cellSize.x; topRight.y     /= cellSize.y; //printf("%i, %i\n", topRight.x    , topRight.y    );
    Vec2 middleLeft;   getSizeFromID(*(AtlasPI*)renderer, rectSpec.middleLeft,   &middleLeft  ); middleLeft.x   /= cellSize.x; middleLeft.y   /= cellSize.y; //printf("%i, %i\n", middleLeft.x  , middleLeft.y  );
    Vec2 middleMiddle; getSizeFromID(*(AtlasPI*)renderer, rectSpec.middleMiddle, &middleMiddle); middleMiddle.x /= cellSize.x; middleMiddle.y /= cellSize.y; //printf("%i, %i\n", middleMiddle.x, middleMiddle.y);
    Vec2 middleRight;  getSizeFromID(*(AtlasPI*)renderer, rectSpec.middleRight,  &middleRight ); middleRight.x  /= cellSize.x; middleRight.y  /= cellSize.y; //printf("%i, %i\n", middleRight.x , middleRight.y );
    Vec2 bottomLeft;   getSizeFromID(*(AtlasPI*)renderer, rectSpec.bottomLeft,   &bottomLeft  ); bottomLeft.x   /= cellSize.x; bottomLeft.y   /= cellSize.y; //printf("%i, %i\n", bottomLeft.x  , bottomLeft.y  );
    Vec2 bottomMiddle; getSizeFromID(*(AtlasPI*)renderer, rectSpec.bottomMiddle, &bottomMiddle); bottomMiddle.x /= cellSize.x; bottomMiddle.y /= cellSize.y; //printf("%i, %i\n", bottomMiddle.x, bottomMiddle.y);
    Vec2 bottomRight;  getSizeFromID(*(AtlasPI*)renderer, rectSpec.bottomRight,  &bottomRight ); bottomRight.x  /= cellSize.x; bottomRight.y  /= cellSize.y; //printf("%i, %i\n", bottomRight.x , bottomRight.y );

    // variables to see what to render TODO use vectors
    uint32_t wCorners = maxU32(topLeft.x, bottomLeft.x) + maxU32(topRight.x, bottomRight.x);
    uint32_t hCorners = maxU32(topLeft.y, topRight.y) + maxU32(bottomLeft.y, bottomRight.y);
    uint32_t wMiddle  = maxU32(middleMiddle.x, maxU32(topMiddle.x, bottomMiddle.x));
    uint32_t hMiddle  = maxU32(middleMiddle.y, maxU32(middleLeft.y, middleRight.y));
    uint32_t minW     = minU32(wCorners, minU32(wMiddle, middleMiddle.x));
    uint32_t minH     = minU32(hCorners, minU32(hMiddle, middleMiddle.y));

    // what to render
    if (minW > rect.w || minH > rect.h) return true; // no render since the bounds are too small

    bool renderCorners = wCorners <= rect.w && hCorners <= rect.h;
    bool renderSideW = false, renderSideH = false;
    if (renderCorners) { //TODO kinda weird results since it treats it like the middleMiddle is always rendered
        // account for corners
        renderSideW = wMiddle <= (rect.w - wCorners);
        renderSideH = hMiddle <= (rect.h - hCorners);
    } else {
        // dont account for them
        renderSideW = wMiddle <= rect.w;
        renderSideH = hMiddle <= rect.h;
    }

    // switch depending on what is rendered
    uint8_t v = (uint8_t)renderCorners; v <<= 1;
    v        |= (uint8_t)renderSideW;   v <<= 1;
    v        |= (uint8_t)renderSideH;
    switch (v) {
        case 0b000: // render middle
            printf("grid.c - rect middle\n");
            break;
        case 0b100: // render corners
            printf("grid.c - rect corners\n");
            break;
        case 0b010: // render sides along width
            printf("grid.c - rect width\n");
            break;
        case 0b110: // render corners and sides along width
            printf("grid.c - rect width + c\n");
            break;
        case 0b001: // render sides along height
            printf("grid.c - rect height\n");
            break;
        case 0b101: // render corners and sides along height
            printf("grid.c - rect height + c\n");
            break;
        case 0b011: // render sides but no actually middle as that makes more sense
            printf("grid.c - rect HUH\n");
            return false;
        case 0b111: // render all
            Vec2 pen = {rect.x, rect.y};
            // bottom left
            insert(renderer, pen, bottomLeft, rectSpec.bottomLeft); pen.x += bottomLeft.x;
            // bottom middle
            Rect2 stamp1 = {pen.x, pen.y, rect.w - wCorners, bottomMiddle.y};
            insertBlock(renderer, stamp1, bottomMiddle, rectSpec.bottomMiddle); pen.x += stamp1.w;
            // bottom right
            insert(renderer, pen, bottomRight, rectSpec.bottomRight); pen.x = rect.x; pen.y += bottomMiddle.y;
            // middle left
            Rect2 stamp2 = {pen.x, pen.y, middleLeft.x, rect.h - hCorners};
            insertBlock(renderer, stamp2, middleLeft, rectSpec.middleLeft); pen.x += stamp2.w;
            // middle middle
            Rect2 stamp3 = {pen.x, pen.y, rect.w - wCorners, rect.h - hCorners};
            insertBlock(renderer, stamp3, middleMiddle, rectSpec.middleMiddle); pen.x += stamp3.w;
            // middle right
            Rect2 stamp4 = {pen.x, pen.y, middleRight.x, rect.h - hCorners};
            insertBlock(renderer, stamp4, middleRight, rectSpec.middleRight); pen.x = rect.x; pen.y += stamp3.h;
            // top left
            insert(renderer, pen, topLeft, rectSpec.topLeft); pen.x += topLeft.x;
            // top middle
            Rect2 stamp5 = {pen.x, pen.y, rect.w - wCorners, topMiddle.y};
            insertBlock(renderer, stamp5, topMiddle, rectSpec.topMiddle); pen.x += stamp5.w;
            // top right
            insert(renderer, pen, topRight, rectSpec.topRight);
            break;
        default:
            printf("grid.c - rect HUHx2\n");
            return false;
    }
}

static inline uint32_t insertUIElement(AtlasPIGridRenderer* renderer, IronUI* ui, uiGrid grid, uint32_t offset) { // Error TAG: imge | grid | inE
    // TODO add relative positioning
    ElementUI* element = (ElementUI*)(ui->data + offset);
    switch (element->type) {
        case UI_PARENT_TYPE:
            break;
        case UI_BACKGROUND_TYPE:
            insertBackground(renderer, grid.background);
            break;
        case UI_RECT_TYPE:
            ElementRectUI* rect = (ElementRectUI*)element;
            insertRect(renderer, rect->rect, grid.rect);
            break;
    }

    return element->length + sizeof(ElementUI) / sizeof(uint32_t);
}

bool insertUI(AtlasPIGridRenderer* renderer, IronUI* ui, uiGrid grid) { // Error TAG: imge | grid | inUI
    uint32_t scanned = 0;
    while (scanned < ui->length) {
        scanned += insertUIElement(renderer, ui, grid, scanned);
    }
}
