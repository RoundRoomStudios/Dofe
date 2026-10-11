#include "ui.h"

IronUI* makeUI() { // Error TAG: FEui | make
    // make the ui
    IronUI* ui = allocate(sizeof(IronUI), "FEui | make", "ui");
    if (ui == NULL) return NULL;

    // include the length variable in itself
    ui->length = sizeof(IronUI) / sizeof(uint32_t);

    return ui;
}

uint32_t scanForElement(UIid id) {

}

bool moveData(uint32_t pos, uint32_t amt) { // use memmove

}

UIid currentID = 1; // id of 0 is an error element

UIid insertData(IronUI** uiRef, ElementUI* element, UIid parent) { // TODO add parents // Error TAG: FEui | data
    // make ui longer to account for element
    IronUI* ui = *uiRef;
    uint32_t elementLength = element->length + sizeof(ElementUI) / sizeof(uint32_t);
    ui->length += elementLength;
    *uiRef = realloc(ui, ui->length * sizeof(uint32_t));
    ui = *uiRef;

    // set id for element
    element->id = currentID;
    currentID++;

    if (parent == 0) {
        // no parent, aka append to end
        uint32_t* location = ui->data + ui->length - elementLength - sizeof(IronUI) / sizeof(uint32_t);
        memcpy(location, element, elementLength * sizeof(uint32_t));

        return element->id;
    } else {
        // parent
        printf("FEui | data | element parents are not supported yet\n");
        return 0;
    }
}

UIid makeParentElement(IronUI** uiRef, UIid parent) { //TODO add parents

}

UIid makeBackgroundElement(IronUI** uiRef, UIid parent) { //TODO add parents
    // insert a null element
    ElementBackgroundUI element = {0, UI_BACKGROUND_TYPE, 0};
    return insertData(uiRef, (ElementUI*)(&element), parent);
}

UIid makeRectElement(IronUI** uiRef, UIid parent, uint32_t x, uint32_t y, uint32_t w, uint32_t h) { //TODO add parents
    // insert a rect element
    ElementRectUI element = {0, UI_RECT_TYPE, 4, x, y, w, h};
    return insertData(uiRef, (ElementUI*)(&element), parent);
}

UIid makeNullElement(IronUI** uiRef, UIid parent) { //TODO add parents
    // insert a null element
    ElementNullUI element = {0, UI_NULL_TYPE, 0};
    return insertData(uiRef, (ElementUI*)(&element), parent);
}

uint32_t printUIElement(IronUI* ui, uint32_t offset, uint32_t depth) { // DEBUG
    for (uint32_t i = 0; i < depth; i++)
        printf("    ");

    ElementUI* element = (ElementUI*)(ui->data + offset);
    switch (element->type) {
        case UI_PARENT_TYPE:
            break;
        case UI_BACKGROUND_TYPE:
            printf("%.8x: background\n", element->id);
            break;
        case UI_RECT_TYPE:
            ElementRectUI* rect = (ElementRectUI*)element;
            printf("%.8x: rect {x: %i, y: %i, w: %i, h: %i}\n", rect->id, rect->rect.x, rect->rect.y, rect->rect.w, rect->rect.h);
            break;
        case UI_NULL_TYPE:
            printf("%.8x: null\n", element->id);
            break;
    }

    return element->length + sizeof(ElementUI) / sizeof(uint32_t);
}

void printUI(IronUI* ui) { // DEBUG
    uint32_t scanned = 0;
    while (scanned < ui->length) {
        scanned += printUIElement(ui, scanned, 0);
    }
    /*for (uint32_t i = 0; i < ui->length - 1; i++) // even more DEBUG
        printf("%.8x\n", ui->data[i]);*/
}
