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
#include <sys/timer.h> // for sleep

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

#include "config.h"
#include "config_model.h"
#include "timer.h"

int main (void);
void* main_prog(void *arg);

static XMbox mbx_display;
XMutex mtx_hw;
XGpio gpio_pb;

pthread_t tid_ball, tid_bar, tid_timer_bar, tid_col[NB_COLUMNS];
sem_t sem_debounce, sem_check_collision, sem_arbitration_done;

volatile Game_state game_state;
Bar bar;
Ball ball;
Timer* timer_bar;


void init_game();

/* Push buttons ISR */
static void pb_ISR(void *arg) {
	static u32 prev_pb_state = 0;

	/* Clear the state of the interrupt flag and read the buttons states */
	XGpio_InterruptClear(&gpio_pb,1);
	u32 pb_state = XGpio_DiscreteRead(&gpio_pb, 1);

	if( (pb_state ^ prev_pb_state) & ((1 << PB_RIGHT) | (1 << PB_LEFT) | (1 << PB_CENTER)) ) {
		XGpio_InterruptDisable(&gpio_pb,1);
		sem_post(&sem_debounce);
		safe_printf("Interrupt: %d \r\n", pb_state); // debug
	};
	prev_pb_state = pb_state;
}

float rad(u16 deg) {
	return deg/180.*M_PI;
}

