#include "timer.h"

#define GET_MS			sys_xget_clock_ticks()*(SYSTMR_INTERVAL / SYSTMR_CLK_FREQ_KHZ)
#define	SLEEP_TIME		1 // ms

struct sched_param sched_par;
pthread_attr_t attr;

void* loop(void *arg);

Timer* timer_init(u32 delay_ms, void (*callback)()) {
	Timer *timer = (Timer *)malloc(sizeof(Timer));
	timer->state = T_STOPPED;
	timer->delay_ms = delay_ms;
	timer->callback = callback;

	sem_init(&timer->trigger, 1, 0);

	pthread_t tid;
	pthread_attr_init(&attr);
	sched_par.sched_priority = 1;
	pthread_attr_setschedparam(&attr, &sched_par);
	int ret = pthread_create(&tid, &attr, (void*)loop, timer);
	if (ret != 0)
		xil_printf("[ERROR]\t (%d) launching timer\r\n", ret);
	else
		xil_printf("[INFO] \t\t timer launched with ID %d \r\n", tid);

	return timer;
}

void timer_start(Timer *timer) {
	sem_post(&timer->trigger);
}

void timer_stop(Timer *timer) {
	timer->state = T_STOPPED;
}

bool is_finished(Timer *timer) {
	return timer->state == T_FINISHED;
}

void* loop(void *arg) {
	Timer *timer = (Timer *) arg;

	while(1) {
		switch(timer->state){
			case T_STOPPED:
				sem_wait(&timer->trigger);
				timer->start_time = GET_MS;
				timer->state = T_RUNNING;
			case T_RUNNING:
				if( (GET_MS - timer->start_time) > timer->delay_ms) {
					timer->state = T_FINISHED;
					(*(timer->callback))();
				}
				else
					sleep(SLEEP_TIME);
				break;
			case T_FINISHED:
				sleep(SLEEP_TIME);
				break;
		}
	}
}
