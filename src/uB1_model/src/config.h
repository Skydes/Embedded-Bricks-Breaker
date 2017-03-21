
#ifndef SRC_CONFIG_H_
#define SRC_CONFIG_H_

/* Thread- and process-safe display macro */
#define safe_printf(fmt,args...) { XMutex_Lock(&mtx_hw, HW_MTX_PRINT_IDX); \
                                   xil_printf(fmt, ##args); fflush(stdout); \
                                   XMutex_Unlock(&mtx_hw, HW_MTX_PRINT_IDX); }

/* Hardware mutex */
#define MUTEX_DEVICE_ID 	XPAR_MUTEX_0_IF_1_DEVICE_ID
#define MUTEX_NUM 			0
#define HW_MTX_PRINT_IDX	0

/* Mailbox between model and display */
#define MY_CPU_ID 			XPAR_CPU_ID
#define MBOX_DEVICE_ID		XPAR_MBOX_0_DEVICE_ID

/* Bricks parameters */
#define NB_COLUMNS          10
#define NB_ROWS				8
#define NB_GOLD_COLUMNS     2
#define BRICK_H				15
#define BRICK_W				40
#define BRICK_OFFSET		5

/* Ball parameters */
#define BALL_RADIUS			7

/* Bar parameters */
#define BAR_W				80
#define BAR_H				5
#define BAR_OFFSET_Y		10


#define BZ_W				455
#define BZ_H				360


/* Useful data types */
typedef enum Game_state {WAITING, RUNNING, WON, LOST} Game_state;
typedef enum Brick_state {BROKEN, NORMAL, GOLDEN} Brick_state;

typedef struct {
	u16 vel, angle;
	int x, y;
} Ball;

typedef struct {
	Game_state game_state;
	Ball ball;
	u16 bar_pos;
	Brick_state bricks[NB_COLUMNS][NB_ROWS];
	u16 score, time;
} Model_state;


#endif