int max(int a, int b) {
	return a>b ? a : b;
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
				if(pb_center)
					game_state = RUNNING;
				break;

			case RUNNING:
				pthread_mutex_lock(&bar.mtx);
				if( !(pb_right ^ pb_left) ) { // both released or both pressed
					if( (actual_time - bar.last_change) < BAR_DELAY_THRESH) {
						timer_stop(timer_bar);
						safe_printf("Jumping\n\r");
						if( !(prev_pb_right ^ prev_pb_left) )
							bar.jump = NONE;
						else if(prev_pb_right)
							bar.jump = RIGHT;
						else if(prev_pb_left)
							bar.jump = LEFT;
					}
					else
						bar.jump = NONE;
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
//				if(pb_center)
//					init_game();
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

void* thread_column (void *arg) {
   u8 idx = *((int *) arg); // local context copy
   free(arg);
   safe_printf("[INFO uB1] \t thread_column_%d launched with ID %d \r\n", idx, tid_ball);

   Collision_result result;
   int msgid = msgget(idx+1, IPC_CREAT);

   while(1) {

	   sem_wait(&sem_check_collision);
	   sem_post(&sem_check_collision);

	   // check collision using the shared structure ball

	   msgsnd(msgid, &result, sizeof(result), 0);

	   sem_wait(&sem_arbitration_done);
	   sem_post(&sem_arbitration_done);

	   // update count of points
	   // update bricks
	   // try to lock the semaphore if available ?

   }

}


void* thread_ball() {
	u32 actual_time, last_sent = GET_MS;
	Model_state model_state;
	Ball ball_new;

	Collision_result collision_columns[NB_COLUMNS];

	model_state.time = 0;
	model_state.score = 0;
	for(u8 col = 0; col < NB_COLUMNS; col++)
		for(u8 row = 0; row < NB_ROWS; row++) {
			if( (col == 1 || col == 5) && (row > 1) && (row < 5))
				model_state.bricks[col][row] = GOLDEN;
			else
				model_state.bricks[col][row] = NORMAL;
		}

	while(1) {

		if(game_state == RUNNING) {

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


			u16 dist_max = UPDATE_S*ball.vel;
			int dist_collision;
			u16 normal = check_angle(ball.angle - 90);

			for(dist_collision = 1; dist_collision < dist_max; dist_collision++) {
				ball_new.x = ball.x + round(dist_collision*cos(rad(ball.angle)));
				ball_new.y = ball.y + round(dist_collision*sin(rad(ball.angle)));

				/* Bounce on left boundary */
				if( (ball_new.x-BALL_R) <= 0) {
					normal = 0;
					print("Bounce left wall.\n\r");
					break;
				}
				/* Bounce on right boundary */
				if( (ball_new.x+BALL_R) >= (BZ_W-1) ) {
					normal = 180;
					print("Bounce right wall.\n\r");
					break;
				}
				/* Bounce on top */
				if( (ball_new.y-BALL_R) <= 0) {
					normal = 90;
					print("Bounce top.\n\r");
					break;
				}
				/* Reach bottom */
				if((ball_new.y+BALL_R) >= BZ_H) {
					//game_state = LOST;
					normal = 270;
					print("Lost game...\n\r");
					break;
				}
				/* Bounce on corner of bar */
				int dx = abs(ball_new.x-bar.pos) - BAR_W/2;
				int dy = BZ_H-BAR_OFFSET_Y-BAR_H-ball_new.y;
				if( (dx*dx + dy*dy) < (BALL_R*BALL_R)) {
					normal = (ball_new.x < bar.pos) ? 225 : 315;
					print("Bounce on corner of bar.\n\r");
					break;
				}
				/* Bounce on flat edge of bar */
				if( ((ball_new.y+BALL_R) >= (BZ_H-BAR_OFFSET_Y-BAR_H))
					&& (abs(ball_new.x-bar.pos) <= BAR_W/2) ) {
					normal = 270;
					print("Bounce on flat bar.\n\r");
					break;
				}
			}

			sem_post(&sem_check_collision);
			for(int i  = 0; i < NB_COLUMNS; i++) {
				int msgid = msgget (i+1, IPC_CREAT);
				msgrcv(msgid, &collision_columns[i], sizeof(Collision_result), 0,0 );

			}

			ball_new.angle = check_angle(2*normal - (int) ball.angle + 180);
			ball_new.vel = ball.vel;
			safe_printf("New angle is: %d; New pos: %d,%d\n\r", ball_new.angle, ball_new.x, ball_new.y);
			ball = ball_new;
		}

		/* Prepapre message to be sent */
		model_state.ball_posx = ball.x;
		model_state.ball_posy = ball.y;
		model_state.ball_vel = ball.vel;
		model_state.bar_pos = bar.pos;
		model_state.game_state = game_state;

		/* Make sure the refresh rate is constant */
		actual_time = GET_MS;
		int wait_time = UPDATE_MS - (int) actual_time + (int) last_sent;
		safe_printf("Wait time: %d\n\r", wait_time);
		sleep(max(wait_time,0));
		last_sent = GET_MS;

		XMbox_WriteBlocking(&mbx_display, (u32*)&model_state, sizeof(model_state));
	}
}

void init_game() {
	game_state = WAITING;

	pthread_mutex_lock(&bar.mtx);
	bar.pos = BZ_W / 2;
	bar.speed = 0;
	bar.jump = NONE;
	bar.last_change = 0;
	pthread_mutex_unlock(&bar.mtx);

	pthread_mutex_lock(&ball.mtx);
	ball.x = BZ_W / 2;
	ball.y = BZ_H - BAR_OFFSET_Y - BAR_H - BALL_R - 80;
	ball.vel = BALL_DEF_SPEED;
	ball.angle = BALL_DEF_ANGLE;
	pthread_mutex_unlock(&ball.mtx);
}

void* main_prog(void *arg) {
    XMutex_Config* configPtr_mutex;
    XMbox_Config* configPtr_mailbox;
    pthread_attr_t attr;
    struct sched_param sched_par;
    int ret;

    init_game();

    print("[INFO uB1] \t Starting configuration\r\n");

    /* Initialize semaphores */
    sem_init(&sem_debounce, 1, 0);
    sem_init(&sem_check_collision, 1, 0);
    sem_init(&sem_arbitration_done, 1, 0);

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


	/* Column management threads, priority 1 */
    sched_par.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &sched_par);
    for(int i = 0; i < NB_COLUMNS; i++) {
        int *arg = malloc(sizeof(arg));
        if (arg == NULL) {
            safe_printf("Couldn't allocate memory for thread column.\r\n");
            free(arg);
            return (void*) XST_FAILURE;
        }
        *arg = i; // pass column index as argument
        pthread_create(&tid_col[i], &attr, (void*)thread_column, arg);
    }

    /* Bar management thread, priority 1 */
    pthread_attr_init(&attr);
    sched_par.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &sched_par);
    ret = pthread_create (&tid_bar, &attr, (void*)thread_buttons, NULL);
    if (ret != 0) {
        safe_printf("[ERROR uB1]\t (%d) launching thread_bar\r\n", ret);
        return (void*) XST_FAILURE;
    }
    else
        safe_printf("[INFO uB1] \t thread_bar launched with ID %d \r\n", tid_bar);

    /* Ball management thread, priority 1 */
    pthread_attr_init (&attr);
    sched_par.sched_priority = 1;
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

int main (void) {
    print("[INFO uB1] \t Entering main()\r\n");
    xilkernel_init();
    xmk_add_static_thread(main_prog,0);
    xilkernel_start();
    xilkernel_main ();

    //Control does not reach here
}
