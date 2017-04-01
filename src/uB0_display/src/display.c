#include "display.h"

static bool erase = false;

void set_erase() { erase = true; }
void set_draw() { erase = false; }

void draw_layout(XTft *Tft) {
	char txt_score[] = TXT_SCORE;
	char txt_speed[] = TXT_SPEED;
	char txt_bricks[] = TXT_BRICKS;

	drawBox(Tft, BZ_OFFSET_X, BZ_OFFSET_X+BZ_W,
				 BZ_OFFSET_Y, BZ_OFFSET_Y+BZ_H, BLACK, false);

	writeText(Tft, TXT_OFFSET_X, SCORE_OFFSET_Y, txt_score, BLACK);
	writeText(Tft, TXT_OFFSET_X, SPEED_OFFSET_Y, txt_speed, BLACK);
	writeText(Tft, TXT_OFFSET_X, BRICKS_OFFSET_Y, txt_bricks, BLACK);
}

void display_msg(XTft *Tft, Game_state state) {
	char buf_won[] = MSG_TXT_WON;
	char buf_lost[] = MSG_TXT_LOST;
	char* buf = state == WON ? buf_won : buf_lost;
	u8 size = 0;
	while(buf[size++ +1] != '\0');
	drawBox(Tft, BZ_OFFSET_X+BZ_W/2-MSG_BOX_WIDTH/2, BZ_OFFSET_X+BZ_W/2+MSG_BOX_WIDTH/2,
				 BZ_OFFSET_Y+BZ_H/2-MSG_BOX_HEIGHT/2, BZ_OFFSET_Y+BZ_H/2+MSG_BOX_HEIGHT/2,
				 state == WON ? GREEN : RED, true);
	writeText(Tft, BZ_OFFSET_X+(BZ_W-size*CHAR_W)/2, BZ_OFFSET_Y+(BZ_H-CHAR_H)/2, buf, WHITE);
}

void draw_ball(XTft *Tft, u16 posx, u16 posy) {
	u32 color = erase ? WHITE : ORANGE;
	drawCircle(Tft, BZ_OFFSET_X+posx, BZ_OFFSET_Y+posy, BALL_R, color, true);
}

void draw_bar(XTft *Tft, u16 bar_pos) {
	u32 color = erase ? WHITE : GREEN;
	drawBox(Tft, BZ_OFFSET_X+bar_pos-BAR_W/2,
				 BZ_OFFSET_X+bar_pos+BAR_W/2,
				 BZ_OFFSET_Y+BZ_H-BAR_OFFSET_Y-BAR_H/2,
				 BZ_OFFSET_Y+BZ_H-BAR_OFFSET_Y+BAR_H/2,
				 color, true);
}

void draw_bricks(XTft *Tft, Brick bricks[NB_COLUMNS][NB_ROWS]) {
	for(u8 col = 0; col < NB_COLUMNS; col++)
		for(u8 row = 0; row < NB_ROWS; row++) {
			u32 color;
			switch(bricks[col][row]) { // TODO: check if new state different than the previous previous one
				case NORMAL:
					color = BLACK;
					break;
				case GOLDEN:
					color = RED;
					break;
				default:
					color = WHITE;
			}
			drawBox(Tft, BZ_OFFSET_X+BRICK_OFFSET+col*(BRICK_W+BRICK_OFFSET),
						 BZ_OFFSET_X+(col+1)*(BRICK_W+BRICK_OFFSET),
						 BZ_OFFSET_Y+BRICK_OFFSET+row*(BRICK_H+BRICK_OFFSET),
						 BZ_OFFSET_Y+(row+1)*(BRICK_H+BRICK_OFFSET),
						 color, true);
		}
}

void display_info(XTft *Tft, Model_state data) {
	char buf[256];
	u8 nb_bricks = 0;
	u32 color = erase ? WHITE : BLACK;

	sprintf(buf, "%03u", data.score); // TODO: replace with a lighter routine
	writeText(Tft, TXT_OFFSET_X, SCORE_OFFSET_Y+CHAR_H, buf, color);

	sprintf(buf, "%3u", data.ball_vel);
	writeText(Tft, TXT_OFFSET_X, SPEED_OFFSET_Y+CHAR_H, buf, color);

	for(u8 col = 0; col < NB_COLUMNS; col++)
			for(u8 row = 0; row < NB_ROWS; row++)
				if(data.bricks[col][row] != BROKEN)
					nb_bricks += 1;

	sprintf(buf, "%2u", nb_bricks);
	writeText(Tft, TXT_OFFSET_X, BRICKS_OFFSET_Y+CHAR_H, buf, color);
}
