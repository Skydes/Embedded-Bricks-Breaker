/*******************************************************************************
 *
 * Title:       EE4214 Bricks-Breaker Project
 * File:        display.h
 * Date:        2017-04-13
 * Author:      Paul-Edouard Sarlin (A0153124U)
 *
 ******************************************************************************/

#ifndef SRC_DISPLAY_H_
#define SRC_DISPLAY_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "xtft.h"

#include "graphic_primitives.h"
#include "config.h"
#include "config_display.h"

void set_erase();
void set_draw();

void draw_layout(XTft *Tft);
void display_msg(XTft *Tft, Game_state state);
void draw_ball(XTft *Tft, u16 posx, u16 posy);
void draw_bar(XTft *Tft, u16 bar_pos);
void display_info(XTft *Tft, Model_state data);
void display_fps(XTft *Tft, u16 fps);
void draw_bricks(XTft *Tft, Brick bricks[NB_COLUMNS][NB_ROWS],
                            Brick bricks_prev[NB_COLUMNS][NB_ROWS]);

#endif
