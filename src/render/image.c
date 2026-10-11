#include "image.h"

bool getOffsetFromID(AtlasPI atlas, uint32_t ID, uint32_t *offset) { // assumes list is sorted (TODO at pcf.c to sort list)
    // binary search
    AtlasPointers* pointerList = atlas.pointerList;
    if (pointerList->pointerAmount == 0) return false; // no ids in list

    // initial values
    uint32_t min = 0;
    uint32_t max = pointerList->pointerAmount - 1;

    while (min <= max) {
        uint32_t mid = min + (max - min) / 2;
        uint32_t foundID = pointerList->pointers[mid].ID;

        if (foundID == ID) {
            *offset = pointerList->pointers[mid].index;
            return true;
        } else if (foundID < ID) {
            min = mid + 1;
        } else {
            max = mid - 1;
        }
    }
    return false; // no id found
}

bool getSizeFromOffset(AtlasPI atlas, uint32_t offset, Vec2* vec) { // assumes list is sorted (TODO at pcf.c to sort list)
    AtlasPackedImages* images = atlas.packedImages;
    uint32_t bitPos = offset;
    uint8_t sizes = images->options & 0xFF;
    if (sizes) {
        // indexed sizes
        uint32_t amount = __builtin_stdc_bit_width((uint8_t)(sizes - 1));
        BitReader r = {images->bytes, bitPos};
        uint8_t sizeID = 0;
        readBitsU8(&r, &sizeID, amount);

        // get read param
        uint32_t sizeX = ((images->options >> 16) & 0x1F) + 1;
        uint32_t sizeY = ((images->options >> 21) & 0x1F) + 1;
        r.bit = (sizeX + sizeY) * sizeID;

        // read atlas
        uint32_t w = 0; readBitsU32(&r, &w, sizeX);
        uint32_t h = 0; readBitsU32(&r, &h, sizeY);
        vec->x = w + 1; vec->y = h + 1;
        return true;
    } else {
        // non-indexed sizes TODO
    }
}

bool getSizeFromID(AtlasPI atlas, uint32_t ID, Vec2* vec) { // assumes list is sorted (TODO at pcf.c to sort list)
    uint32_t offset;
    if (!getOffsetFromID(atlas, ID, &offset)) return false;
    Vec2 temp;
    if (!getSizeFromOffset(atlas, offset, &temp)) return false;
    *vec = temp;
    return true;
}
