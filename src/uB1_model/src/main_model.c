#include "xmk.h"
#include "xgpio.h"
#include "xparameters.h"
#include "xmutex.h"
#include "xmbox.h"

#include <errno.h>
#include <pthread.h>
#include <sys/init.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/intr.h>
#include <sys/timer.h>
#include <sys/process.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "config.h"
#include "config_model.h"
#include "timer.h"

int main(void);
void* main_prog(void *arg);
void init_game();

/* Hardware objects */
static XMbox mbx_display;
XMutex mtx_hw;
XGpio gpio_pb;

/* Synchronization objects */
pthread_t tid_ball, tid_bar, tid_timer_bar, tid_col[NB_COLUMNS];
sem_t sem_debounce, sem_start_check, sem_check_done[NB_COLUMNS], sem_arbitration_done;
pthread_mutex_t mtx_bricks, mtx_msgq;
Timer* timer_bar;

/* Model data */
volatile Game_state game_state;
Bar bar;
Ball ball;
Collision next_colli, column_colli[NB_COLUMNS];
Brick bricks[NB_COLUMNS][NB_ROWS];
u32 score, tot_remaining_bricks;
u8 remaining_bricks[NB_COLUMNS];
sem_t golden_brick, golden_release;


/* Push buttons ISR */
static void pb_ISR(void *arg) {
	static u32 prev_pb_state = 0;

	/* Clear the state of the interrupt flag and read the buttons states */
	XGpio_InterruptClear(&gpio_pb,1);
	u32 pb_state = XGpio_DiscreteRead(&gpio_pb, 1);

	if( (pb_state ^ prev_pb_state) & ((1 << PB_RIGHT) | (1 << PB_LEFT) | (1 << PB_CENTER)) ) {
		XGpio_InterruptDisable(&gpio_pb,1);
		sem_post(&sem_debounce);
//		safe_printf("Interrupt: %d \r\n", pb_state); // debug
	};
	prev_pb_state = pb_state;
}

static inline float rad(u16 deg) {
	return deg/180.*M_PI;
}

static inline int max(int a, int b) {
	return a>b ? a : b;
}

static inline int min(int a, int b) {
	return a<b ? a : b;
}

static inline int sign(double a) {
	return a >= 0 ? 1 : -1;
}

u16 check_angle(int angle) {
	int r = angle % 360;
	return r < 0 ? r + 360 : r;
}

void* thread_buttons() {
	u32 pb_state;
	u8 pb_right, pb_left, pb_center;
	u8 prev_pb_right = 0, prev_pb_left = 0;
	u32 actual_time;

	while(1) {
		sem_wait(&sem_debounce);
		if(!is_finished(timer_bar))
			sleep(DEBOUNCE_DELAY);

		pb_state = XGpio_DiscreteRead(&gpio_pb, 1);
		pb_right = (pb_state >> PB_RIGHT) & 1;
		pb_left = (pb_state >> PB_LEFT) & 1;
		pb_center = (pb_state >> PB_CENTER) & 1;

		actual_time = GET_MS;

		/* Manage bar movement */
		switch(game_state) {
			case WAITING:
				//ball.angle = BAR_MIN_ANGLE + sys_xget_clock_ticks()%(BAR_MAX_ANGLE - BAR_MIN_ANGLE);
			case PAUSED:
				if(pb_center)
					game_state = RUNNING;
				break;

			case RUNNING:
				if(pb_center) {
					game_state = PAUSED;
					break;
				}

				pthread_mutex_lock(&bar.mtx);
				if( !(pb_right ^ pb_left) ) { // both released or both pressed
					if( (actual_time - bar.last_change) < BAR_DELAY_THRESH) {
						timer_stop(timer_bar);
						if( !(prev_pb_right ^ prev_pb_left) )
							bar.jump = NONE;
						else if(prev_pb_right)
							bar.jump = RIGHT;
						else if(prev_pb_left)
							bar.jump = LEFT;
					}
					else
						bar.jump = NONE;
					if(bar.jump != NONE)
						safe_printf("Jumping\n\r");
					bar.speed = 0;
				}
				else {
					if(is_finished(timer_bar)) {
						bar.jump = NONE;
						bar.speed = BAR_DEF_SPEED * (pb_right ? 1 : -1);
						timer_stop(timer_bar);
					}
					else
						timer_start(timer_bar);
				}
				bar.last_change = actual_time;
				pthread_mutex_unlock(&bar.mtx);
				break;

			default:
				;
		}

		prev_pb_right = pb_right;
		prev_pb_left = pb_left;
		XGpio_InterruptEnable(&gpio_pb,1);
	}
}

