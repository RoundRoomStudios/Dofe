#ifndef IRON_RENDER_WINDOW

#define IRON_RENDER_WINDOW

#include <stdlib.h>

#include <SDL3/SDL.h>
#include <glad/gl.h>

#include "../basic.h"

typedef struct IronWindow {
    SDL_Window* window;
    SDL_GLContext context;
} IronWindow;

IronWindow* createWindow(const char* title);

void deleteWindow(IronWindow* window);

#endif
