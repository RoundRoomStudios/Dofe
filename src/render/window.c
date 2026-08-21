#include "window.h"

#define DEAFULT_WINDOW_HEIGHT 360
#define DEAFULT_WINDOW_WIDTH  640

IronWindow* createWindow(const char* title) { // Error code win  | make
    // Create window and error check
    SDL_Window* blank = SDL_CreateWindow(title, DEAFULT_WINDOW_WIDTH, DEAFULT_WINDOW_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE);
    if (blank == NULL) {
        printf("win  | make | Window failed to be created.\n");
        printf("win  | make | SDL Error:\n%s\n", SDL_GetError());
        return NULL;
    }

    // Create context and error check
    SDL_GLContext context = SDL_GL_CreateContext(blank);
    if (context == NULL) {
        printf("win  | make | Context failed to be created.\n");
        printf("win  | make | SDL Error:\n%s\n", SDL_GetError());
        return NULL;
    }

    // Allocate return value
    IronWindow* window;
    if (!(window = allocate(sizeof(IronWindow), "win  | make", "Window object"))) {
        printf("SDL Error:\n%s\n", SDL_GetError());
        return NULL;
    }

    // Load gl and store window
    window->window = blank;
    if (!gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress)) {
        printf("win  | make | Failed to initialize GLAD.\n");
        return NULL;
    }

    // Show window
    if (!SDL_GL_SwapWindow(blank)) {
        printf("win  | make | Window failed to be swapped.\n");
        printf("win  | make | SDL Error:\n%s\n", SDL_GetError());
        return NULL;
    }

    return window;
}

void deleteWindow(IronWindow* window) { //TODO delete all even if not really important
    // Destroy context

    // Destroys and frees error free objects
    SDL_DestroyWindow(window->window);
    free(window);
}
