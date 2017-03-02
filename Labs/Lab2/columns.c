/***************************************************************************************
 *
 * Title:       EE4214 Lab2_1
 * File:        columns.c
 * Date:        2017-03-02
 * Author:      Paul-Edouard Sarlin (A0153124U)
 * Comment:     safe_printf is used to ensure that only one thread prints at one time.
 *              do_sthing_mutex can also be used to ensure that only one thread
 *              runs do_something at one time.
 *
 ***************************************************************************************/

#include "xmk.h"
#include "sys/init.h"
#include <xparameters.h>
#include <pthread.h>
#include <errno.h>
#include <stdio.h>

struct sched_param sched_par;
pthread_attr_t attr;
pthread_t tid1, tid2, tid3, tid4;

pthread_mutex_t do_sthing_mutex, printf_mutex;

#define safe_printf(fmt,args...) { pthread_mutex_lock(&printf_mutex); \
        xil_printf(fmt, ##args); fflush(stdout); \
        pthread_mutex_unlock(&printf_mutex); }

// ---------------------------------------

volatile int taskrunning;

void do_something(int max, int ID) {
    int i,j;
    //pthread_mutex_lock(&do_sthing_mutex);
    for(i=0; i< max; i++) {
        if (taskrunning != ID) {
            taskrunning = ID;
            safe_printf ("\r\n Column changed to : %d\r\n", ID);
        }
        safe_printf ("Microblaze 0: Column %d, i %d\r\n", ID, i);
        for(j=0; j< 0xffff; j++) {
            ;
        }
    }
    //pthread_mutex_unlock(&do_sthing_mutex);
}


void* thread_func_1 () {
    while(1) {
        do_something(0x0004, 1);
        sleep(200);
    }
}


void* thread_func_2 () {
    while(1) {
        do_something(0x000f, 2);
        sleep(100);
    }
}


void* thread_func_3 () {
    while(1) {
        do_something(0x000f, 3);
        sleep(10);
    }
}


void* thread_func_4 () {
    while(1) {
        do_something(0x000f, 4);
        sleep(10);
    }
}

void* main_prog(void *arg);

int main (void) {


    print("-- Entering main() uB0--\r\n");
    xilkernel_init();
    xmk_add_static_thread(main_prog,0);
    xilkernel_start();
    //Start Xilkernel
    xilkernel_main ();

    //Control does not reach here
}

void* main_prog(void *arg) {

    int ret;

    print("-- Entering main_prog() uB0--\r\n");

    pthread_attr_init (&attr);

    // initialize the SW Mutex
    pthread_mutex_init (&do_sthing_mutex, NULL);
    pthread_mutex_init(&printf_mutex, NULL);

    print("--Initialized -- uB0 \r\n");
    // Priority 1 for thread 1
    sched_par.sched_priority = 1;
    pthread_attr_setschedparam(&attr,&sched_par);
    print("--Priority Set uB0 --\r\n");
    //start thread 1
    ret = pthread_create (&tid1, &attr, (void*)thread_func_1, NULL);
    print("--returned --\r\n");
    if (ret != 0) {
        xil_printf ("-- ERROR (%d) launching thread_func_1...\r\n", ret);
    }
    else {
        xil_printf ("Thread 1 launched with ID %d \r\n",tid1);
    }


    // Priority 2 for thread 2
    sched_par.sched_priority = 2;
    pthread_attr_setschedparam(&attr,&sched_par);

    //start thread 2
    ret = pthread_create (&tid2, &attr, (void*)thread_func_2, NULL);
    if (ret != 0) {
        xil_printf ("-- ERROR (%d) launching thread_func_2...\r\n", ret);
    }
    else {
        xil_printf ("Thread 2 launched with ID %d \r\n",tid2);
    }


    // Priority 3 for thread 3
    sched_par.sched_priority = 3;
    pthread_attr_setschedparam(&attr,&sched_par);

    //start thread 3
    ret = pthread_create (&tid3, &attr, (void*)thread_func_3, NULL);
    if (ret != 0) {
        xil_printf ("-- ERROR (%d) launching thread_func_3...\r\n", ret);
    }
    else {
        xil_printf ("Thread 3 launched with ID %d \r\n",tid3);
    }


    // Priority 4 for thread 4
    sched_par.sched_priority = 4;
    pthread_attr_setschedparam(&attr,&sched_par);

    //start thread 4
    ret = pthread_create (&tid4, &attr, (void*)thread_func_4, NULL);
    if (ret != 0) {
        xil_printf ("-- ERROR (%d) launching thread_func_4...\r\n", ret);
    }
    else {
        xil_printf ("Thread 4 launched with ID %d \r\n",tid4);
    }

    return 0;
}

