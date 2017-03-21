
#ifndef SRC_CONFIG_MODEL_H_
#define SRC_CONFIG_MODEL_H_

#define UPDATE_MS			500
#define UPDATE_S			UPDATE_MS/1000.

#define GET_MS				sys_xget_clock_ticks()*(SYSTMR_INTERVAL / SYSTMR_CLK_FREQ_KHZ)

#define PB_MIDDLE			0
#define PB_DOWN				1
#define PB_RIGHT			2
#define PB_LEFT				3
#define	PB_UP				4
#define DEBOUNCE_DELAY		100

#define BAR_DELAY_THRESH	250
#define BAR_JUMP_DIST		25
#define BAR_DEF_SPEED		200

#define BALL_DEF_SPEED		250
#define BALL_DEF_ANGLE		270

typedef enum {NONE, LEFT, RIGHT} Bar_jump;

typedef struct {
	pthread_mutex_t mtx;
	int speed;
	Bar_jump jump;
	u32 last_change;
	int pos;
} Bar;


#endif
