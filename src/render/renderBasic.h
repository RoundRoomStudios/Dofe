#ifndef IRON_RENDER_BASIC

#define IRON_RENDER_BASIC

#define INDEX_UNIF_GLBL_VIEW 0
#define INDEX_UNIF_GRID_TYPE 1
#define INDEX_SSBO_GRID_IMGE 2
#define INDEX_SSBO_GRID_INDX 3

#include <glad/gl.h>
#include <stdio.h>

static inline bool getError(const char* errorType, const char* justDid) {
    GLenum e = glGetError();
    switch (e) {
        case GL_NO_ERROR:
            return true;
        case GL_INVALID_ENUM:
            printf("%s | GL invalid enum when %s\n", errorType, justDid);
            break;
        case GL_INVALID_VALUE:
            printf("%s | GL invalid value when %s\n", errorType, justDid);
            break;
        case GL_INVALID_OPERATION:
            printf("%s | GL invalid opertaion when %s\n", errorType, justDid);
            break;
        case GL_INVALID_FRAMEBUFFER_OPERATION:
            printf("%s | GL invalid framebuffer operation when %s\n", errorType, justDid);
            break;
        case GL_OUT_OF_MEMORY:
            printf("%s | GL out of memory when %s\n", errorType, justDid);
            break;
        case GL_STACK_UNDERFLOW:
            printf("%s | GL stack underflow when %s\n", errorType, justDid);
            break;
        case GL_STACK_OVERFLOW:
            printf("%s | GL stack overflow when %s\n", errorType, justDid);
            break;
        default:
            printf("%s | Unknown GL error when %s\n", errorType, justDid);
            break;
    }
    return false;
}

static inline bool getShaderError(GLint shader, const char* errorType, const char* shaderName) {
    GLint e;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &e);
    if (!e) { char log[512]; glGetShaderInfoLog(shader, 512, NULL, log); printf("%s | %s \n%s | for %s\n", errorType, log, errorType, shaderName); }
    return e;
}

#endif
