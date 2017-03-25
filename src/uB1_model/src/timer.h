
#ifndef SRC_TIMER_H_
#define SRC_TIMER_H_


#include "xmk.h"
#include "xparameters.h"

#include "xil_types.h"

#include <pthread.h>
#include <sys/init.h>
#include <sys/timer.h> // for sleep

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


typedef enum Timer_state {T_STOPPED, T_RUNNING, T_FINISHED} Timer_state;
typedef struct {
	Timer_state state;
	u32 delay_ms;
	sem_t trigger;
	u32 start_time;
	void (*callback)();
} Timer;

Timer* timer_init(u32 delay_ms, void (*callback)());
void timer_start(Timer *timer);
void timer_stop(Timer *timer);
bool is_finished(Timer *timer);

#endif
