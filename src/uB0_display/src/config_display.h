
#ifndef SRC_CONFIG_DISPLAY_H_
#define SRC_CONFIG_DISPLAY_H_

#include "xtft_hw.h"

#define TFT_DEVICE_ID       XPAR_TFT_0_DEVICE_ID
#define DDR_HIGH_ADDR       XPAR_PS7_DDR_0_S_AXI_HIGHADDR


#define DISP_W				XTFT_DISPLAY_WIDTH
#define DISP_H				XTFT_DISPLAY_HEIGHT
#define MAX_X               (DISP_WIDTH - 1)
#define MAX_Y               (DISP_HEIGHT - 1)

#define NB_FRAMES           2
#define TFT_FRAME_ADDR1     0x10000000
#define TFT_FRAME_ADDR2     0x10200000

#define RED					0x00ff0000
#define GREEN				0x00002000
#define BLUE                0x000000ff
#define YELLOW				0x00ffff00
#define ORANGE				0x00ffa500
#define PURPLE				0x00800080
#define WHITE               0x00ffffff
#define GRAY				0x00aaaaaa
#define BLACK               0x00000000

#define BZ_OFFSET_X			60
#define BZ_OFFSET_Y			60

#define BAR_FILLED			false

#define CHAR_H				XTFT_CHAR_HEIGHT
#define CHAR_W				XTFT_CHAR_WIDTH

#define TXT_OFFSET_X		520
#define SCORE_OFFSET_Y		120
#define TXT_SCORE			"Score:"
#define SPEED_OFFSET_Y		(SCORE_OFFSET_Y+3*CHAR_H)
#define TXT_SPEED			"Ball speed:"
#define BRICKS_OFFSET_Y		(SPEED_OFFSET_Y+3*CHAR_H)
#define TXT_BRICKS			"Bricks:"

#define MSG_BOX_HEIGHT		100
#define MSG_BOX_WIDTH		200
#define MSG_TXT_LOST		"You lost..."
#define MSG_TXT_WON			"You won!!!"

#endif
