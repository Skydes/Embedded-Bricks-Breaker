#include "xmk.h"
//#include "xgpio.h"
#include "xparameters.h"
#include "xmutex.h"
#include "xmbox.h"

#include <errno.h>
#include <pthread.h>
#include <sys/init.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/timer.h> // for sleep

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "config.h"

int main (void);
void* main_prog(void *arg);

static XMbox mbx_model;
XMutex mtx_hw;
pthread_t tid_ball;

void* thread_ball() {
	;
}

void* main_prog(void *arg) {
    XMutex_Config* configPtr_mutex;
    XMbox_Config* configPtr_mailbox;
    pthread_attr_t attr;
    struct sched_param sched_par;
    int ret;

    print("[INFO uB1] \t Starting configuration\r\n");
    /* Configure the HW Mutex */
    configPtr_mutex = XMutex_LookupConfig(MUTEX_DEVICE_ID);
    if (configPtr_mutex == (XMutex_Config *)NULL){
        print("[ERROR uB1]\t While configuring the Hardware Mutex\r\n");
        return XST_FAILURE;
    }
    ret = XMutex_CfgInitialize(&mtx_hw, configPtr_mutex, configPtr_mutex->BaseAddress);
    if (ret != XST_SUCCESS){
        print("[ERROR uB1]\t While initializing the Hardware Mutex\r\n");
        return XST_FAILURE;
    }

    /* Configure the mailbox */
    configPtr_mailbox = XMbox_LookupConfig(MBOX_DEVICE_ID);
    if (configPtr_mailbox == (XMbox_Config *)NULL) {
    	print("[ERROR uB1]\t While configuring the Mailbox\r\n");
        return XST_FAILURE;
    }
    ret = XMbox_CfgInitialize(&mbx_model, configPtr_mailbox, configPtr_mailbox->BaseAddress);
    if (ret != XST_SUCCESS) {
    	print("[ERROR uB1]\t While initializing the Mailbox\r\n");
        return XST_FAILURE;
    }

    /* Ball management thread, priority 1 */
    pthread_attr_init (&attr);
    sched_par.sched_priority = 1;
    pthread_attr_setschedparam(&attr, &sched_par);
    ret = pthread_create (&tid_ball, &attr, (void*)thread_ball, NULL);
    if (ret != 0)
        xil_printf("[ERROR uB1]\t (%d) launching thread_display\r\n", ret);
    else
        xil_printf("[INFO uB1] \t Thread_ball launched with ID %d \r\n", tid_ball);

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
