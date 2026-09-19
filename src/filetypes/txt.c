#include "txt.h"

TXT_File* loadFileTXT(char* fileName) { // Error TAG: txt
    // TODO add NULL at EOF TODO at font.c
    FILE* file = fopen(fileName, "rb");

    // get file length
    if (!seekFile(file, 0, SEEK_END, "txt ", "end of file")) return NULL;
    long fileLength;
    if (!tellFile(file, &fileLength, "txt ", "end of file")) return NULL;

    // allocation
    uint32_t storeSize = sizeof(TXT_File) + fileLength * sizeof(char);
    TXT_File* txtStore = allocate(storeSize, "txt ", "text file");
    if (txtStore == NULL) return NULL;
    txtStore->length = fileLength;

    // read file
    if (!seekFile(file, 0, SEEK_SET, "txt ", "start of file")) return NULL;
    if (!readChar(txtStore->text, fileLength, file, "txt ", "text")) return NULL;

    //DEBUG
    //printf("%s\n", txtStore->text);

    // return
    return txtStore;
}
