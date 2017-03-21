#include "xmk.h"
#include "xtft.h"
#include "xtft_hw.h"
#include "xgpio.h"
#include "xparameters.h"
#include "xmutex.h"
#include "xmbox.h"

#include <errno.h>
#include <pthread.h>
#include <sys/init.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/timer.h> // for sleep

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "config.h"
#include "config_display.h"
#include "graphic_primitives.h"


int main (void);
void* main_prog(void *arg);
void* thread_display();
u8 lastCnt();
u8 nextCnt();

static XTft TftInstance;
static XMbox mbx_model;
XMutex mtx_hw;
pthread_t tid_disp;

/* Two different frame buffers are used here */
u32 frames_addr[NB_FRAMES] = {TFT_FRAME_ADDR1, TFT_FRAME_ADDR2};
u8 frames_cnt = 0;


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
	safe_printf("Displaying message %s of size %d\n\r", buf, size);
	drawBox(Tft, BZ_OFFSET_X+BZ_W/2-MSG_BOX_WIDTH/2, BZ_OFFSET_X+BZ_W/2+MSG_BOX_WIDTH/2,
				 BZ_OFFSET_Y+BZ_H/2-MSG_BOX_HEIGHT/2, BZ_OFFSET_Y+BZ_H/2+MSG_BOX_HEIGHT/2,
				 state == WON ? GREEN : RED, true);
	writeText(Tft, BZ_OFFSET_X+(BZ_W-size*CHAR_W)/2, BZ_OFFSET_Y+(BZ_H-CHAR_H)/2, buf, WHITE);
}

void draw_ball(XTft *Tft, Ball ball) {
	drawCircle(&TftInstance, ball.x, ball.y, BALL_RADIUS, ORANGE, true);
}

void draw_bar(XTft *Tft, u16 bar_pos) {
	drawBox(Tft, BZ_OFFSET_X+bar_pos-BAR_W/2,
				 BZ_OFFSET_X+bar_pos+BAR_W/2,
				 BZ_OFFSET_Y+BZ_H-BAR_OFFSET_Y-BAR_H/2,
				 BZ_OFFSET_Y+BZ_H-BAR_OFFSET_Y+BAR_H/2,
				 PURPLE, true);
}

void display_info(XTft *Tft, Model_state data) {
	char buf[256];
	u8 nb_bricks = 0;

	sprintf(buf, "%03u", data.score);
	writeText(Tft, TXT_OFFSET_X, SCORE_OFFSET_Y+CHAR_H, buf, BLACK);

	sprintf(buf, "%3u", data.ball.vel);
	writeText(Tft, TXT_OFFSET_X, SPEED_OFFSET_Y+CHAR_H, buf, BLACK);

	for(u8 col = 0; col < NB_COLUMNS; col++)
			for(u8 row = 0; row < NB_ROWS; row++)
				if(data.bricks[col][row] != BROKEN)
					nb_bricks += 1;

	sprintf(buf, "%2u", nb_bricks);
	writeText(Tft, TXT_OFFSET_X, BRICKS_OFFSET_Y+CHAR_H, buf, BLACK);
}


