#ifndef IRON_BASIC

#define IRON_BASIC

#include <stdlib.h>
#include <stdio.h>

static inline void* allocate(size_t size, const char* errorType, const char* dataName) {
    // Helper malloc with error checking
    void* x = malloc(size);
    if (x == NULL) {
        printf("%s | Failed to malloc %s.\n", errorType, dataName);
        return NULL;
    }
    return x;
}

#endif
