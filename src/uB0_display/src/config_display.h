
#ifndef SRC_CONFIG_DISPLAY_H_
#define SRC_CONFIG_DISPLAY_H_

#include "xtft_hw.h"

#define to6bits(c)			( ((((c >> 16) & 0xff) >> 2) << 16) | ((((c >> 8) & 0xff) >> 2) << 8) | (c >> 2) )

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
#define PURPLE				to6bits(0x9400D3)//0x0011061C
#define CORAL				to6bits(0xFF7F50)
#define SALMON				to6bits(0xFA8072)
#define PINK				to6bits(0xDC143C)
#define WHITE               0x00ffffff
#define GRAY				0x00aaaaaa
#define BLACK               0x00000000

#define BZ_OFFSET_X			60
#define BZ_OFFSET_Y			60

#define BAR_FILLED			true

#define CHAR_H				XTFT_CHAR_HEIGHT
#define CHAR_W				XTFT_CHAR_WIDTH

#define TXT_OFFSET_X		520
#define SCORE_OFFSET_Y		120
#define TXT_SCORE			"Score:"
#define SPEED_OFFSET_Y		(SCORE_OFFSET_Y+3*CHAR_H)
#define TXT_SPEED			"Ball speed:"
#define BRICKS_OFFSET_Y		(SPEED_OFFSET_Y+3*CHAR_H)
#define TXT_BRICKS			"Bricks left:"

#define MSG_BOX_HEIGHT		CHAR_H*2
#define MSG_BOX_WIDTH		100
#define MSG_TXT_LOST		"You lost"
#define MSG_TXT_WON			"You won"
#define MSG_TXT_PAUSED		"Pause"

#endif
