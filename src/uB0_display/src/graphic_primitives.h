
#ifndef SRC_GRAPHIC_PRIMITIVES_H_
#define SRC_GRAPHIC_PRIMITIVES_H_

#include <stdbool.h>

#include "xtft.h"

#include "config.h"
#include "config_display.h"

void drawBox(XTft *Tft, u16 x1, u16 x2, u16 y1, u16 y2, u32 color, bool filled);
void drawHLine(XTft *Tft, int x1, int x2, u16 y, u32 color);
void drawVLine(XTft *Tft, u16 x, int y1, int y2, u32 color);
void drawCircle(XTft *Tft, u16 x0, u16 y0, u16 radius, u32 color, bool filled);
void writeText(XTft *Tft, u16 x, u16 y, char* txt, u32 color);



#endif
