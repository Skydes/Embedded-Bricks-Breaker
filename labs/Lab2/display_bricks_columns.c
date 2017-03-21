 /***************************************************************************************
 *
 * Title:       EE4214 Lab2_1
 * File:        display_bricks_columns.c
 * Date:        2017-03-02
 * Author:      Paul-Edouard Sarlin (A0153124U)
 * Comment:     Set ps7_ddr_0_GP0_AXI_BASENAME base address to 0x10400000 in linker.
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

#define NB_COLUMNS          8
#define NB_RED_COLUMNS      2
#define COLUMN_HEIGHT       240
#define RATE                100

#define TFT_DEVICE_ID       XPAR_TFT_0_DEVICE_ID
#define DDR_HIGH_ADDR       XPAR_PS7_DDR_0_S_AXI_HIGHADDR

#define NB_FRAMES           2
#define TFT_FRAME_ADDR1     0x10000000
#define TFT_FRAME_ADDR2     0x10200000

#define RED                 0x00ff0000
#define BLUE                0x000000ff
#define WHITE               0x00ffffff
#define BLACK               0x00000000

#define safe_printf(fmt,args...) { pthread_mutex_lock(&printf_mutex); \
                                   xil_printf(fmt, ##args); fflush(stdout); \
                                   pthread_mutex_unlock(&printf_mutex); }

void* thread_display();
int drawSolidBox(XTft *Tft, int x1, int y1, int x2, int y2, unsigned int col);
u8 lastCnt();
u8 nextCnt();
void* main_prog(void *arg);


struct sched_param sched_par;
pthread_attr_t attr;
pthread_t tid_disp, tid_columns[NB_COLUMNS];
pthread_mutex_t red_mutex, printf_mutex;
sem_t sem;

u16 display_rate = 0;
u8 col_is_red[NB_COLUMNS];

/* Two different frame buffers are used here */
u32 frames_addr[NB_FRAMES] = {TFT_FRAME_ADDR1, TFT_FRAME_ADDR2};
u8 frames_cnt = 0;

static XTft TftInstance;
XGpio gpPB; //PB device instance


/* Display thread */
void* thread_display() {
    int Status;
    XTft_Config *TftConfigPtr;
    u32 TftDeviceId = TFT_DEVICE_ID;

    u8 i, j, colors[NB_COLUMNS], colors_last[NB_FRAMES][NB_COLUMNS];

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
        TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];
        XTft_SetColor(&TftInstance, 0, WHITE);
        XTft_ClearScreen(&TftInstance);
        frames_cnt = nextCnt();
        for(j = 0; j < NB_COLUMNS; j++)
            colors_last[i][j] = -1;
    }
    TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];

    while(1) {

        /* Locally copy the columns states */
        pthread_mutex_lock(&red_mutex);
        memcpy(colors, col_is_red, NB_COLUMNS*sizeof(u8));
        pthread_mutex_unlock(&red_mutex);

        /* Draw columns */
        for(i = 0; i < NB_COLUMNS; i++) {
            if(colors[i] != colors_last[frames_cnt][i]) {
                u32 color = colors[i] ? RED : BLUE; // change to red for some values of i
                drawSolidBox(&TftInstance, i*(XTFT_DISPLAY_WIDTH/NB_COLUMNS) + 1, 0,
                                           (i+1)*(XTFT_DISPLAY_WIDTH/NB_COLUMNS)-1 - 1,
                                           COLUMN_HEIGHT, color);
                colors_last[frames_cnt][i] = colors[i];
            }
        }

        /* Wait previous frame to be displayed */
        while (XTft_GetVsyncStatus(&TftInstance) != XTFT_IESR_VADDRLATCH_STATUS_MASK);
        /* Force display to the current frame buffer */
        XTft_SetFrameBaseAddr(&TftInstance, frames_addr[frames_cnt]);
        /* Switch frame counter */
        frames_cnt = nextCnt();
        /* Set the new frame address for subsequent draws */
        TftInstance.TftConfig.VideoMemBaseAddr = frames_addr[frames_cnt];
    }
}

void* thread_column (void *arg) {
    u8 idx = *((int *) arg); // local context copy
    free(arg);
    safe_printf("Thread %d launched.\r\n", idx);
    srand(idx);
    u16 delay;
    while(1) {
        sem_wait(&sem);
        pthread_mutex_lock(&red_mutex);
        col_is_red[idx] = 1;
        pthread_mutex_unlock(&red_mutex);
        delay = rand() * (double) RATE / RAND_MAX;
        safe_printf("Thread %u waiting %u.\r\n", idx, delay);
        sleep(delay);
        pthread_mutex_lock(&red_mutex);
        col_is_red[idx] = 0;
        pthread_mutex_unlock(&red_mutex);
        sem_post(&sem);
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
    pthread_mutex_init(&red_mutex, NULL);
    sem_init(&sem, 1, NB_RED_COLUMNS);

    for(int i = 0; i < NB_COLUMNS; i++)
        col_is_red[i] = 0;

    xil_printf("--> Configuring threads.\r\n");
    pthread_attr_init(&attr);

    /* Display management thread, priority 1 */
    sched_par.sched_priority = 2;
    pthread_attr_setschedparam(&attr, &sched_par);
    ret = pthread_create (&tid_disp, &attr, (void*)thread_display, NULL);
    if (ret != 0)
        xil_printf ("-- ERROR (%d) launching thread_func_1...\r\n", ret);
    else
        xil_printf ("Thread 1 launched with ID %d \r\n",tid_disp);

    /* One thread for each column, priority 1 */
    sched_par.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &sched_par);
    for (int i = 0; i < NB_COLUMNS; i++) {
        int *arg = malloc(sizeof(arg));
        if (arg == NULL) {
            xil_printf("Couldn't allocate memory for thread column.\r\n");
            continue;
        }
        *arg = i; // pass column index as argument
        pthread_create(&tid_columns[i], &attr, (void*)thread_column, arg);
    }

    return 0;
}