void timer_bar_cb() {
	XGpio_InterruptDisable(&gpio_pb,1);
	sem_post(&sem_debounce);
}

void* thread_column(void *arg) {
	u8 idx = *((int *) arg); // local context copy
	free(arg);
	safe_printf("[INFO uB1] \t thread_column_%d launched with ID %d \r\n", idx, tid_col[idx]);

	Collision colli;
    Ball ball_new;
    bool out, golden = false;
    u16 iter_max;
    u8 row;
    int x_f, y_f, dist_x, dist_y, dx, dy;

    colli.idx = idx;

	while(1) {
		sem_wait(&sem_start_check);
		sem_post(&sem_start_check);

		if(golden && !sem_trywait(&golden_release)) {
			sem_post(&golden_brick);
			golden = false;
			for(u8 row = 0; row < NB_ROWS; row++) {
				if(bricks[idx][row] == GOLDEN)
					bricks[idx][row] = NORMAL;
			}
		}
		else if(!golden && !sem_trywait(&golden_brick)){
			golden = true;
			for(u8 row = 0; row < NB_ROWS; row++) {
				if(bricks[idx][row] == NORMAL)
					bricks[idx][row] = GOLDEN;
			}
		}

		/* Check collision using the shared structure ball */
		iter_max = round(UPDATE_S*ball.vel);
		colli.happened = false;

		x_f = ball.x + round(iter_max*ball.c);
		y_f = ball.y + round(iter_max*ball.s);
		out = ( (ball.y > (NB_COLUMNS*(BRICK_H+BRICK_OFFSET)+BALL_R+5)) &&
				(y_f > (NB_COLUMNS*(BRICK_H+BRICK_OFFSET)+BALL_R+5)) ) ||
			  ( (ball.x < (BRICK_OFFSET+idx*(BRICK_W+BRICK_OFFSET)-BALL_R-5)) &&
				(x_f < (BRICK_OFFSET+idx*(BRICK_W+BRICK_OFFSET)-BALL_R-5)) ) ||
		      ( (ball.x > ((idx+1)*(BRICK_W+BRICK_OFFSET)+BALL_R+5)) &&
		    	(x_f > ((idx+1)*(BRICK_W+BRICK_OFFSET)+BALL_R+5)) );


		if(!out) {
			for(colli.iter = 1; (colli.iter <= iter_max) && !colli.happened; colli.iter++) {
				ball_new.x = ball.x + round(colli.iter*ball.c);
				ball_new.y = ball.y + round(colli.iter*ball.s);

				dist_x = ball_new.x - (BRICK_OFFSET + BRICK_W/2 + idx*(BRICK_W + BRICK_OFFSET));

				if( (abs(dist_x) > (BRICK_W/2 + BALL_R)) || (ball_new.y > (NB_ROWS*(BRICK_OFFSET+BRICK_H)+BALL_R)) )
					continue;

				for(row = 0; row < NB_ROWS; row++) {
					if(bricks[idx][row] == BROKEN)
						continue;
					colli.happened = true;
					dist_y = ball_new.y - (BRICK_OFFSET + BRICK_H/2 + row*(BRICK_H + BRICK_OFFSET));
					/* Bounce on vertical edge */
					if( (abs(dist_x) == (BRICK_W/2 + BALL_R)) && (abs(dist_y) < BRICK_H/2)) {
						colli.normal = dist_x > 0 ? 0 : 180;
						break;
					}
					/* Bounce on horizontal edge */
					if( (abs(dist_y) == (BRICK_H/2 + BALL_R)) && (abs(dist_x) < BRICK_W/2) ) {
						colli.normal = dist_y > 0 ? 90 : 270;
						break;
					}
					/* Bounce on corner */
					dx = abs(dist_x) - BRICK_W/2;
					dy = abs(dist_y) - BRICK_H/2;
					if( (dx*dx + dy*dy) <= (BALL_R*BALL_R)) {
						colli.normal = (dist_y > 0 ? 1 : -1) * (90 + (dist_x > 0 ? -1 : 1) * 45);
						break;
					}
					colli.happened = false;
				}
				if(colli.happened == true) {
//					safe_printf("[col%d]\tCollision detected with normal %d\n\r", idx, colli.normal);
					break;
				}
			}
		}

		column_colli[idx] = colli;
		sem_post(&sem_check_done[idx]);

//		safe_printf("[col%d]\tWaiting for arbitration\n\r", idx);
		sem_wait(&sem_arbitration_done);
		sem_post(&sem_arbitration_done);

		if(next_colli.happened && (next_colli.idx == idx)) {
//			safe_printf("[col%d]\tCollision accepted, destruction of brick %d\n\r", idx, row);
			pthread_mutex_lock(&mtx_bricks);
			u8 inc = (bricks[idx][row] == GOLDEN) ? SCORE_INC_GOLDEN : SCORE_INC_NORMAL;
			bricks[idx][row] = BROKEN;
			if( (score/THRESH_GOLDEN) < ((score+inc)/THRESH_GOLDEN))
				for(u8 i = 0; i < NB_GOLDEN_COLS; i++)
					sem_post(&golden_release);
			score += inc;
			tot_remaining_bricks--;
            if(--remaining_bricks[idx] == 0) {
            	pthread_mutex_unlock(&mtx_bricks);
            	if(golden)
            		sem_post(&golden_brick);
            	pthread_exit(0);
            }
            else
            	pthread_mutex_unlock(&mtx_bricks);
		}
	}
}


