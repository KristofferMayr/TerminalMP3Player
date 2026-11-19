#ifndef UI_H
#define UI_H

#include "audio/player.h"
#include "library/library.h"

void ui_init(void);
void ui_close(void);
void ui_draw(const library *lib, int selected, int playing, const char *message);

#endif
