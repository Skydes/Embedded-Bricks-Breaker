#include "display.h"

#define PALETTE_SIZE	6
u32 golden_palette[] = {RED,GREEN,BLUE,YELLOW,PURPLE,CORAL};
static bool erase = false;

/* A thread-safe, fast and lightweight alternative to snpritf */
void alt_snprintf(char* buf, unsigned u, unsigned size);

void set_erase() { erase = true; }
void set_draw() { erase = false; }

void draw_layout(XTft *Tft) {
	char txt_score[] = TXT_SCORE;
	char txt_speed[] = TXT_SPEED;
	char txt_bricks[] = TXT_BRICKS;
	char txt_time[] = TXT_TIME;

	drawBox(Tft, BZ_OFFSET_X, BZ_OFFSET_X+BZ_W,
				 BZ_OFFSET_Y, BZ_OFFSET_Y+BZ_H, BLACK, UNFILLED);

	writeText(Tft, TXT_OFFSET_X, SCORE_OFFSET_Y, txt_score, BLACK);
	writeText(Tft, TXT_OFFSET_X, SPEED_OFFSET_Y, txt_speed, BLACK);
	writeText(Tft, TXT_OFFSET_X, BRICKS_OFFSET_Y, txt_bricks, BLACK);
	writeText(Tft, TXT_OFFSET_X, TIME_OFFSET_Y, txt_time, BLACK);
}

void display_msg(XTft *Tft, Game_state state) {
	char buf_won[] = MSG_TXT_WON;
	char buf_lost[] = MSG_TXT_LOST;
	char buf_paused[] = MSG_TXT_PAUSED;
	char* buf;
	u32 color;
	u8 size = 0;

	switch(state) {
		case WAITING:
		case RUNNING:
			return;

		case WON:
			color = erase ? WHITE : GREEN;
			buf = buf_won;
			break;
		case LOST:
			color = erase ? WHITE : RED;
			buf = buf_lost;
			break;
		case PAUSED:
			color = erase ? WHITE : ORANGE;
			buf = buf_paused;
			break;
		default:
			return;
	}

	drawBox(Tft, BZ_OFFSET_X+BZ_W/2-MSG_BOX_WIDTH/2, BZ_OFFSET_X+BZ_W/2+MSG_BOX_WIDTH/2,
				 BZ_OFFSET_Y/2-MSG_BOX_HEIGHT/2, BZ_OFFSET_Y/2+MSG_BOX_HEIGHT/2,
				 color, UNFILLED);
	while(buf[size++ +1] != '\0');
	writeText(Tft, BZ_OFFSET_X+(BZ_W-size*CHAR_W)/2, (BZ_OFFSET_Y-CHAR_H)/2, buf, color);
}

void draw_ball(XTft *Tft, u16 posx, u16 posy) {
	u32 color = erase ? WHITE : ORANGE;
	drawCircle(Tft, BZ_OFFSET_X+posx, BZ_OFFSET_Y+posy, BALL_R, color, FILLED);
}

