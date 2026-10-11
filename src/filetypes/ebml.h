#ifndef IRON_FILETYPES_EBML

#define IRON_FILETYPES_EBML

#include <stdio.h>
#include <stdint.h>

#include "./filetypesBasic.h"
#include "../basic.h"

typedef struct Vint {
    uint64_t vint;
    uint8_t len;
} Vint;

void loadFileEBML(char* fileName);

#endif
