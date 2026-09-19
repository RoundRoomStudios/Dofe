#ifndef IRON_RENDER_FONT

#define IRON_RENDER_FONT

#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <stddef.h>
#include "./renderBasic.h"

#include "./window.h"

#include "../filetypes/pcf.h"
#include "../filetypes/txt.h"

typedef struct PCFGrid {
    uint16_t width;
    uint16_t height;
    uint16_t paddingX;
    uint16_t paddingY;
    uint32_t offsets[];
} PCFGrid;

typedef struct RendererPCFGrid {
    GLuint program;
    PCF_CharacterAtlas atlas;
    PCFGrid* grid;
    GLuint bitmapSSBO;
    GLuint offsetSSBO;
    GLuint VAO; // unused but opengl requires it
} RendererPCFGrid;

RendererPCFGrid setupPCFGrid(PCF_CharacterAtlas atlas, IronWindow* window, uint16_t scale);

void setCharacters(RendererPCFGrid* renderer, char32_t* characters, int32_t length);

void renderPCFGrid(RendererPCFGrid* grid, IronWindow* window);

#endif
