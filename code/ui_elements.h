#ifndef UI_ELEMENTS_H
#define UI_ELEMENTS_H

#include "screen.h"


struct Ui_Button {
    int x;
    int y;
    int w;
    int h;
    char text[40];
    void (*on_click)(void *data);
};

void ui_button_render(struct Screen *screen, struct Ui_Button *btn);


#endif // UI_ELEMENTS_H
