#ifndef IRON_GENERAL_GRID

#define IRON_GENERAL_GRID

#include <stdint.h>

typedef struct Vec2 { // TODO implement everywhere
    int32_t x;
    int32_t y;
} Vec2;

typedef struct UVec2_16 { // TODO implement everywhere
    uint16_t x;
    uint16_t y;
} UVec2_16;

typedef struct Rect2 {
    int32_t x;
    int32_t y;
    int32_t w;
    int32_t h;
} Rect2;

#endif
