 /***************************************************************************************
 *
 * Title:       EE4214 Lab3_3
 * File:        assignment_dual_uB0.c
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

#define NB_COLUMNS      4
#define BLACK           0x00ffffff
#define COL_WIDTH       640/NB_COLUMNS

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

pthread_t tid_disp, tid_columns[NB_COLUMNS];

typedef struct {
    u16 x,y,w,l;
    u32 c;
} Msg_col;

typedef struct {
    u16 x,y,d;
    u32 c;
} Msg_ball;

void* main_prog();


void* display() {
    Msg_col msg_col;
    Msg_ball msg_ball;
    int i,msgid;
    while (1) {
        // Receive messages from columns
        for (i=1; i<=NB_COLUMNS; i++) {

            msgid = msgget (i, IPC_CREAT);
            if( msgid == -1 ) {
                xil_printf ("Column: Consumer -- ERROR while opening Message Queue. Errno: %d \r\n", errno);
                pthread_exit(&errno) ;
            }
            if( msgrcv( msgid, &msg_col, sizeof(msg_col), 0,0 ) != sizeof(msg_col) ) {
                xil_printf ("PRODCON: Consumer -- msgrcv of message(%d) ran into ERROR. Errno: %d. Halting...\r\n", i, errno);
                pthread_exit(&errno);
            }
            // print message
            safe_printf("\rDisplay received for column %u, x = %3u  y= %3u  "
                        "w = %3u  l = %3u  c = %d\r\n", i, msg_col.x,msg_col.y,msg_col.w,msg_col.l,msg_col.c);
        }

        // Receive message from ball
        msgid = msgget (5, IPC_CREAT);
        if( msgid == -1 ) {
            xil_printf ("Ball: Consumer -- ERROR while opening Message Queue. Errno: %d \r\n", errno);
            pthread_exit(&errno) ;
        }

        XMbox_ReadBlocking(&Mbox, (u32*)&msg_ball, sizeof(msg_ball));
        safe_printf("\rDisplay received for ball, x = %3d  y= %3d  d = %3d  c = %d\r\n", i, msg_ball.x,msg_ball.y,msg_ball.d,msg_ball.c);

    }
}


void* thread_column (void *arg) {
    u8 idx = *((int *) arg); // local context copy
    free(arg);

    Msg_col msg;
    msg.x = (idx-1)*COL_WIDTH;
    msg.y = 0;
    msg.w = COL_WIDTH;
    msg.l = idx*100;
    msg.c = BLACK;

    int msgid;
    msgid = msgget(idx, IPC_CREAT);

    while(1) {
        if( msgsnd(msgid, &msg, sizeof(msg), 0) < 0 ) { // blocking send
            safe_printf ("PRODCON: Producer -- msgsnd of message(%d) ran into ERROR. Errno: %d. Halting..\r\n", errno);
            pthread_exit(&errno);
        }
        else
            safe_printf("PRODCON: Producer %d done !\r\n", idx);
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

    /* One thread for each column */
    for (int i = 0; i < NB_COLUMNS; i++) {
        int *arg = malloc(sizeof(arg));
        if (arg == NULL) {
            safe_printf("Couldn't allocate memory for thread column.\r\n");
            continue;
        }
        *arg = i+1; // pass column index as argument
        ret = pthread_create(&tid_columns[i], NULL, (void*)thread_column, arg);
        if (ret != 0) {
            safe_printf ("-- ERROR (%d) launching thread for column %d...\r\n", ret, i+1);
        }
        else {
            safe_printf ("Thread for column %d launched with ID %d \r\n",i+1,tid_columns[i]);
        }
    }

    // Start thread display
    ret = pthread_create (&tid_disp, NULL, (void*)display, NULL);
    if (ret != 0) {
        safe_printf ("-- ERROR (%d) launching display...\r\n", ret);
    }
    else {
        safe_printf ("Display launched with ID %d \r\n",tid_disp);
    }

    return 0;
}

