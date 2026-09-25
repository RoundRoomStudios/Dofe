#include <stdlib.h>

int main(int argc, char* argv[]) {
    #ifdef __linux__
        system(
            "gcc main.c"
            " src/filetypes/pcf.c"
            " src/filetypes/txt.c"
            " src/render/window.c"
            " src/render/image.c"
            " glad/src/gl.c -Iglad/include -lSDL3 -lGL"
            " -o main -g"
            " && ./main"
            " ; echo $?"
        );
    #endif
}

// gcc build.c -o build
