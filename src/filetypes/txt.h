#ifndef IRON_FILETYPES_TXT

#define IRON_FILETYPES_TXT

#include <stdio.h>
#include <stdint.h>

#include "./filetypesBasic.h"
#include "../basic.h"

typedef struct TXT_File {
    uint32_t length;
    char     text[];
} TXT_File;

TXT_File* loadFileTXT(char* fileName);

#endif