void* thread_ball() {
	u32 actual_time, last_sent = GET_MS;
	Model_state model_state;
	Ball ball_new;
	int offset_x, offset_y, offset_angle, offset_vel;
	sem_post(&sem_arbitration_done);

	while(1) {

		/* Copy current state of bricks */
		pthread_mutex_lock(&mtx_bricks);
		model_state.score = score;
		memcpy(&(model_state.bricks), &bricks, NB_COLUMNS*NB_ROWS*sizeof(Brick));
		if(tot_remaining_bricks == 0)
			game_state = WON;
		pthread_mutex_unlock(&mtx_bricks);

		if(game_state == RUNNING) {

            ball.c = cos(rad(ball.angle));
            ball.s = sin(rad(ball.angle));

			/* Launch collision checking with bricks */
			sem_wait(&sem_arbitration_done); // cancel last post
//			safe_printf("[ball]\tUnlocking columns for collision checking\n\r");
			sem_post(&sem_start_check);

			/* Update Bar position */
			pthread_mutex_lock(&bar.mtx);
			switch(bar.jump) {
				case RIGHT:
					bar.pos += BAR_JUMP_DIST;
					break;
				case LEFT:
					bar.pos -= BAR_JUMP_DIST;
					break;
				default:
					bar.pos += bar.speed*UPDATE_S;
			}
			bar.jump = NONE;
			pthread_mutex_unlock(&bar.mtx);

			/* Enforce screen width limits for bar */
			if(bar.pos < (BAR_W/2 + 1))
				bar.pos = BAR_W/2 + 1;
			else if(bar.pos >= (BZ_W - BAR_W/2))
				bar.pos = BZ_W - BAR_W/2 - 1;


			u16 iter_max = round(UPDATE_S*ball.vel);
			next_colli.idx = -1; // default value for bounce against wall/bar
			next_colli.happened = false;
			ball_new.vel = ball.vel; // maybe fully copy ball
            ball_new.angle = ball.angle;
            offset_angle = 0;
            offset_vel = 0;

			for(next_colli.iter = 1; next_colli.iter <= iter_max; next_colli.iter++) {
				ball_new.x = ball.x + round(next_colli.iter*ball.c);
				ball_new.y = ball.y + round(next_colli.iter*ball.s);
				next_colli.happened = true;

				/* Bounce on left boundary */
				if( (ball_new.x-BALL_R) <= 0) {
					next_colli.normal = 0;
//					print("Bounce left wall.\n\r");
					break;
				}
				/* Bounce on right boundary */
				if( (ball_new.x+BALL_R) >= (BZ_W) ) {
					next_colli.normal = 180;
//					print("Bounce right wall.\n\r");
					break;
				}
				/* Bounce on top */
				if( (ball_new.y-BALL_R) <= 0) {
					next_colli.normal = 90;
//					print("Bounce top.\n\r");
					break;
				}
				/* Reach bottom */
				if((ball_new.y+BALL_R) >= BZ_H) {
					game_state = LOST;
					next_colli.normal = 270;
//					print("Lost game...\n\r");
					break;
				}
				/* Bounce on corner of bar */
				int dx = abs(ball_new.x-bar.pos) - BAR_W/2;
				int dy = BZ_H-BAR_OFFSET_Y-BAR_H-ball_new.y;
				if( (dx*dx + dy*dy) < (BALL_R*BALL_R)) {
					next_colli.normal = (ball_new.x < bar.pos) ? 225 : 315;
//					print("Bounce on corner of bar.\n\r");
					break;
				}
				/* Bounce on flat edge of bar */
				if( ((ball_new.y+BALL_R) >= (BZ_H-BAR_OFFSET_Y-BAR_H))
					&& (abs(ball_new.x-bar.pos) <= BAR_W/2) ) {
					next_colli.normal = 270;
//					print("Bounce on flat bar.\n\r");
                    /* Bounce on A segment */
                    if(abs(ball_new.x-bar.pos) > (BAR_N/2+BAR_S))
                        offset_angle = BAR_ANGLE_CHANGE*sign(ball_new.x-bar.pos);
                    /* Bounce on S segment */
                    else if(abs(ball_new.x-bar.pos) > BAR_N/2)
                        offset_vel = BAR_VEL_CHANGE*sign(ball_new.x-bar.pos);
					break;
				}
				next_colli.happened = false;
			}

			/* Receive result from collision checking with bricks */
//			safe_printf("[ball]\tReceive result from columns\n\r");
			for(u8 i  = 0; i < NB_COLUMNS; i++) {
				pthread_mutex_lock(&mtx_bricks);
                if(remaining_bricks[i] == 0) { // no brick left
                	pthread_mutex_unlock(&mtx_bricks);
                	continue;
                }
                pthread_mutex_unlock(&mtx_bricks);
                sem_wait(&sem_check_done[i]);
				/* Pick the soonest collision */
				if( !column_colli[i].happened || (column_colli[i].iter >= next_colli.iter) )
					continue;
				next_colli = column_colli[i];
				safe_printf("[ball]\tNew collision detected\n\r");
			}

			sem_wait(&sem_start_check); // cancel last post

			/* Confirm the result of the arbitration */
			sem_post(&sem_arbitration_done);
			yield();

			pthread_mutex_lock(&mtx_bricks);
//			if( ((model_state.score % THRESH_GOLDEN) != 0) && ((score % THRESH_GOLDEN) == 0))
			if( (model_state.score/10) < (score/10))
				ball_new.vel = min(ball_new.vel+SCORE_VEL_CHANGE,BALL_MAX_VEL);
			pthread_mutex_unlock(&mtx_bricks);

			if(next_colli.happened) {
				ball_new.angle = check_angle(2*next_colli.normal - (int) ball.angle + 180);
				offset_x = sign(cos(rad(next_colli.normal)))*round(fabs(cos(rad(next_colli.normal))));
				offset_y = sign(sin(rad(next_colli.normal)))*round(fabs(sin(rad(next_colli.normal))));
				ball_new.x = ball.x + round(next_colli.iter*cos(rad(ball.angle))) + offset_x;
				ball_new.y = ball.y + round(next_colli.iter*sin(rad(ball.angle))) + offset_y;
                
                if(next_colli.idx == -1) {
                    if(offset_angle > 0)
                        ball_new.angle = min(ball_new.angle+offset_angle,BAR_MAX_ANGLE);
                    if(offset_angle < 0)
                        ball_new.angle = max(ball_new.angle+offset_angle,BAR_MIN_ANGLE);
                    if(offset_vel > 0)
                        ball_new.vel = min(ball_new.vel+offset_vel,BALL_MAX_VEL);
                    if(offset_vel < 0)
                        ball_new.vel = max(ball_new.vel+offset_vel,BALL_MIN_VEL);
                }
			}
			ball = ball_new;
		}

		/* Prepare message to be sent */
		model_state.ball_posx = ball.x;
		model_state.ball_posy = ball.y;
		model_state.ball_vel = ball.vel;
		model_state.bar_pos = bar.pos;
		model_state.game_state = game_state;
		model_state.time = GET_MS / 1000;

		/* Make sure the refresh rate is constant */
		actual_time = GET_MS;
		int wait_time = UPDATE_MS - (int) actual_time + (int) last_sent;
//		safe_printf("Wait time: %d\n\r", wait_time);
		sleep(max(wait_time,0));
		last_sent = GET_MS;

		XMbox_WriteBlocking(&mbx_display, (u32*)&model_state, sizeof(model_state));
	}
}

