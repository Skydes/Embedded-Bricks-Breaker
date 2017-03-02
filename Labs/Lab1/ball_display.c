 /***************************************************************************************
 *
 * Title:       EE4214 Lab1_3
 * File:        ball_display.c
 * Date:        2017-02-14
 * Author:      Paul-Edouard Sarlin (A0153124U)
 * Comment:        Set ps7_ddr_0_GP0_AXI_BASENAME base address to 0x10400000 in linker.
 *
 ***************************************************************************************/

#include "xmk.h"
#include "sys/init.h"
#include "xtft.h"
#include "xtft_hw.h"
#include "xparameters.h"
#include "sys/timer.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_X               (XTFT_DISPLAY_WIDTH - 1)
#define MAX_Y               (XTFT_DISPLAY_HEIGHT - 1)
#define RATE                1000 // ms

#define TFT_DEVICE_ID       XPAR_TFT_0_DEVICE_ID
#define DDR_HIGH_ADDR       XPAR_PS7_DDR_0_S_AXI_HIGHADDR

#define NB_FRAMES           2
#define TFT_FRAME_ADDR1     0x10000000
#define TFT_FRAME_ADDR2     0x10200000

#define BLUE                0x000000ff
#define WHITE               0x00ffffff
#define BLACK               0x00ffffff
#define RADIUS              7
#define TEXT_X              2
#define TEXT_Y              2

#define safe_printf(fmt,args...) { pthread_mutex_lock(&printf_mutex); \
                                   xil_printf(fmt, ##args); fflush(stdout); \
                                   pthread_mutex_unlock(&printf_mutex); }

void* thread_func_0();
void* thread_func_1();
void draw(XTft *Tft, u16 x, u16 y, u32 color_text, u32 color_circle);
void drawCircle(XTft *Tft, u16 x0, u16 y0, u16 radius, u32 color, bool filled);
void drawHLine(XTft *Tft, int x1, int x2, u16 y, u32 color);
void setPixel(XTft *InstancePtr, int ColVal, int RowVal, u32 PixelVal);
u8 lastCnt();
u8 nextCnt();
void* main_prog(void *arg);

struct sched_param sched_par;
pthread_attr_t attr;

pthread_t tid0, tid1;
pthread_mutex_t pt_mutex, printf_mutex;
u16 pt_x, pt_y, display_rate = 0;

/* Two different frame buffers are used here */
u32 frames_addr[NB_FRAMES] = {TFT_FRAME_ADDR1, TFT_FRAME_ADDR2};
u8 frames_cnt = 0;

static XTft TftInstance;

/* Coordinates generator thread */
void* thread_func_0() {
    srand(0);
    u16 x, y;

    while(1) {
        x = rand() * (double) MAX_X / RAND_MAX;
        y = rand() * (double) MAX_Y / RAND_MAX;
        pthread_mutex_lock(&pt_mutex);
        pt_x = x; pt_y = y;
        pthread_mutex_unlock(&pt_mutex);
        safe_printf("x: %3u, y: %3u\n\r", x, y);
        sleep(RATE);
    }
}

/* Display thread */
void* thread_func_1() {
    int Status;
    XTft_Config *TftConfigPtr;
    u32 TftDeviceId = TFT_DEVICE_ID;

    u16 x, y, x_last[NB_FRAMES], y_last[NB_FRAMES];
    u32 ticks_current, ticks_last = sys_xget_clock_ticks(); // measure performances
    u8 i;

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
        x_last[i] = 0; y_last[i] = 0;
        TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];
        XTft_SetColor(&TftInstance, 0, WHITE);
        XTft_ClearScreen(&TftInstance);
        frames_cnt = nextCnt();
    }
    TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];

    while(1) {
        /* Acquire coordinates */
        pthread_mutex_lock(&pt_mutex);
        x = pt_x; y = pt_y;
        pthread_mutex_unlock(&pt_mutex);

        if((x != x_last[lastCnt()]) || (y != y_last[lastCnt()])) {

            draw(&TftInstance, x_last[frames_cnt], y_last[frames_cnt], WHITE, WHITE);
            draw(&TftInstance, x, y, BLACK, BLUE);

            x_last[frames_cnt] = x; y_last[frames_cnt] = y;

            /* Wait previous frame to be displayed */
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

void draw(XTft *Tft, u16 x, u16 y, u32 color_text, u32 color_circle) {
    char buff_print_x[6], buff_print_y[6];
    u8 i;

    /* Generate strings */
    sprintf(buff_print_x, "X=%3u",x);
    sprintf(buff_print_y, "Y=%3u",y);

    /* Print coordinates */
    XTft_SetColor(Tft, 0x000000, color_text);
    XTft_SetPosChar(Tft, TEXT_X,TEXT_Y);
    for(i = 0; i < 5; i++)
        XTft_Write(Tft,buff_print_x[i]);
    XTft_SetPosChar(Tft, TEXT_X, TEXT_Y+XTFT_CHAR_HEIGHT);
    for(i = 0; i < 5; i++)
        XTft_Write(Tft,buff_print_y[i]);

    /* Draw circle */
    drawCircle(Tft, x, y, RADIUS, color_circle, true);
}

u8 lastCnt() {
    return (NB_FRAMES + frames_cnt + 1) % NB_FRAMES;
}

u8 nextCnt() {
    return (NB_FRAMES + frames_cnt  - 1) % NB_FRAMES;
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
            XTft_SetPixel(Tft, x0 + x, y0 + y, color);
            XTft_SetPixel(Tft, x0 - x, y0 + y, color);
            XTft_SetPixel(Tft, x0 + y, y0 + x, color);
            XTft_SetPixel(Tft, x0 - y, y0 + x, color);
            XTft_SetPixel(Tft, x0 - x, y0 - y, color);
            XTft_SetPixel(Tft, x0 + x, y0 - y, color);
            XTft_SetPixel(Tft, x0 - y, y0 - x, color);
            XTft_SetPixel(Tft, x0 + y, y0 - x, color);
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

void setPixel(XTft *InstancePtr, int x, int y, u32 PixelVal) {
    if ((x < 0) || (y < 0) || (x >= XTFT_DISPLAY_WIDTH) || (y >= XTFT_DISPLAY_HEIGHT))
        return;
    Xil_Out32(InstancePtr->TftConfig.VideoMemBaseAddr 
        + (4 * ((y) * XTFT_DISPLAY_BUFFER_WIDTH + x)), PixelVal);
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
    pthread_mutex_init(&pt_mutex, NULL);
    pthread_mutex_init(&printf_mutex, NULL);

    print("-- Configuring threads.\r\n");

    pthread_attr_init(&attr);

    /* Thread 0: coordinates generation, priority 1 */
    sched_par.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &sched_par);
    ret = pthread_create (&tid0, &attr, (void*)thread_func_0, NULL);
    if (ret != 0)
        xil_printf ("-- ERROR (%d) launching thread_func_0...\r\n", ret);
    else
        xil_printf ("Thread 0 launched with ID %d \r\n",tid0);

    /* Thread 1: display management, priority 2 */
    sched_par.sched_priority = 2;
    pthread_attr_setschedparam(&attr, &sched_par);
    ret = pthread_create (&tid1, &attr, (void*)thread_func_1, NULL);
    if (ret != 0)
        xil_printf ("-- ERROR (%d) launching thread_func_1...\r\n", ret);
    else
        xil_printf ("Thread 1 launched with ID %d \r\n",tid1);

    return 0;
}

