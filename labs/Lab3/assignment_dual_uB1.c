 /***************************************************************************************
 *
 * Title:       EE4214 Lab3_3
 * File:        assignment_dual_uB1.c
 * Date:        2017-03-02
 * Author:      Paul-Edouard Sarlin (A0153124U)
 *
 ***************************************************************************************/

#include "xmk.h"
#include "xparameters.h"
#include "xil_types.h"
#include "xil_printf.h"
#include "xmutex.h"
#include "xmbox.h"
#include <pthread.h>
#include <errno.h>
#include <sys/init.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <sys/timer.h>

#include <stdio.h>
#include <stdlib.h>

#define BLUE        0x000000ff

#define safe_printf(fmt,args...) { XMutex_Lock(&Mutex, 0); \
                                   xil_printf(fmt, ##args); fflush(stdout); \
                                   XMutex_Unlock(&Mutex, 0); }

// Mailbox
#define MY_CPU_ID XPAR_CPU_ID
#define MBOX_DEVICE_ID        XPAR_MBOX_0_DEVICE_ID
static XMbox Mbox;

// Hardware Mutex
#define MUTEX_DEVICE_ID XPAR_MUTEX_0_IF_1_DEVICE_ID
#define MUTEX_NUM 0
XMutex Mutex;

pthread_t tid_ball;

typedef struct {
    u16 x,y,d;
    u32 c;
} Msg_ball;

void* main_prog();


void* thread_ball() {
    Msg_ball msg;
    msg.x = 240;
    msg.y = 320;
    msg.d = 90;
    msg.c = BLUE;

    while(1) {
        XMbox_WriteBlocking(&Mbox, (u32*)&msg, sizeof(msg));
        sleep(500);
    }
}

int main (void) {
    print("-- Entering main() --\r\n");
    xilkernel_init();
    xmk_add_static_thread(main_prog,0);
    xilkernel_start();
    //Start Xilkernel
    xilkernel_main ();

    //Control does not reach here
}

void* main_prog() {
    XMbox_Config* ConfigPtr_mailbox;
    XMutex_Config* ConfigPtr_mutex;
    int ret;
    print("-- Entering main() --\r\n");

    // Configure the mailbox
    ConfigPtr_mailbox = XMbox_LookupConfig(MBOX_DEVICE_ID);
    if (ConfigPtr_mailbox == (XMbox_Config *)NULL) {
        print("-- Error configuring Mbox uB1 Sender--\r\n");
        return XST_FAILURE;
    }
    ret = XMbox_CfgInitialize(&Mbox, ConfigPtr_mailbox, ConfigPtr_mailbox->BaseAddress);
    if (ret != XST_SUCCESS) {
        print("-- Error initializing Mbox uB1 Sender--\r\n");
        return XST_FAILURE;
    }

    // Configure the HW Mutex
    ConfigPtr_mutex = XMutex_LookupConfig(MUTEX_DEVICE_ID);
    if (ConfigPtr_mutex == (XMutex_Config *)NULL){

        print("B1-- ERROR  init HW mutex...\r\n");
        return XST_FAILURE;
    }
    ret = XMutex_CfgInitialize(&Mutex, ConfigPtr_mutex, ConfigPtr_mutex->BaseAddress);
    if (ret != XST_SUCCESS){
        print("B1-- ERROR  init HW mutex...\r\n");
        return XST_FAILURE;
    }

    // Start thread ball
    ret = pthread_create (&tid_ball, NULL, (void*)thread_ball, NULL);
    if (ret != 0) {
        safe_printf ("-- ERROR (%d) launching thread_ball...\r\n", ret);
    }
    else {
        safe_printf ("Thread ball launched with ID %d \r\n",tid_ball);
    }

    return 0;
}

