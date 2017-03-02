 /***************************************************************************************
 *
 * Title:       EE4214 Lab2_1
 * File:        display_move_bar.c
 * Date:        2017-03-02
 * Author:      Paul-Edouard Sarlin (A0153124U)
 * Comment:	    Set ps7_ddr_0_GP0_AXI_BASENAME base address to 0x10400000 in linker.
 *
 ***************************************************************************************/

#include "xmk.h"
#include "sys/init.h"
#include "xtft.h"
#include "xtft_hw.h"
#include "xgpio.h"
#include "xparameters.h"
#include <sys/timer.h> // for sleep
#include <sys/intr.h> // for interrupts

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define BAR_POS_Y 			(3 * XTFT_DISPLAY_HEIGHT / 4)
#define BAR_HEIGHT			10
#define BAR_WIDTH			40
#define REFRESH_INC			2

#define TFT_DEVICE_ID    	XPAR_TFT_0_DEVICE_ID
#define DDR_HIGH_ADDR    	XPAR_PS7_DDR_0_S_AXI_HIGHADDR

#define NB_FRAMES			2
#define TFT_FRAME_ADDR1    	0x10000000
#define TFT_FRAME_ADDR2    	0x10200000

#define BLUE				0x000000ff
#define WHITE				0x00ffffff
#define BLACK				0x00ffffff

#define safe_printf(fmt,args...) { pthread_mutex_lock(&printf_mutex); \
								   xil_printf(fmt, ##args); fflush(stdout); \
								   pthread_mutex_unlock(&printf_mutex); }

static void gpPBIntHandler(void *arg);
void* thread_display();
int drawSolidBox(XTft *Tft, int x1, int y1, int x2, int y2, unsigned int col);
u8 lastCnt();
u8 nextCnt();
void* main_prog(void *arg);

enum PB_STATE {RIGHT, LEFT, NONE};
typedef enum PB_STATE PB_STATE;
PB_STATE pb_state = NONE;

struct sched_param sched_par;
pthread_attr_t attr;

pthread_t tid_disp;
pthread_mutex_t printf_mutex;
u16 pt_x, pt_y, display_rate = 0;

/* Two different frame buffers are used here */
u32 frames_addr[NB_FRAMES] = {TFT_FRAME_ADDR1, TFT_FRAME_ADDR2};
u8 frames_cnt = 0;

static XTft TftInstance;
XGpio gpPB; //PB device instance


/* Push buttons ISR */
static void gpPBIntHandler(void *arg)
{
	unsigned char val;
	/* Clear the state of the interrupt flag and read the buttons states */
	XGpio_InterruptClear(&gpPB,1);
	val = XGpio_DiscreteRead(&gpPB, 1);

	/* Update the state variable, assume atomic write */
	u8 pb_right = (val >> 2) & 1, pb_left = (val >> 3) & 1;
	if(pb_right && !pb_left)
		pb_state = RIGHT;
	else if(pb_left && !pb_right)
		pb_state = LEFT;
	else
		pb_state = NONE;

	safe_printf("PB event, val = %d \r\n", val); // debug
}