void init_game() {
	srand(GET_MS);
	game_state = WAITING;

	pthread_mutex_lock(&bar.mtx);
	bar.pos = BZ_W / 2;
	bar.speed = 0;
	bar.jump = NONE;
	bar.last_change = 0;
	pthread_mutex_unlock(&bar.mtx);

	pthread_mutex_lock(&ball.mtx);
	ball.x = BALL_DEF_X;
	ball.y = BALL_DEF_Y;
	ball.vel = BALL_DEF_SPEED;
	ball.angle = BALL_DEF_ANGLE;
	pthread_mutex_unlock(&ball.mtx);

	pthread_mutex_lock(&mtx_bricks);
//	srand(GET_MS);
//	bool golden[NB_COLUMNS] = {false};
//	for(u8 col = 0; col < NB_GOLDEN_COLS; col++) {
//		u8 try = rand() % NB_COLUMNS;
//		if(!golden[try])
//			golden[try] = true;
//		else
//			col--;
//	}
	for(u8 col = 0; col < NB_COLUMNS; col++) {
		remaining_bricks[col] = NB_ROWS;
		for(u8 row = 0; row < NB_ROWS; row++) {
			bricks[col][row] = NORMAL;
//			if(golden[col])
//				bricks[col][row] = GOLDEN;
//			else
//				bricks[col][row] = NORMAL;
		}
	}
	score = 0;
	tot_remaining_bricks = NB_COLUMNS*NB_ROWS;
	pthread_mutex_unlock(&mtx_bricks);
}

