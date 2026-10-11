#include "ebml.h"

static Vint readEBMLVint(FILE* file) { // ERROR Tag: ebml | vint
    Vint nullInt = {0, 0xFF};
    Vint dataInt = {0, 0};
    uint8_t byte;
    if (!readU8(&byte, 1, file, "ebml | vint", "start byte")) return nullInt;
    dataInt.len = 8 - __builtin_stdc_bit_width(byte);
    dataInt.vint = byte;
    dataInt.vint ^= 0xF0 >> dataInt.len;
    for (uint8_t i = 0; i < dataInt.len; i++) {
        dataInt.vint <<= 8;
        uint8_t byte;
        if (!readU8(&byte, 1, file, "ebml | vint", "start byte")) return nullInt;
        dataInt.vint |= byte;
    }
    return dataInt;
}

static void readEBMLElement(FILE* file) { // ERROR Tag: ebml | elem
    Vint id =  readEBMLVint(file);
    Vint len = readEBMLVint(file);
    readEBMLVint(file);
}

static void readHeader(FILE* file) { // ERROR Tag: ebml | head
    readEBMLElement(file);
}

void loadFileEBML(char* fileName) { // ERROR Tag: ebml
    FILE* file = fopen(fileName, "rb");

    readHeader(file);
}
