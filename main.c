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
    PCF_CharacterAtlas atlas = loadFilePCF("unifont.pcf", NULL);
    RendererPCFGrid renderer = setupPCFGrid(atlas, window, 2);

    setCharacters(&renderer, U"Hello, World! ☕\n¡Hola Mundo! ☔\n안녕하세요, 세상! ☄\n你好世界. ☙\nこんにちは世界! ☃\nlorem ipsum.", -1);
    char32_t string[440] = U""; // 11x40
    int stringLen = 0;

    // HACK quit window
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    SDL_GL_SetSwapInterval(1);

    SDL_StartTextInput(window->window);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            switch (event.type) {
                case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                    running = false;
                    break;
                case SDL_EVENT_KEY_DOWN:
                    if (event.key.key == SDLK_BACKSPACE && stringLen > 0) {
                        // backspace
                        stringLen--;
                        string[stringLen] = ' ';
                        setCharacters(&renderer, string, -1);
                    } else if (event.key.key == SDLK_RETURN) {
                        // enter
                        if (stringLen >= 440) break;
                        string[stringLen] = '\n';
                        stringLen++;
                        setCharacters(&renderer, string, -1);
                    } else if (event.key.key == SDLK_TAB) {
                        // tab
                        for (uint8_t i = 0; i < 4; i++)
                            string[stringLen + i] = ' ';
                        stringLen += 4;
                        setCharacters(&renderer, string, -1);
                    }
                    break;
                case SDL_EVENT_TEXT_INPUT:
                    // normal characters
                    if (stringLen >= 440) break;

                    // utf-8 to unicode
                    char32_t c;
                    if (!(event.text.text[0] & 0x80)) {
                        // 1 byte
                        c = event.text.text[0];
                    } else if (!(event.text.text[0] & 0x20)) {
                        // 2 byte
                        c =  (event.text.text[0] & 0x1F) << 6;
                        c += (event.text.text[1] & 0x3F);
                    } else if (!(event.text.text[0] & 0x10)) {
                        // 3 byte
                        c =  (event.text.text[0] & 0xF ) << 12;
                        c += (event.text.text[1] & 0x3F) << 6;
                        c += (event.text.text[2] & 0x3F);
                    } else {
                        // 3 byte
                        c =  (event.text.text[0] & 0x7 ) << 18;
                        c += (event.text.text[1] & 0x3F) << 12;
                        c += (event.text.text[1] & 0x3F) << 6;
                        c += (event.text.text[2] & 0x3F);
                    }

                    // input
                    string[stringLen] = c;
                    stringLen++;
                    setCharacters(&renderer, string, -1);
                    break;
            }
        }

        renderPCFGrid(&renderer, window);

        SDL_GL_SwapWindow(window->window);
    }

    // TODO cleanup everything for code simplicity
    // Cleanup
    //deleteWindow(window);
    return 0;
}
