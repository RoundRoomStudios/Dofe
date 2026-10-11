#ifndef IRON_UI

#define IRON_UI

#include <string.h>

#include "uiGrid.h"
#include "../general/types.h"

// NOTE length only includes itself for the main IronUI data

typedef struct IronUI {
    uint32_t length;
    uint32_t data[]; // note function pointers take 2 slots
} IronUI;

typedef uint32_t UIid;  // id of 0 is an error element

typedef enum TypeUI {
    UI_PARENT_TYPE = 0x0,
    UI_BACKGROUND_TYPE = 0x100,
    UI_RECT_TYPE = 0x200,
    UI_NULL_TYPE = 0xFFFFFFFF
} TypeUI;

typedef struct ElementUI {
    UIid id;
    TypeUI type;
    uint32_t length; // every word (aka 32 bits)
    uint32_t data[];
} ElementUI;

typedef struct ElementParentUI {
    UIid id;
    TypeUI type;   // 0
    uint32_t length; // variable
    uint32_t children[];
} ElementParentUI;

typedef struct ElementBackgroundUI {  // not an error element, just blank
    UIid id;
    TypeUI type;   // 0x100
    uint32_t length; // 0
} ElementBackgroundUI;

typedef struct ElementRectUI {
    UIid id;
    TypeUI type;   // 0x200
    uint32_t length; // 4
    Rect2 rect;
} ElementRectUI;

typedef struct ElementNullUI {  // not an error element, just blank
    UIid id;
    TypeUI type;   // 0xFFFFFFFF
    uint32_t length; // 0
} ElementNullUI;

IronUI* makeUI();

uint32_t scanForElement(UIid id);

bool moveData(uint32_t pos, uint32_t amt);

UIid insertData(IronUI** uiRef, ElementUI* element, UIid parent);

UIid makeParentElement(IronUI** uiRef, UIid parent);

UIid makeBackgroundElement(IronUI** uiRef, UIid parent);

UIid makeRectElement(IronUI** uiRef, UIid parent, uint32_t x, uint32_t y, uint32_t w, uint32_t h);

UIid makeNullElement(IronUI** uiRef, UIid parent);

void printUI(IronUI* ui); // DEBUG

#endif