/* Display thread */
void* thread_display() {
	int Status;
	XTft_Config *TftConfigPtr;
	u32 TftDeviceId = TFT_DEVICE_ID;

	u8 i;
	u16 x = XTFT_DISPLAY_WIDTH/2 , x_last[NB_FRAMES];
	u32 ticks_current, ticks_last = sys_xget_clock_ticks(); // measure performances
	PB_STATE pb_state_local;

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
	for(i = 0; i < NB_FRAMES; i++) {
		x_last[i] = -1;
		TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];
		XTft_SetColor(&TftInstance, 0, WHITE);
		XTft_ClearScreen(&TftInstance);
		frames_cnt = nextCnt();
	}
	TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];

	while(1) {

		/* Update new bar position */
		pb_state_local = pb_state;
		switch(pb_state_local) {
		case RIGHT:
			x -= REFRESH_INC;
			break;
		case LEFT:
			x += REFRESH_INC;
			break;
		default:
			break;
		}

		/* Enforce screen width limits */
		if(x < BAR_WIDTH/2) {
			x = BAR_WIDTH/2;
		}
		else if(x >= (XTFT_DISPLAY_WIDTH - BAR_WIDTH/2)) {
			x = XTFT_DISPLAY_WIDTH - BAR_WIDTH/2 - 1;
		}

		if(x != x_last[lastCnt()]) {

			drawSolidBox(&TftInstance, x_last[frames_cnt] - BAR_WIDTH/2,
                                       BAR_POS_Y + BAR_HEIGHT/2,
									   x_last[frames_cnt] + BAR_WIDTH/2, 
                                       BAR_POS_Y - BAR_HEIGHT/2, 
                                       WHITE);
			drawSolidBox(&TftInstance, x - BAR_WIDTH/2, 
                                       BAR_POS_Y + BAR_HEIGHT/2,
									   x + BAR_WIDTH/2, 
                                       BAR_POS_Y - BAR_HEIGHT/2,
                                       BLUE);

			x_last[frames_cnt] = x;

			/* Wait for the previous frame to be displayed */
			while (XTft_GetVsyncStatus(&TftInstance) != XTFT_IESR_VADDRLATCH_STATUS_MASK);
			/* Force display to the current frame buffer */
			XTft_SetFrameBaseAddr(&TftInstance, frames_addr[frames_cnt]);
			/* Switch frame counter */
			frames_cnt = nextCnt();
			/* Set the new frame address for subsequent draws */
			TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];

			/* Measure and display FPS */
			ticks_current = sys_xget_clock_ticks();
			safe_printf("Display refreshment period: %u ms\n\r",
                (ticks_current - ticks_last) * (SYSTMR_INTERVAL / SYSTMR_CLK_FREQ_KHZ));
			ticks_last = ticks_current;
		}
	}
}


u8 lastCnt() {
	return (NB_FRAMES + frames_cnt + 1) % NB_FRAMES;
}

u8 nextCnt() {
	return (NB_FRAMES + frames_cnt  - 1) % NB_FRAMES;
}

int drawSolidBox(XTft *Tft, int x1, int y1, int x2, int y2, unsigned int col) {
  int xmin,xmax,ymin,ymax,i,j;

  if (x1 >= 0 &&
      x1 <= XTFT_DISPLAY_WIDTH-1 &&
      x2 >= 0 &&
      x2 <= XTFT_DISPLAY_WIDTH-1 &&
      y1 >= 0 &&
      y1 <= XTFT_DISPLAY_HEIGHT-1 &&
      y2 >= 0 &&
      y2 <= XTFT_DISPLAY_HEIGHT-1) {
    if (x2 < x1) {
      xmin = x2;
      xmax = x1;
    }
    else {
      xmin = x1;
      xmax = x2;
    }
    if (y2 < y1) {
      ymin = y2;
      ymax = y1;
    }
    else {
      ymin = y1;
      ymax = y2;
    }

    for (i=xmin; i<=xmax; i++) {
      for (j=ymin; j<=ymax; j++) {
        XTft_SetPixel(Tft, i, j, col);
      }
    }
    return 0;
  }
  return 1;
}

int main (void) {
	print("-- Starting.\r\n");

	xilkernel_init();
	xmk_add_static_thread(main_prog, 0);
	xilkernel_start();

	return 0;
}

void* main_prog(void *arg) {
	int ret;
	pthread_mutex_init(&printf_mutex, NULL);

	xil_printf("--> Configuring threads.\r\n");
	pthread_attr_init(&attr);

	/* Display management */
	sched_par.sched_priority = 1;
	pthread_attr_setschedparam(&attr, &sched_par);
	ret = pthread_create (&tid_disp, &attr, (void*)thread_display, NULL);
	if (ret != 0)
		xil_printf ("-- ERROR (%d) launching thread_func_1...\r\n", ret);
	else
		xil_printf ("Thread 1 launched with ID %d \r\n",tid_disp);

	xil_printf("--> Configuring interrupts.\r\n");
	ret = XGpio_Initialize(&gpPB, XPAR_GPIO_0_DEVICE_ID);
	XGpio_SetDataDirection(&gpPB, 1, 0x000000FF); // all 1 --> intput
	XGpio_InterruptGlobalEnable(&gpPB);
	XGpio_InterruptEnable(&gpPB,1);
	register_int_handler(XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_0_IP2INTC_IRPT_INTR, 
                         gpPBIntHandler, 
                         &gpPB);
	enable_interrupt(XPAR_MICROBLAZE_0_AXI_INTC_AXI_GPIO_0_IP2INTC_IRPT_INTR);

	return 0;
}
