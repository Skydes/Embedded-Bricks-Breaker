 /***************************************************************************************
 *
 * Title:       EE4214 Lab3_1
 * File:        ball_3B.c
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
pthread_t tid1;
static int MAX_ITERATION=10;
volatile int taskrunning;

void ball()
{
    int i=0;
    for (i=0; i<MAX_ITERATION;i++){
        xil_printf(" Microblaze 1 : thread to control ball !! \r\n");
    }
}


void* thread_func_1 () {
    while(1) {
        XMutex_Lock(&Mutex, 0);
        ball();
        XMutex_Unlock(&Mutex, 0);
        sleep(200);
    }
}

void main_prog(void);
int main (void) {
    print("-- Entering main() uB1 --\r\n");
    xilkernel_init();
    xmk_add_static_thread(main_prog,0);
    xilkernel_start();
    //Start Xilkernel
    xilkernel_main ();

    //Control does not reach here
}

void main_prog(void) {   // This thread is statically created (as configured in the kernel configuration) and has priority 0 (This is the highest possible)


    print("-- Entering main() uB1 --\r\n");
    XStatus Status;
    int ret;

    // configure the HW Mutex
    XMutex_Config*ConfigPtr;
    ConfigPtr = XMutex_LookupConfig(MUTEX_DEVICE_ID);
    if (ConfigPtr == (XMutex_Config *)NULL){

        ("B1-- ERROR  init HW mutex...\r\n");
    }

    // initialize the HW Mutex    
    Status = XMutex_CfgInitialize(&Mutex, ConfigPtr, ConfigPtr->BaseAddress);
    if (Status != XST_SUCCESS){

        xil_printf ("B1-- ERROR  init HW mutex...\r\n");
    }

    // configure thread
    print("--Initialized -- uB1 \r\n");
    ret = pthread_create (&tid1, NULL, (void*)thread_func_1, NULL);
    if (ret != 0) {
        xil_printf ("-- ERROR (%d) launching thread_func_1...\r\n", ret);
    }
    else {
        xil_printf ("Thread 1 launched with ID %d \r\n",tid1);
    }
}

