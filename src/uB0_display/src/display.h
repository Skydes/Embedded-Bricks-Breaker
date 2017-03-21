
#ifndef SRC_DISPLAY_H_
#define SRC_DISPLAY_H_

#include <stdio.h>
#include <stdbool.h>

#include "xtft.h"

#include "graphic_primitives.h"
#include "config.h"
#include "config_display.h"


void draw_layout(XTft *Tft);
void display_msg(XTft *Tft, Game_state state);
void draw_ball(XTft *Tft, Ball ball);
void draw_bar(XTft *Tft, u16 bar_pos);
void display_info(XTft *Tft, Model_state data);
void draw_bricks(XTft *Tft, Brick_state bricks[NB_COLUMNS][NB_ROWS]);

#endif