void* thread_display() {
	int Status;
	XTft_Config *TftConfigPtr;
	u32 TftDeviceId = TFT_DEVICE_ID;

	Model_state data;

	safe_printf("[INFO uB0] \t Configuring the display\r\n");

	/* Get address of the XTft_Config structure for the given device id */
	TftConfigPtr = XTft_LookupConfig(TftDeviceId);
	if (TftConfigPtr == (XTft_Config *)NULL)
		return XST_FAILURE;

	/* Initialize all the TftInstance members */
	Status = XTft_CfgInitialize(&TftInstance, TftConfigPtr, TftConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS)
		return XST_FAILURE;

	/* Set background color to white and clean each buffer */
	frames_cnt = 0;
	TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];
	for(u8 i = 0; i < NB_FRAMES; i++) {
		XTft_SetColor(&TftInstance, 0, WHITE);
		XTft_ClearScreen(&TftInstance);
		frames_cnt = nextCnt();
		TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];
	}

	safe_printf("[INFO uB0] \t Listening to the model\r\n");

	draw_layout(&TftInstance);

	/* Test msg */
	data.game_state = RUNNING;
	data.time = 46;
	data.score = 11;
	data.ball.vel = 250;
	data.ball.x = BZ_OFFSET_X+BZ_W/2;
	data.ball.y = BZ_OFFSET_Y+BZ_H-BAR_OFFSET_Y-BAR_H-BALL_RADIUS;
	data.bar_pos = BZ_W/2;
	for(u8 col = 0; col < NB_COLUMNS; col++)
		for(u8 row = 0; row < NB_ROWS; row++) {
			if( (col == 1 || col == 5) && (row > 1) && (row < 5))
				data.bricks[col][row] = GOLDEN;
			else
				data.bricks[col][row] = NORMAL;
		}

	// TODO: put all the display routines in parallel: ball, bricks, bar
	while(1) {
		//XMbox_ReadBlocking(&mbx_model, (u32*)&data, sizeof(data));

		/* Draw the bricks */
		for(u8 col = 0; col < NB_COLUMNS; col++)
			for(u8 row = 0; row < NB_ROWS; row++) {
				u32 color;
				switch(data.bricks[col][row]) { // TODO: check if new state different than the previous previous one
					case NORMAL:
						color = BLACK;
						break;
					case GOLDEN:
						color = RED;
						break;
					default:
						color = WHITE;
				}
				drawBox(&TftInstance, BZ_OFFSET_X+BRICK_OFFSET+col*(BRICK_W+BRICK_OFFSET),
									  BZ_OFFSET_X+(col+1)*(BRICK_W+BRICK_OFFSET),
									  BZ_OFFSET_Y+BRICK_OFFSET+row*(BRICK_H+BRICK_OFFSET),
									  BZ_OFFSET_Y+(row+1)*(BRICK_H+BRICK_OFFSET),
									  color, true);
			}

		draw_ball(&TftInstance, data.ball);
		draw_bar(&TftInstance, data.bar_pos);
		display_info(&TftInstance, data);

		/* Display message */
		if(data.game_state != RUNNING)
			display_msg(&TftInstance, data.game_state);

		while(1){
			safe_printf("Still alive\n\r");
			sleep(500);
		}
	}
}

u8 lastCnt() {
    return (NB_FRAMES + frames_cnt + 1) % NB_FRAMES; }

u8 nextCnt() {
    return (NB_FRAMES + frames_cnt  - 1) % NB_FRAMES; }


void* main_prog(void *arg) {
    XMutex_Config* configPtr_mutex;
    XMbox_Config* configPtr_mailbox;
    pthread_attr_t attr;
    struct sched_param sched_par;
    int ret;

    print("[INFO uB0] \t Starting configuration\r\n");
    /* Configure the HW Mutex */
    configPtr_mutex = XMutex_LookupConfig(MUTEX_DEVICE_ID);
    if (configPtr_mutex == (XMutex_Config *)NULL){
        print("[ERROR uB0]\t While configuring the Hardware Mutex\r\n");
        return XST_FAILURE;
    }
    ret = XMutex_CfgInitialize(&mtx_hw, configPtr_mutex, configPtr_mutex->BaseAddress);
    if (ret != XST_SUCCESS){
        print("[ERROR uB0]\t While initializing the Hardware Mutex\r\n");
        return XST_FAILURE;
    }

    /* Configure the mailbox */
    configPtr_mailbox = XMbox_LookupConfig(MBOX_DEVICE_ID);
    if (configPtr_mailbox == (XMbox_Config *)NULL) {
    	print("[ERROR uB0]\t While configuring the Mailbox\r\n");
        return XST_FAILURE;
    }
    ret = XMbox_CfgInitialize(&mbx_model, configPtr_mailbox, configPtr_mailbox->BaseAddress);
    if (ret != XST_SUCCESS) {
    	print("[ERROR uB0]\t While initializing the Mailbox\r\n");
        return XST_FAILURE;
    }

    /* Display management thread, priority 1 */
    pthread_attr_init (&attr);
    sched_par.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &sched_par);
    ret = pthread_create (&tid_disp, &attr, (void*)thread_display, NULL);
    if (ret != 0)
        xil_printf("[ERROR uB0]\t (%d) launching thread_display\r\n", ret);
    else
        xil_printf("[INFO uB0] \t Thread_display launched with ID %d \r\n",tid_disp);

    return 0;
}

int main (void) {
    print("[INFO uB0] \t Entering main()\r\n");
    xilkernel_init();
    xmk_add_static_thread(main_prog,0);
    xilkernel_start();
    xilkernel_main ();

    //Control does not reach here
}