void draw_bar(XTft *Tft, u16 bar_pos) {
	u16 y_min = BZ_OFFSET_Y+BZ_H-BAR_OFFSET_Y-BAR_H;
	u16 y_max = BZ_OFFSET_Y+BZ_H-BAR_OFFSET_Y;

	if(erase) {
		drawBox(Tft, BZ_OFFSET_X+bar_pos-BAR_W/2,
					 BZ_OFFSET_X+bar_pos+BAR_W/2,
					 y_min, y_max, WHITE, FILLED);
		return;
	}

	u32 color_n = erase ? WHITE : BLUE;
	u32 color_s = erase ? WHITE : GREEN;
	u32 color_a = erase ? WHITE : RED;

	drawBox(Tft, BZ_OFFSET_X+bar_pos-BAR_N/2,
				 BZ_OFFSET_X+bar_pos+BAR_N/2,
				 y_min, y_max, color_n, BAR_FILLED);
	drawBox(Tft, BZ_OFFSET_X+bar_pos-BAR_N/2-BAR_S,
				 BZ_OFFSET_X+bar_pos-BAR_N/2,
				 y_min, y_max, color_s, BAR_FILLED);
	drawBox(Tft, BZ_OFFSET_X+bar_pos-BAR_N/2-BAR_S-BAR_A,
				 BZ_OFFSET_X+bar_pos-BAR_N/2-BAR_S,
				 y_min, y_max, color_a, BAR_FILLED);
	drawBox(Tft, BZ_OFFSET_X+bar_pos+BAR_N/2,
				 BZ_OFFSET_X+bar_pos+BAR_N/2+BAR_S,
				 y_min, y_max, color_s, BAR_FILLED);
	drawBox(Tft, BZ_OFFSET_X+bar_pos+BAR_N/2+BAR_S,
				 BZ_OFFSET_X+bar_pos+BAR_N/2+BAR_S+BAR_A,
				 y_min, y_max, color_a, BAR_FILLED);
}

void draw_bricks(XTft *Tft, Brick bricks[NB_COLUMNS][NB_ROWS], Brick bricks_prev[NB_COLUMNS][NB_ROWS]) {
	for(u8 col = 0; col < NB_COLUMNS; col++)
		for(u8 row = 0; row < NB_ROWS; row++) {
			if(bricks[col][row] != bricks_prev[col][row])
			{
				u32 color;
				switch(bricks[col][row]) {
					case NORMAL:
						color = BLACK;
						break;
					case GOLDEN:
						color = golden_palette[((row)+(col))%PALETTE_SIZE];
						break;
					default:
						color = WHITE;
				}
				drawBox(Tft, BZ_OFFSET_X+BRICK_OFFSET+col*(BRICK_W+BRICK_OFFSET),
							 BZ_OFFSET_X+(col+1)*(BRICK_W+BRICK_OFFSET),
							 BZ_OFFSET_Y+BRICK_OFFSET+row*(BRICK_H+BRICK_OFFSET),
							 BZ_OFFSET_Y+(row+1)*(BRICK_H+BRICK_OFFSET),
							 color, FILLED);
			}
		}
}

void display_info(XTft *Tft, Model_state data) {
	char buf[256];
	u8 nb_bricks = 0;
	u32 color = erase ? WHITE : BLACK;

	alt_snprintf(buf,data.score,3);
	writeText(Tft, TXT_OFFSET_X, SCORE_OFFSET_Y+CHAR_H, buf, color);

	alt_snprintf(buf,data.ball_vel,4);
	writeText(Tft, TXT_OFFSET_X, SPEED_OFFSET_Y+CHAR_H, buf, color);

	for(u8 col = 0; col < NB_COLUMNS; col++)
		for(u8 row = 0; row < NB_ROWS; row++)
			if(data.bricks[col][row] != BROKEN)
				nb_bricks += 1;

	alt_snprintf(buf,nb_bricks,3);
	writeText(Tft, TXT_OFFSET_X, BRICKS_OFFSET_Y+CHAR_H, buf, color);

	alt_snprintf(buf,data.time,4);
	writeText(Tft, TXT_OFFSET_X, TIME_OFFSET_Y+CHAR_H, buf, color);
}

void display_fps(XTft *Tft, u16 fps) {
	char* name = "FPS: ";
	char buf[8];
	alt_snprintf(buf,fps,3);
	writeText(Tft,10,10,name, BLACK);
	writeText(Tft, 10+5*CHAR_W, 10, buf, BLACK);
}

void alt_snprintf(char* buf, unsigned u, unsigned size) {
	u8 i;
	unsigned b = u;
	for(i = 0; i < size; i++) {
		buf[size-i-1] = (char)((b % 10) + (int)'0');
		b /= 10;
	}
	buf[size] = '\0';
}
