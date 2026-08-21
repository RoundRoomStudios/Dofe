#ifndef IRON_RENDER_FONT

#define IRON_RENDER_FONT

#include "../filetypes/ttf.h"
#include <glad/gl.h>
#include <SDL3/SDL.h>
#include <stddef.h>

bool renderFont(TTF_CharsAndGlyphs* charAndGlyph);

#endif
