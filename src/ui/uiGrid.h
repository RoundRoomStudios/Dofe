#ifndef IRON_UI_GRID

#define IRON_UI_GRID

#include <uchar.h>
#include <stdint.h>
#include "../basic.h"

typedef struct uiGridRect {
    uint32_t topLeft;
    uint32_t topMiddle;
    uint32_t topRight;
    uint32_t middleLeft;
    uint32_t middleMiddle;
    uint32_t middleRight;
    uint32_t bottomLeft;
    uint32_t bottomMiddle;
    uint32_t bottomRight;
} uiGridRect;

typedef struct uiGridBoxes {
    uint32_t box0000;
    uint32_t box1000;
    uint32_t box0100;
    uint32_t box1100;
    uint32_t box0010;
    uint32_t box1010;
    uint32_t box0110;
    uint32_t box1110;
    uint32_t box0001;
    uint32_t box1001;
    uint32_t box0101;
    uint32_t box1101;
    uint32_t box0011;
    uint32_t box1011;
    uint32_t box0111;
    uint32_t box1111;
} uiGridBoxes;

typedef struct uiGrid {
    uint32_t background;
    uiGridRect rect;
    uiGridBoxes boxes;
    uint32_t penX;
    uint32_t penY;
} uiGrid;

uiGrid uiGridFromText(char32_t* background, char32_t* selectedRect, char32_t* boxes);

#endif
