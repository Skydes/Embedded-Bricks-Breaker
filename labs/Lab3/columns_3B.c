 /***************************************************************************************
 *
 * Title:       EE4214 Lab3_1
 * File:        column_3B.c
 * Date:        2017-03-02
 * Author:      Paul-Edouard Sarlin (A0153124U)
 *
 ***************************************************************************************/

#include "xmk.h"
#include <xparameters.h>
#include <pthread.h>
#include <errno.h>
#include "xmutex.h"

// declare the Hardware Mutex
#define MUTEX_DEVICE_ID XPAR_MUTEX_0_IF_1_DEVICE_ID
#define MUTEX_NUM 0
XMutex Mutex;

struct sched_param sched_par;
pthread_attr_t attr;
pthread_t tid1, tid2, tid3, tid4;

// SW Mutex
pthread_mutex_t uart_mutex;

// ---------------------------------------

volatile int taskrunning;

void do_something(int max, int ID) {
    // int max : Number of outer loops.
    // int ID  : The number of the thread. This is used for debugging.

    int i,j;
    for(i=0; i< max; i++) {
        if (taskrunning != ID) {
            taskrunning = ID;
            XMutex_Lock(&Mutex, 0);
            xil_printf ("\r\n Column changed to : %d\r\n", ID);
            XMutex_Unlock(&Mutex, 0);
        }
        XMutex_Lock(&Mutex, 0);
        xil_printf ("Microblaze 0: Column %d, i %d\r\n", ID, i);
        XMutex_Unlock(&Mutex, 0);
        for(j=0; j< 0xffff; j++) {
            ;
        }
    }

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



void main_prog(void);

int main (void) {


    print("-- Entering main() uB0--\r\n");
    xilkernel_init();
    xmk_add_static_thread(main_prog,0);
    xilkernel_start();
    //Start Xilkernel
    xilkernel_main ();

    //Control does not reach here

}

void main_prog(void) {   // This thread is statically created (as configured in the kernel configuration) and has priority 0 (This is the highest possible)

    int ret;
    pthread_attr_init (&attr);
    XStatus Status;

    print("-- Entering main_prog() uB0--\r\n");

    // initialize SW Mutex
    ret = pthread_mutex_init (&uart_mutex, NULL);
    if (ret != 0) {
        xil_printf ("-- ERROR (%d) init uart_mutex...\r\n", ret);
    }

    // setup HW Mutex
    XMutex_Config*ConfigPtr;
    ConfigPtr = XMutex_LookupConfig(MUTEX_DEVICE_ID);
    XMutex_CfgInitialize(&Mutex, ConfigPtr, ConfigPtr->BaseAddress);

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

}

