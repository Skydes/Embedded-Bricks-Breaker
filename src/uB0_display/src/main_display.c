#include "xmk.h"
#include "xtft.h"
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
#include "display.h"

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


void* thread_display() {
	int Status;
	XTft_Config *TftConfigPtr;
	u32 TftDeviceId = TFT_DEVICE_ID;

	Model_state data, prev_data[2];

	safe_printf("[INFO uB0] \t Configuring the display\r\n");

	/* Get address of the XTft_Config structure for the given device id */
	TftConfigPtr = XTft_LookupConfig(TftDeviceId);
	if (TftConfigPtr == (XTft_Config *)NULL)
		return (void*) XST_FAILURE;

	/* Initialize all the TftInstance members */
	Status = XTft_CfgInitialize(&TftInstance, TftConfigPtr, TftConfigPtr->BaseAddress);
	if (Status != XST_SUCCESS)
		return (void*) XST_FAILURE;

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
//	data.game_state = RUNNING;
//	data.time = 46;
//	data.score = 11;
//	data.ball.vel = 250;
//	data.ball.x = BZ_OFFSET_X+BZ_W/2;
//	data.ball.y = BZ_OFFSET_Y+BZ_H-BAR_OFFSET_Y-BAR_H-BALL_RADIUS;
//	data.bar_pos = BZ_W/2;
//	for(u8 col = 0; col < NB_COLUMNS; col++)
//		for(u8 row = 0; row < NB_ROWS; row++) {
//			if( (col == 1 || col == 5) && (row > 1) && (row < 5))
//				data.bricks[col][row] = GOLDEN;
//			else
//				data.bricks[col][row] = NORMAL;
//		}

	// TODO: put all the display routines in parallel: ball, bricks, bar
	while(1) {
		XMbox_ReadBlocking(&mbx_model, (u32*)&data, sizeof(data));
		safe_printf("Display: received bar %u\n\r", data.bar_pos);
		safe_printf("Received size: %d\n\r", sizeof(data));

		draw_bricks(&TftInstance, data.bricks);
		draw_ball(&TftInstance, data.ball);

		set_erase();
		draw_bar(&TftInstance, prev_data[0].bar_pos);
		set_draw();
		draw_bar(&TftInstance, data.bar_pos);
		display_info(&TftInstance, data);

		/* Display message */
		if(data.game_state == WON || data.game_state == LOST)
			display_msg(&TftInstance, data.game_state);

		prev_data[0] = data;
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
        return (void*) XST_FAILURE;
    }
    ret = XMutex_CfgInitialize(&mtx_hw, configPtr_mutex, configPtr_mutex->BaseAddress);
    if (ret != XST_SUCCESS){
        print("[ERROR uB0]\t While initializing the Hardware Mutex\r\n");
        return (void*) XST_FAILURE;
    }

    /* Configure the mailbox */
    configPtr_mailbox = XMbox_LookupConfig(MBOX_DEVICE_ID);
    if (configPtr_mailbox == (XMbox_Config *)NULL) {
    	print("[ERROR uB0]\t While configuring the Mailbox\r\n");
        return (void*) XST_FAILURE;
    }
    ret = XMbox_CfgInitialize(&mbx_model, configPtr_mailbox, configPtr_mailbox->BaseAddress);
    if (ret != XST_SUCCESS) {
    	print("[ERROR uB0]\t While initializing the Mailbox\r\n");
        return (void*) XST_FAILURE;
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
