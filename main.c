#include "src/src.h"

// TODO used for things I need to do
// BUG for bugs
// HACK for things I "hacked" together
// DEBUG for things that are only used when debugging

int main(int argc, char* argv[]) {
    printf("confirm printing\n");
    SDL_Init(SDL_INIT_VIDEO);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    IronWindow* window = createWindow("Test");
    if (window == NULL) return 111;

    // Load font a▩∞☕
    PCF_CharacterAtlas atlas = loadFilePCF("unifont.pcf", U"☕▩");
    RendererPCFGrid grid = setupPCFGrid(atlas, window, 2);

    // HACK quit window
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SetSwapInterval(1);
    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED) {
                running = false;
                break;
            }
        }

        renderPCFGrid(&grid, window);

        SDL_GL_SwapWindow(window->window);
    }

    // TODO cleanup everything for code simplicity
    // Cleanup
    //deleteWindow(window);
    return 0;
}
