/*******************************************************************************
 *
 * Title:       EE4214 Bricks-Breaker Project
 * File:        config.h
 * Date:        2017-04-13
 * Author:      Paul-Edouard Sarlin (A0153124U)
 * Description: Contains the global parameters use by the model and the display.
 *
 ******************************************************************************/

#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

/* Thread- and process-safe display macro */
#define safe_printf(fmt,args...) { XMutex_Lock(&mtx_hw, HW_MTX_PRINT_IDX); \
                                   xil_printf(fmt, ##args); fflush(stdout); \
                                   XMutex_Unlock(&mtx_hw, HW_MTX_PRINT_IDX); }

#define GET_MS              (sys_xget_clock_ticks()*(SYSTMR_INTERVAL \
                                                     / SYSTMR_CLK_FREQ_KHZ))

/* Hardware mutex */
#define MUTEX_DEVICE_ID     XPAR_MUTEX_0_IF_1_DEVICE_ID
#define MUTEX_NUM           0
#define HW_MTX_PRINT_IDX    0

/* Mailbox between model and display */
#define MY_CPU_ID           XPAR_CPU_ID
#define MBOX_DEVICE_ID      XPAR_MBOX_0_DEVICE_ID

/* Bricks parameters */
#define NB_COLUMNS          10
#define NB_ROWS             8
#define NB_GOLD_COLUMNS     2
#define BRICK_H             15
#define BRICK_W             40
#define BRICK_OFFSET        5

/* Ball parameters */
#define BALL_R              7

/* Bar parameters */
#define BAR_W               80
#define BAR_H               5
#define BAR_OFFSET_Y        10
#define BAR_S               10
#define BAR_A               10
#define BAR_N               (BAR_W-2*(BAR_S+BAR_A))
#define BZ_W                455
#define BZ_H                360


/* Useful data types */
typedef enum Game_state {WAITING, RUNNING, WON, LOST, PAUSED} Game_state;
typedef enum Brick {BROKEN, NORMAL, GOLDEN} Brick;

typedef struct {
    Game_state game_state;
    u16 ball_posx, ball_posy, bar_pos;
    u16 ball_vel;
    Brick bricks[NB_COLUMNS][NB_ROWS];
    u16 score, time;
} Model_state;


#endif
