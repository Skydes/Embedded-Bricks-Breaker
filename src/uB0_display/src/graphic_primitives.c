#include "graphic_primitives.h"

/* Faster and lighter setPixel function, avoid call stack */
#define setPixel(Tft, x, y, c)  { if(((x) >= 0) && ((y) >= 0) && ((x) < XTFT_DISPLAY_WIDTH) && ((y) < XTFT_DISPLAY_HEIGHT)) \
								       Xil_Out32((Tft)->TftConfig.VideoMemBaseAddr + (4 * ((y) * XTFT_DISPLAY_BUFFER_WIDTH + (x))), c); }



void drawBox(XTft *Tft, u16 x1, u16 x2, u16 y1, u16 y2, u32 color, bool filled) {
	if(!filled){
		drawHLine(Tft, x1, x2, y1, color);
		drawHLine(Tft, x1, x2, y2, color);
		drawVLine(Tft, x1, y1, y2, color);
		drawVLine(Tft, x2, y1, y2, color);
		return;
	}

	for(u16 x = x1; x <= x2; x++)
		for(u16 y = y1; y <= y2; y++)
			setPixel(Tft, x, y, color);
}

void drawHLine(XTft *Tft, int x1, int x2, u16 y, u32 color) {
    int start, end;
    if(x1 > x2) {
        start = x2;
        end = x1;
    }
    else {
        start = x1;
        end = x2;
    }
    for(int i = 0; i <= (end-start); i++){
        setPixel(Tft, start+i, y, color);
    }
}

void drawVLine(XTft *Tft, u16 x, int y1, int y2, u32 color) {
    int start, end;
    if(y1 > y2) {
        start = y2;
        end = y1;
    }
    else {
        start = y1;
        end = y2;
    }
    for(int i = 0; i <= (end-start); i++){
        setPixel(Tft, x, start+i, color);
    }
}

/* Midpoint circle algorithm for filled or empty circle */
void drawCircle(XTft *Tft, u16 x0, u16 y0, u16 radius, u32 color, bool filled) {
    int x = radius, y = 0, err = 0;

    while (x >= y) {
        if(filled) {
            drawHLine(Tft, x0 - x, x0 + x, y0 + y, color);
            drawHLine(Tft, x0 - y, x0 + y, y0 + x, color);
            drawHLine(Tft, x0 - x, x0 + x, y0 - y, color);
            drawHLine(Tft, x0 - y, x0 + y, y0 - x, color);
        }
        else {
            setPixel(Tft, x0 + x, y0 + y, color);
            setPixel(Tft, x0 - x, y0 + y, color);
            setPixel(Tft, x0 + y, y0 + x, color);
            setPixel(Tft, x0 - y, y0 + x, color);
            setPixel(Tft, x0 - x, y0 - y, color);
            setPixel(Tft, x0 + x, y0 - y, color);
            setPixel(Tft, x0 - y, y0 - x, color);
            setPixel(Tft, x0 + y, y0 - x, color);
        }

        if (err <= 0) {
            y += 1;
            err += 2*y + 1;
        }
        if (err > 0) {
            x -= 1;
            err -= 2*x + 1;
        }
    }
}

void writeText(XTft *Tft, u16 x, u16 y, char *txt, u32 color) {
	int i = 0;
	u32 background_color = WHITE;
	/* Try to use surrounding color as background */
	if(x>0 && y>0)
		XTft_GetPixel(Tft, x-1, y-1, &background_color);

	XTft_SetColor(Tft, color, background_color);
	XTft_SetPosChar(Tft, x, y);
	while(txt[i] != '\0')
		XTft_Write(Tft,txt[i++]);
}
