#include "txt.h"

TXT_File* loadFileTXT(char* fileName, bool appendNull) { // Error TAG: txt
    FILE* file = fopen(fileName, "rb");

    // get file length
    if (!seekFile(file, 0, SEEK_END, "txt ", "end of file")) return NULL;
    long fileLength;
    if (!tellFile(file, &fileLength, "txt ", "end of file")) return NULL;

    // allocation
    uint32_t storeSize = sizeof(TXT_File) + fileLength * sizeof(char) + (uint32_t)appendNull;
    TXT_File* txtStore = allocate(storeSize, "txt ", "text file");
    if (txtStore == NULL) return NULL;
    txtStore->length = fileLength + (uint32_t)appendNull;

    // read file
    if (!seekFile(file, 0, SEEK_SET, "txt ", "start of file")) return NULL;
    if (!readChar(txtStore->text, fileLength, file, "txt ", "text")) return NULL;

    // add NULL if needed
    if (appendNull)
        txtStore->text[fileLength] = 0x00;

    //DEBUG || DEBUG_TXT
    #if DEBUG
    printf("\ntxt\n");
    printf("%s\n", txtStore->text);
    #endif

    // return
    return txtStore;
}
