#include "font.h"

// broad rendering:
// input 3 SSBOs into shader
// SSBO 1: main data chunk of bitmaps
// SSBO 2: pointers to SSBO 1 from SSBO 3, array of 65536 uint32t
// SSBO 3: codepoints, x, y for characters

// grid rendering:
// input 2 SSBOs into shader
// SSBO 1: main data chunk of bitmaps
// SSBO 2: padding, width, height, bit offsets

GLuint currentGridOffsets; //GLuint offsetSSBO;

const char* vertShaderSource =
"#version 330 core\n"
"void main() {\n"
"    vec2 positions[4] = vec2[](\n"
"       vec2(-1.0, -1.0),\n"
"       vec2(1.0, -1.0),\n"
"       vec2(1.0, 1.0),\n"
"       vec2(-1.0, 1.0)\n"
"    );\n"
"    \n"
"    gl_Position = vec4(positions[gl_VertexID], 0.0, 1.0);\n"
"}\n";

const char* fragShaderSource =
"#version 330 core\n"
"out vec4 FragColor;\n"
"void main() {\n"
"    FragColor = vec4(1.0);\n"
"}\n";

static void printGLerrors() { // HACK
    GLenum err;
    while ((err = glGetError()) != GL_NO_ERROR) {
        const char *msg;
        switch (err) {
            case GL_INVALID_ENUM:                   msg = "GL_INVALID_ENUM"; break;
            case GL_INVALID_VALUE:                  msg = "GL_INVALID_VALUE"; break;
            case GL_INVALID_OPERATION:              msg = "GL_INVALID_OPERATION"; break;
            case GL_INVALID_FRAMEBUFFER_OPERATION:  msg = "GL_INVALID_FRAMEBUFFER_OPERATION"; break;
            case GL_OUT_OF_MEMORY:                  msg = "GL_OUT_OF_MEMORY"; break;
            case GL_STACK_UNDERFLOW:                msg = "GL_STACK_UNDERFLOW"; break;
            case GL_STACK_OVERFLOW:                 msg = "GL_STACK_OVERFLOW"; break;
            default:                                msg = "UNKNOWN_ERROR"; break;
        }
        printf("error: %s\n", msg);
    }
}

RendererPCFGrid setupPCFGrid(PCF_CharacterAtlas atlas, IronWindow* window, uint16_t scale) {  // Error TAG: set  | pcf  | grid
    //for errors TODO add more error checks
    RendererPCFGrid nullRenderer = {0, atlas, NULL, 0, 0};

    // setup renderer object
    RendererPCFGrid renderer;
    renderer.atlas = atlas;

    // vert shader
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertShaderSource, NULL);
    glCompileShader(vertShader);

    // frag shader
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragShaderSource, NULL);
    glCompileShader(fragShader);

    // HACK
    GLint success;
    char infoLog[512];
    glGetShaderiv(vertShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertShader, 512, NULL, infoLog);
        fprintf(stderr, "Vertex shader compilation failed:\n%s\n", infoLog);
    }
    glGetShaderiv(fragShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragShader, 512, NULL, infoLog);
        fprintf(stderr, "Vertex shader compilation failed:\n%s\n", infoLog);
    }

    // program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    renderer.program = program;

    // delete shaders
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);

    // calculate grid dimensions
    int w, h;
    SDL_GetWindowSize(window->window, &w, &h);
    uint16_t gridWidth  = (w / scale / 16) * 2;
    uint16_t gridHeight = h / scale / 16;

    // allocate grid
    uint32_t gridSize = sizeof(PCFGrid) + gridWidth * gridHeight * sizeof(uint32_t);
    PCFGrid* grid = allocate(gridSize, "set  | pcf  | grid", "grid of characters");
    if (grid == NULL) return nullRenderer;
    grid->width = gridWidth;
    grid->height = gridHeight;
    renderer.grid = grid;

    // get grid padding
    grid->paddingX = (w - 8 * scale * gridWidth)   / 2;
    grid->paddingY = (h - 16 * scale * gridHeight) / 2;

    // HACK setup grid to index of first letter
    for (uint16_t i = 0; i < gridWidth * gridHeight; i++) {
        grid->offsets[i] = atlas.pointers->encodings[0].bit;
    }

    // allocate SSBOs
    GLuint bitmapSSBO; //SSBO1
    GLuint offsetSSBO; //SSBO2
    glCreateBuffers(1, &bitmapSSBO);
    glCreateBuffers(1, &offsetSSBO);

    // for checking current shader
    currentGridOffsets = offsetSSBO;

    // buffer data
    glNamedBufferData(bitmapSSBO, atlas.images->size, &atlas.images->options, GL_STATIC_DRAW);
    glNamedBufferData(offsetSSBO, grid->width * grid->height * sizeof(uint32_t) + sizeof(PCFGrid), grid, GL_DYNAMIC_DRAW);

    // bind buffers
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_INDEX_FONT_GRID_1, bitmapSSBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SSBO_INDEX_FONT_GRID_2, offsetSSBO);

    // make VAO
    GLuint VAO;
    glCreateVertexArrays(1, &VAO); // or glGenVertexArrays + glBindVertexArray
    glBindVertexArray(VAO);

    //HACK
    printGLerrors(); // HACK
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        fprintf(stderr, "Program link error:\n%s\n", infoLog);
    }

    // use program
    glUseProgram(program);

    // finish renderer
    renderer.bitmapSSBO = bitmapSSBO;
    renderer.offsetSSBO = offsetSSBO;
    renderer.VAO = VAO;
    return renderer;
}

void renderPCFGrid(RendererPCFGrid* renderer, IronWindow* window) { // Error TAG: rend | pcf  | grid
    // TODO check if currently using this renderer
    uint32_t quadAmount = renderer->grid->width * renderer->grid->height;
    glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, quadAmount);

    printGLerrors(); // HACK
}
