#include "uiGrid.h"

uiGrid uiGridFromText(char32_t* background, char32_t* selectedRect, char32_t* boxes) { // Error TAG: uiTY | grid | text
    // for errors
    uiGrid nullGrid = {0};

    // make grid
    uiGrid grid = {0};
    uiGridRect rect = {0};
    uiGridBoxes box = {0};

    // get lengths for text
    uint32_t lenD = char32Length(background);
    uint32_t lenR = char32Length(selectedRect);
    uint32_t lenB = char32Length(boxes);
    if (lenD == 0) return nullGrid;
    if (lenR == 0) selectedRect = background;
    if (lenB == 0) boxes = background;

    // set background
    grid.background = background[0];

    // switch depending on lengths for rectangle
    switch (lenR) { // all other lengths skip TODO make code readable
        case 1: // 0 is all
            rect.topLeft      = selectedRect[0];
            rect.topMiddle    = selectedRect[0];
            rect.topRight     = selectedRect[0];
            rect.middleLeft   = selectedRect[0];
            rect.middleMiddle = selectedRect[0];
            rect.middleRight  = selectedRect[0];
            rect.bottomLeft   = selectedRect[0];
            rect.bottomMiddle = selectedRect[0];
            rect.bottomRight  = selectedRect[0];
            break;
        case 2: // 0 is middle 1 is rest
            rect.topLeft      = selectedRect[1];
            rect.topMiddle    = selectedRect[1];
            rect.topRight     = selectedRect[1];
            rect.middleLeft   = selectedRect[1];
            rect.middleMiddle = selectedRect[0];
            rect.middleRight  = selectedRect[1];
            rect.bottomLeft   = selectedRect[1];
            rect.bottomMiddle = selectedRect[1];
            rect.bottomRight  = selectedRect[1];
            break;
        case 3: // 0 is middle 1 is bottom left 2 is top right
            rect.topLeft      = selectedRect[0];
            rect.topMiddle    = selectedRect[2];
            rect.topRight     = selectedRect[2];
            rect.middleLeft   = selectedRect[1];
            rect.middleMiddle = selectedRect[0];
            rect.middleRight  = selectedRect[2];
            rect.bottomLeft   = selectedRect[1];
            rect.bottomMiddle = selectedRect[1];
            rect.bottomRight  = selectedRect[0];
            break;
        case 4: // 0 is top 1 is right 2 is bottom 3 is left
            rect.topMiddle    = selectedRect[0];
            rect.middleLeft   = selectedRect[3];
            rect.middleRight  = selectedRect[1];
            rect.bottomMiddle = selectedRect[2];
            break;
        case 5: // 0 is middle 1 is top 2 is right 3 is bottom 4 is left
            rect.topLeft      = selectedRect[0];
            rect.topMiddle    = selectedRect[1];
            rect.topRight     = selectedRect[0];
            rect.middleLeft   = selectedRect[4];
            rect.middleMiddle = selectedRect[0];
            rect.middleRight  = selectedRect[2];
            rect.bottomLeft   = selectedRect[0];
            rect.bottomMiddle = selectedRect[3];
            rect.bottomRight  = selectedRect[0];
            break;
        case 6: // 0 is top 1 is right 2 is bottom 3 is left 4 is top corners 5 is bottom corners
            rect.topLeft      = selectedRect[4];
            rect.topMiddle    = selectedRect[0];
            rect.topRight     = selectedRect[4];
            rect.middleLeft   = selectedRect[3];
            rect.middleRight  = selectedRect[1];
            rect.bottomLeft   = selectedRect[5];
            rect.bottomMiddle = selectedRect[2];
            rect.bottomRight  = selectedRect[5];
            break;
        case 7: // 0 is middle 1 is top 2 is right 3 is bottom 4 is left 5 is top corners 6 is bottom corners
            rect.topLeft      = selectedRect[5];
            rect.topMiddle    = selectedRect[1];
            rect.topRight     = selectedRect[5];
            rect.middleLeft   = selectedRect[4];
            rect.middleMiddle = selectedRect[0];
            rect.middleRight  = selectedRect[2];
            rect.bottomLeft   = selectedRect[6];
            rect.bottomMiddle = selectedRect[3];
            rect.bottomRight  = selectedRect[6];
            break;
        case 8: // 0-7 is everything but middle
            rect.topLeft      = selectedRect[0];
            rect.topMiddle    = selectedRect[1];
            rect.topRight     = selectedRect[2];
            rect.middleLeft   = selectedRect[3];
            rect.middleRight  = selectedRect[4];
            rect.bottomLeft   = selectedRect[5];
            rect.bottomMiddle = selectedRect[6];
            rect.bottomRight  = selectedRect[7];
            break;
        default: //backup
        case 9: // 0-8 is everything
            rect.topLeft      = selectedRect[0];
            rect.topMiddle    = selectedRect[1];
            rect.topRight     = selectedRect[2];
            rect.middleLeft   = selectedRect[3];
            rect.middleMiddle = selectedRect[4];
            rect.middleRight  = selectedRect[5];
            rect.bottomLeft   = selectedRect[6];
            rect.bottomMiddle = selectedRect[7];
            rect.bottomRight  = selectedRect[8];
            break;
    }

    // switch depending on lengths for boxes
    switch (lenB) { // all other lengths skip TODO make code readable TODO add all cases
        case 0x1:
            box.box1000 = boxes[0x0];
            box.box0100 = boxes[0x0];
            box.box1100 = boxes[0x0];
            box.box0010 = boxes[0x0];
            box.box1010 = boxes[0x0];
            box.box0110 = boxes[0x0];
            box.box1110 = boxes[0x0];
            box.box0001 = boxes[0x0];
            box.box1001 = boxes[0x0];
            box.box0101 = boxes[0x0];
            box.box1101 = boxes[0x0];
            box.box0011 = boxes[0x0];
            box.box1011 = boxes[0x0];
            box.box0111 = boxes[0x0];
            box.box1111 = boxes[0x0];
            break;
        case 0xF:
            box.box1000 = boxes[0x0];
            box.box0100 = boxes[0x1];
            box.box1100 = boxes[0x2];
            box.box0010 = boxes[0x3];
            box.box1010 = boxes[0x4];
            box.box0110 = boxes[0x5];
            box.box1110 = boxes[0x6];
            box.box0001 = boxes[0x7];
            box.box1001 = boxes[0x8];
            box.box0101 = boxes[0x9];
            box.box1101 = boxes[0xA];
            box.box0011 = boxes[0xB];
            box.box1011 = boxes[0xC];
            box.box0111 = boxes[0xD];
            box.box1111 = boxes[0xE];
            break;
        default: //backup
        case 0x10:
            box.box0000 = boxes[0x0];
            box.box1000 = boxes[0x1];
            box.box0100 = boxes[0x2];
            box.box1100 = boxes[0x3];
            box.box0010 = boxes[0x4];
            box.box1010 = boxes[0x5];
            box.box0110 = boxes[0x6];
            box.box1110 = boxes[0x7];
            box.box0001 = boxes[0x8];
            box.box1001 = boxes[0x9];
            box.box0101 = boxes[0xA];
            box.box1101 = boxes[0xB];
            box.box0011 = boxes[0xC];
            box.box1011 = boxes[0xD];
            box.box0111 = boxes[0xE];
            box.box1111 = boxes[0xF];
            break;
    }

    // add elements to grid
    grid.rect = rect;
    grid.boxes = box;

    return grid;
}
