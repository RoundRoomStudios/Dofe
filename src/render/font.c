#include "font.h"
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

// uScale now comes from a uniform instead of a hardcoded 3000.f
const char *vertexShaderSource = "#version 330 core\n"
"layout (location = 0) in ivec2 aPos;\n"
"uniform vec2 uScale;\n"
"uniform vec2 uOffset;\n"
"void main()\n"
"{\n"
"   gl_Position = vec4((float(aPos.x) - uOffset.x) / uScale.x, (float(aPos.y) - uOffset.y) / uScale.y, 0.0, 1.0);\n"
"}\0";
const char *fragmentShaderSource = "#version 330 core\n"
"out vec4 FragColor;\n"
"void main()\n"
"{\n"
"    FragColor = vec4(1.f, 1.f, 1.f, 1.f);\n"
"}\0";


bool renderFont(TTF_CharsAndGlyphs* charAndGlyph) { // Error TAG: rend | font
    // HACK only renders one letter with no fill and no input data
    // TODO check for opengl errors after running a function instead of once at end of function

    // Variables for more clean code
    TTF_CharToGlyph* charToGlyph = charAndGlyph->charToGlyph;
    TTF_GlyphPointList* glyphPoints = charAndGlyph->glyphPoints;

    // Scan data for letter "A"
    uint16_t iOfA = 0xFFFF;
    for (uint16_t i = 0; i < charToGlyph->characterAmount; i++) {
        if (charToGlyph->characters[i].codepoint == 0x41) {
            iOfA = i;
            break;
        }
    }

    // Stop if no "A"
    if (iOfA == 0xFFFF) {
        printf("rend | font | Letter A not found\n");
    }

    // Get data about "A"
    uint32_t indexOfA = charToGlyph->characters[iOfA].index;
    uint32_t i = indexOfA;
    while (i < glyphPoints->pointAmount) {
        if (glyphPoints->points[i].flag & 0x80)
            break;
        i++;
    }
    uint32_t pointAmount = (i - indexOfA + 1);

    // Make a Vertex Buffer Object
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, pointAmount * sizeof(TTF_GlyphPoint), &glyphPoints->points[indexOfA], GL_STATIC_DRAW);

    // Maka a Vertex Array Object
    GLuint vao;
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    // Fill coordinates
    glVertexAttribIPointer(0, 2, GL_SHORT, sizeof(TTF_GlyphPoint), (void*)offsetof(TTF_GlyphPoint, x));
    glEnableVertexAttribArray(0);

    // Compile vertex shader
    GLuint vertShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertShader);

    // Compile fragment shader
    GLuint fragShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragShader);

    // Create and use program
    GLuint program = glCreateProgram();
    glAttachShader(program, vertShader);
    glAttachShader(program, fragShader);
    glLinkProgram(program);
    glUseProgram(program);

    // Scale points
    GLint location = glGetUniformLocation(program, "uScale");
    glUniform2f(location, 1000, 1000);
    location = glGetUniformLocation(program, "uOffset");
    glUniform2f(location, 200, 200);

    // Render
    glLineWidth(3);
    glDrawArrays(GL_LINE_LOOP, 0, pointAmount);

    // Cleanup
    glDeleteBuffers(1, &vbo);
    glDeleteVertexArrays(1, &vao);
    glDeleteShader(vertShader);
    glDeleteShader(fragShader);
    glDeleteProgram(program);

    // Errors
    GLenum error;
    while ((error = glGetError()) != GL_NO_ERROR) {
        printf("OpenGL error: 0x%x\n", error);
    }

    return true;
}
