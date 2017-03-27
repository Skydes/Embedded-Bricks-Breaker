
#ifndef SRC_CONFIG_MODEL_H_
#define SRC_CONFIG_MODEL_H_

#define UPDATE_MS			60
#define UPDATE_S			UPDATE_MS/1000.

#define GET_MS				sys_xget_clock_ticks()*(SYSTMR_INTERVAL / SYSTMR_CLK_FREQ_KHZ)

#define PB_CENTER			0
#define PB_DOWN				1
#define PB_LEFT				2
#define PB_RIGHT			3
#define	PB_UP				4
#define DEBOUNCE_DELAY		30

#define BAR_DELAY_THRESH	250
#define BAR_JUMP_DIST		25
#define BAR_DEF_SPEED		200

#define BALL_DEF_SPEED		150
#define BALL_DEF_ANGLE		270

#define NB_GOLDEN_COLS		2
#define THRESH_GOLDEN		10

typedef enum {NONE, LEFT, RIGHT} Bar_jump;

typedef struct {
	pthread_mutex_t mtx;
	int speed;
	Bar_jump jump;
	u32 last_change;
	int pos;
} Bar;

typedef struct {
	pthread_mutex_t mtx;
	u16 vel, angle;
	int x, y;
} Ball;

typedef struct {
	int idx;
	bool happened;
	u16 iter;
	u16 normal;
} Collision;


#endif
