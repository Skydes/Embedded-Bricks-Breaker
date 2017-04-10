
#ifndef SRC_CONFIG_MODEL_H_
#define SRC_CONFIG_MODEL_H_

#define UPDATE_MS			40
#define UPDATE_S			(UPDATE_MS/1000.)

#define PB_CENTER			0
#define PB_DOWN				1
#define PB_LEFT				2
#define PB_RIGHT			3
#define	PB_UP				4
#define DEBOUNCE_DELAY		30

#define BAR_DELAY_THRESH	250
#define BAR_JUMP_DIST		25
#define BAR_DEF_SPEED		200
#define BAR_ANGLE_CHANGE	15
#define BAR_VEL_CHANGE		100
#define BAR_MAX_ANGLE		345
#define BAR_MIN_ANGLE		195

#define BALL_DEF_SPEED		250
#define BALL_DEF_ANGLE		270
#define BALL_MAX_VEL		1000
#define BALL_MIN_VEL		50
#define BALL_DEF_X			(BZ_W)/2
#define BALL_DEF_Y			(BZ_H-BAR_OFFSET_Y-BAR_H-BALL_R-1)

#define NB_GOLDEN_COLS		2
#define THRESH_GOLDEN		10
#define SCORE_INC_GOLDEN	2
#define SCORE_INC_NORMAL	1
#define SCORE_VEL_CHANGE	25

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
	double c, s;
} Ball;

typedef struct {
	int idx;
	bool happened;
	u16 iter;
	u16 normal;
} Collision;


#endif