void* main_prog(void *arg) {
    XMutex_Config* configPtr_mutex;
    XMbox_Config* configPtr_mailbox;
    pthread_attr_t attr;
    struct sched_param sched_par;
    int ret;

    init_game();

    print("[INFO uB1] \t Starting configuration\r\n");

    pthread_attr_init(&attr);

    /* Initialize semaphores */
    sem_init(&sem_debounce, 1, 0);
    sem_init(&sem_start_check, 1, 0);
    sem_init(&sem_arbitration_done, 1, 0);
    sem_init(&golden_brick, 1, NB_GOLDEN_COLS);
    sem_init(&golden_release, 1, 0);
    for(u8 i = 0; i < NB_COLUMNS; i++)
    	sem_init(&sem_check_done[i], 1, 0);

    /* Initialize mutexes */
    pthread_mutex_init(&ball.mtx, NULL);
    pthread_mutex_init(&bar.mtx, NULL);
    pthread_mutex_init(&mtx_bricks, NULL);
    pthread_mutex_init(&mtx_msgq, NULL);

    /* Configure the HW Mutex */
    configPtr_mutex = XMutex_LookupConfig(MUTEX_DEVICE_ID);
    if (configPtr_mutex == (XMutex_Config *)NULL){
        print("[ERROR uB1]\t While configuring the Hardware Mutex\r\n");
        return (void*) XST_FAILURE;
    }
    ret = XMutex_CfgInitialize(&mtx_hw, configPtr_mutex, configPtr_mutex->BaseAddress);
    if (ret != XST_SUCCESS){
        print("[ERROR uB1]\t While initializing the Hardware Mutex\r\n");
        return (void*) XST_FAILURE;
    }

    /* Configure the mailbox */
    configPtr_mailbox = XMbox_LookupConfig(MBOX_DEVICE_ID);
    if (configPtr_mailbox == (XMbox_Config *)NULL) {
    	print("[ERROR uB1]\t While configuring the Mailbox\r\n");
        return (void*) XST_FAILURE;
    }
    ret = XMbox_CfgInitialize(&mbx_display, configPtr_mailbox, configPtr_mailbox->BaseAddress);
    if (ret != XST_SUCCESS) {
    	print("[ERROR uB1]\t While initializing the Mailbox\r\n");
        return (void*) XST_FAILURE;
    }

    /* Configure interrupts */
    ret = XGpio_Initialize(&gpio_pb, XPAR_GPIO_0_DEVICE_ID);
	XGpio_SetDataDirection(&gpio_pb, 1, 0x000000FF); // all 1 --> intput
	XGpio_InterruptGlobalEnable(&gpio_pb);
	XGpio_InterruptEnable(&gpio_pb,1);
	register_int_handler(XPAR_MICROBLAZE_1_AXI_INTC_AXI_GPIO_0_IP2INTC_IRPT_INTR,
						 pb_ISR, &gpio_pb);
	enable_interrupt(XPAR_MICROBLAZE_1_AXI_INTC_AXI_GPIO_0_IP2INTC_IRPT_INTR);


	/* Create timers */
	timer_bar = timer_init(BAR_DELAY_THRESH, &timer_bar_cb);


	/* Column management threads, priority 2 */
    sched_par.sched_priority = 2;
    pthread_attr_setschedparam(&attr, &sched_par);
    for(int i = 0; i < NB_COLUMNS; i++) {
        int *arg = malloc(sizeof(arg));
        if (arg == NULL) {
            safe_printf("Couldn't allocate memory for thread column.\r\n");
            free(arg);
            return (void*) XST_FAILURE;
        }
        *arg = i; // pass column index as argument
        ret = pthread_create(&tid_col[i], &attr, (void*)thread_column, arg);
        if (ret != 0) {
			safe_printf("[ERROR uB1]\t (%d) launching thread_column %d\r\n", ret, i);
			return (void*) XST_FAILURE;
		}
    }

    /* Bar management thread, priority 1 */
    sched_par.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &sched_par);
    ret = pthread_create (&tid_bar, &attr, (void*)thread_buttons, NULL);
    if (ret != 0) {
        safe_printf("[ERROR uB1]\t (%d) launching thread_bar\r\n", ret);
        return (void*) XST_FAILURE;
    }
    else
        safe_printf("[INFO uB1] \t thread_bar launched with ID %d \r\n", tid_bar);

    /* Ball management thread, priority 3 */
    sched_par.sched_priority = 3;
    pthread_attr_setschedparam(&attr, &sched_par);
    ret = pthread_create (&tid_ball, &attr, (void*)thread_ball, NULL);
    if (ret != 0) {
        safe_printf("[ERROR uB1]\t (%d) launching thread_ball\r\n", ret);
        return (void*) XST_FAILURE;
    }
    else
        safe_printf("[INFO uB1] \t thread_ball launched with ID %d \r\n", tid_ball);

    return 0;
}

int main(void) {
    print("[INFO uB1] \t Entering main()\r\n");
    xilkernel_init();
    xmk_add_static_thread(main_prog,0);
    xilkernel_start();
    xilkernel_main ();

    //Control does not reach here
}
