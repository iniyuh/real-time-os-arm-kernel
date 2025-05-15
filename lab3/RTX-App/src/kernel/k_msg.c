/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTX LAB  
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *---------------------------------------------------------------------------
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright
 *    notice and the following disclaimer.
 *
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS AND CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *---------------------------------------------------------------------------*/
 

/**************************************************************************//**
 * @file        k_msg.c
 * @brief       kernel message passing routines          
 * @version     V1.2021.06
 * @authors     Yiqing Huang
 * @date        2021 JUN
 *****************************************************************************/

#include "k_inc.h"
#include "k_rtx.h"
//#include "k_msg.h"

/*
 *===========================================================================
 *                       WAITING LIST FUNCTIONS
 *===========================================================================
 */
 // Helper function for finding priority level from queue
 /*CREATING MESSAGE_SCHEDULER*/
TCB* message_scheduler(U32 mailbox_tid){
    // Searches each priority level for a TCB
    return peak_head_waiting_list(mailbox_tid);
}

U32 get_index_waiting_list(U32 priority){
    return (priority - QUEUE_OFFSET);
}

//return 0 for fail, 1 for success
uint8_t insert_back_waiting_list(task_t tid, U32 prio, TCB *new_tcb){
    U32 index = get_index_waiting_list(prio);

    new_tcb->waiting_list_tid = tid;

    if (!g_tcbs[tid].mailbox_exists)
    {
        //cant add if there is no mailbox
        return 0;
    }

    // If this is the first TCB added to the queue
    if(g_tcbs[tid].mailbox->waiting_list[index].head == NULL){
        g_tcbs[tid].mailbox->waiting_list[index].head = new_tcb;
        g_tcbs[tid].mailbox->waiting_list[index].tail = new_tcb;
    }else{
        new_tcb->prev = g_tcbs[tid].mailbox->waiting_list[index].tail;
        new_tcb->next = NULL;

        g_tcbs[tid].mailbox->waiting_list[index].tail->next = new_tcb;
        g_tcbs[tid].mailbox->waiting_list[index].tail = new_tcb;
    }
    return 1;
}

TCB *pop_tid_waiting_list(task_t tid_mailbox, task_t tid_to_pop, U32 prio){
    U32 index = get_index_waiting_list(prio);
    
    // If queue is empty
    if(g_tcbs[tid_mailbox].mailbox->waiting_list[index].head == NULL){
        return NULL;
    }

    // Temp TCB
    TCB * temp = g_tcbs[tid_mailbox].mailbox->waiting_list[index].head;

    while(temp != NULL){
        // Check if the current tid is the one we are looking for
        if(temp->tid == tid_to_pop){
            // Check if head
            if(temp == g_tcbs[tid_mailbox].mailbox->waiting_list[index].head){
                return pop_head_waiting_list(tid_mailbox, prio);
            }

            // Check if tail
            if(temp == g_tcbs[tid_mailbox].mailbox->waiting_list[index].tail){
                temp->prev->next = NULL;
                g_tcbs[tid_mailbox].mailbox->waiting_list[index].tail = temp->prev;

                temp->next = NULL;
                temp->prev = NULL;

                temp->waiting_list_tid = 255;

                return temp;
            }

            // if not head or tail
            temp->prev->next = temp->next;
            temp->next->prev = temp->prev;

            temp->next = NULL;
            temp->prev = NULL;

            temp->waiting_list_tid = 255;

            return temp;
        }
        
        temp = temp->next;
    }

    return NULL;
}

TCB* peak_head_waiting_list(task_t tid)
{
    for(int prio = 0; prio < NUM_PRIO; ++prio)
    {
        if (g_tcbs[tid].mailbox->waiting_list[prio].head)
        {
            return g_tcbs[tid].mailbox->waiting_list[prio].head;
        }
    }
    return NULL;
}

TCB *pop_head_waiting_list(task_t tid, U32 prio){
    U32 index = get_index_waiting_list(prio);

    // If queue is empty
    if(g_tcbs[tid].mailbox->waiting_list[index].head == NULL){
        return NULL;
    }

    // Temp TCB
    TCB * temp = g_tcbs[tid].mailbox->waiting_list[index].head;

    // If only tcb in queue
    if(g_tcbs[tid].mailbox->waiting_list[index].head == g_tcbs[tid].mailbox->waiting_list[index].tail){
        g_tcbs[tid].mailbox->waiting_list[index].head = NULL;
        g_tcbs[tid].mailbox->waiting_list[index].tail = NULL;
    }else{
        g_tcbs[tid].mailbox->waiting_list[index].head = g_tcbs[tid].mailbox->waiting_list[index].head->next;
        g_tcbs[tid].mailbox->waiting_list[index].head->prev = NULL;

        temp->next = NULL;
        temp->prev = NULL;
    }

    temp->waiting_list_tid = 255;

    return temp;
}

U8 is_waiting_list_empty(task_t tid){
    for(size_t i = 0; i < NUM_PRIO; i++){
        if(g_tcbs[tid].mailbox->waiting_list[i].head != NULL){
            return 0;
        }
    }
    return 1;
}

/*
 *===========================================================================
 *                      END WAITING LIST FUNCTIONS
 *===========================================================================
 */
 
/*
 *===========================================================================
 *                        RING BUFFER FUNCTIONS
 *===========================================================================
 */

RING_BUF ringBuffs[MAX_TASKS]; //-1 bc of null task

int init_ring_buffer(size_t size){
    gp_current_task->mailbox = &ringBuffs[gp_current_task->tid-1];
    gp_current_task->mailbox->ring_buf = k_mpool_alloc(MPID_IRAM2, size);

    if(gp_current_task->mailbox->ring_buf == NULL){
        return RTX_ERR;
    }

    for (int i = 0; i < NUM_PRIO; i++) {
        gp_current_task->mailbox->waiting_list[i].head = NULL;
        gp_current_task->mailbox->waiting_list[i].tail = NULL;
    }

    gp_current_task->mailbox->total_size = size;
    gp_current_task->mailbox->size_left = size;
    gp_current_task->mailbox->head = -1;
    gp_current_task->mailbox->tail = 0;
    gp_current_task->mailbox_exists = 1;

    return RTX_OK;
}

// Inserts message into queue
int write_to_queue(task_t tid, const void *msg){
    RTX_MSG_HDR *p = (RTX_MSG_HDR *) msg;
    char* data = (char*) msg;
    
    // Errors if not enough space in queue
    if(p->length > (g_tcbs[tid].mailbox->size_left)){
        //should never get here?
        return RTX_ERR;
    }

    for(size_t i = 0; i < p->length; i++){
        g_tcbs[tid].mailbox->head = (g_tcbs[tid].mailbox->head + 1) % g_tcbs[tid].mailbox->total_size;
        g_tcbs[tid].mailbox->ring_buf[g_tcbs[tid].mailbox->head] = data[i];
        g_tcbs[tid].mailbox->size_left--;
    }

    return RTX_OK;
}

void ece_memcpy(char* dest, char* source, uint16_t size)
{
	for(uint16_t a = 0; a < size; ++a)
	{
		dest[a] = source[a];
	}
}

// Returns next message from queue
void read_from_queue(char* buff, int length, task_t tid){
    // Fill rest of buf
    for(size_t i = 0; i < length; i++){
        buff[i] = g_tcbs[tid].mailbox->ring_buf[g_tcbs[tid].mailbox->tail];
        g_tcbs[tid].mailbox->tail = (g_tcbs[tid].mailbox->tail + 1) % g_tcbs[tid].mailbox->total_size;
        g_tcbs[tid].mailbox->size_left++;
    }
}

int get_size_left_tid(task_t tid){
    return g_tcbs[tid].mailbox->size_left;
}

U8 is_empty(task_t tid){
    return (g_tcbs[tid].mailbox->size_left == g_tcbs[tid].mailbox->total_size) ? 1 : 0;
}

U8 is_full(task_t tid){
    return (g_tcbs[tid].mailbox->size_left == 0) ? 1 : 0;
}

// int get_length_of_next_msg(void){
//     // Returns ERR if queue is empty
//     if(gp_current_task->mailbox->size_left == gp_current_task->mailbox->total_size){
//         return RTX_ERR;
//     }

//     RTX_MSG_HDR tmp;
//     char* p = (char*) &tmp;
//     int tail = gp_current_task->mailbox->tail;

//     // Extract Length
//     for(size_t i = 0; i < MSG_HDR_SIZE; i++){
//         p[i] = gp_current_task->mailbox->ring_buf[tail];
//         tail = (tail + 1) % gp_current_task->mailbox->total_size;
//     }

//     return tmp.length;
// }

int get_length_of_next_msg(task_t tid){
    // Returns ERR if queue is empty
    TCB* curr_tcb = &g_tcbs[tid];
    if(curr_tcb->mailbox->size_left == curr_tcb->mailbox->total_size){
        return RTX_ERR;
    }

    RTX_MSG_HDR tmp;
    char* p = (char*) &tmp;
    int tail = curr_tcb->mailbox->tail;

    // Extract Length
    for(size_t i = 0; i < MSG_HDR_SIZE; i++){
        p[i] = curr_tcb->mailbox->ring_buf[tail];
        tail = (tail + 1) % curr_tcb->mailbox->total_size;
    }

    return tmp.length;
}

 /*
 *===========================================================================
 *                     END OF RING BUFFER FUNCTIONS
 *===========================================================================
 */

int k_mbx_create(size_t size) {
#ifdef DEBUG_0
    printf("k_mbx_create: size = %u\r\n", size);
#endif /* DEBUG_0 */
    if (size < MIN_MSG_SIZE) {
        errno = EINVAL;
        return RTX_ERR;
    }
    if (gp_current_task->mailbox_exists) {
        errno = EEXIST;
        return RTX_ERR;
    }
    if(init_ring_buffer(size) == RTX_ERR){
        errno = ENOMEM;
        return RTX_ERR;
    }

    return (int) gp_current_task->tid;
}

void contextSwitchTcb(TCB* tcb)
{
    //context switch
		
    TCB* p_tcb_old = gp_current_task;
    gp_current_task = tcb;

    gp_current_task->state = RUNNING;   // change state of the to-be-switched-in  tcb
    p_tcb_old->state = READY;           // change state of the to-be-switched-out tcb
    p_tcb_old->u_sp = __get_PSP();  // TODO: DOUBLE CHECK THIS
    k_tsk_switch(p_tcb_old);            // switch kernel stacks
}

int k_send_msg(task_t receiver_tid, const void *buf) {
 
#ifdef DEBUG_0
    printf("k_send_msg: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif

    //check for valid receiver tid?
    //printf("testing receiver_task get tid: [%d], prio:[%d], state:[%d]\r\n", receiver_task->tid, receiver_task->prio, receiver_task->state);
    // prioqueue for each mailbox
    struct rtx_msg_hdr* input =  (struct rtx_msg_hdr*) buf;
    if (input == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    if (!(receiver_tid < MAX_TASKS || receiver_tid == TID_UART_MBX) || (input->length < MIN_MSG_SIZE)){
        errno = EINVAL;
        return RTX_ERR;
    }
    TCB *receiver_task = &g_tcbs[receiver_tid];
    if (receiver_task->mailbox_exists == 0) {
        errno = ENOENT;
        return RTX_ERR;
    }
    if (receiver_task->mailbox->total_size < input->length) {
        errno = EMSGSIZE;
        return RTX_ERR;
    }
    switch (receiver_tid) {
        case TID_CON:
        {
            //special cond?
            if(input->type != DISPLAY)
            {
                //ignore messages
                return 0;
            }
            break;
        }
        case TID_KCD: 
        {   
            if (!(input->type == KCD_REG || input->type == KEY_IN)) {
                //ignore messages
                return 0;
            }
            break;
            // check if msg is of type KCD_REG or KEY_IN then run, else ignore
        }
    }

    uint8_t wasBlocked = 0;
    if (receiver_task->mailbox->size_left < input->length)
    {
        wasBlocked = 1;
        
        //transfer sending_task from ready-prio-queue to task-specific-prio-queue
        if (insert_back_waiting_list(receiver_tid, gp_current_task->prio, gp_current_task))
        {
            //change state of sending_task
            gp_current_task->state = BLK_SEND;
            gp_current_task->waiting_list_tid = receiver_tid;
        }
        else
        {
            //receiver's mailbox DNE. SMTH is wrong. Shouldnt ever get here
            return RTX_ERR;
        }
        
        do { //MSG DOES NOT FIT IN MAILBOX (GOES INTO TASK-WAITING PRIO-QUEUE)
            //New scheduling decision
            // if (wasBlocked) {
            //     contextSwitchTcb(&g_tcbs[gp_current_task->waiting_list_tid]);
            // }
            
            if (wasBlocked == 1)
            {
                k_tsk_run_new();
            } else {
                contextSwitchTcb(&g_tcbs[gp_current_task->waiting_list_tid]);
            }
            if (wasBlocked == 1)
            {
                wasBlocked = 2;
            }
        } while (receiver_task->mailbox->size_left < input->length );

    }
    //MSG FITS IN MAILBOX         
    //write to queue using function Ahsan will implement where you can pass in receiver function TID
    write_to_queue(receiver_tid, buf);
    if(receiver_task->state == BLK_RECV)
    {
        //state changes from BLK_RECV to READY
        receiver_task->state = READY;
        insert_front_queue(get_index_waiting_list(gp_current_task->prio), gp_current_task);
        if (receiver_tid == TID_KCD || receiver_tid == TID_CON)
        {
            insert_front_queue(get_index_waiting_list(receiver_task->prio), receiver_task);
        }
        else
        {
            insert_back_queue(get_index_waiting_list(receiver_task->prio), receiver_task);
        }
        k_tsk_run_new();
    }
    else if (wasBlocked) //this is in an else if block since you cant have a receiver and sender both in the blocked state. But if the recv was blocked we dont want to run this code
    {
        //remove from waiting list
        U8 waiting_list_tid = gp_current_task->waiting_list_tid; // if it's popped, need to keep track of waiting list tid to know where to go back to after removed
        pop_tid_waiting_list(gp_current_task->waiting_list_tid, gp_current_task->tid, gp_current_task->prio);
        //go back to the recv'ing function that is unblocking this send call
        contextSwitchTcb(&g_tcbs[waiting_list_tid]);
    }
    return 0;
}

int k_send_msg_nb(task_t receiver_tid, const void *buf) {
 
#ifdef DEBUG_0
    printf("k_send_msg_nb: receiver_tid = %d, buf=0x%x\r\n", receiver_tid, buf);
#endif

    struct rtx_msg_hdr* input =  (struct rtx_msg_hdr*) buf;
    if (input == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    if (!(receiver_tid < MAX_TASKS || receiver_tid == TID_UART_MBX) || (input->length < MIN_MSG_SIZE)){
        errno = EINVAL;
        return RTX_ERR;
    }
    TCB *receiver_task = &g_tcbs[receiver_tid]; //assuming g_tcbs is now extern variable
		//MIGHT NEED TO DECLARE receiver_task VOLATILE 
    if (receiver_task->mailbox_exists == 0) {
        errno = ENOENT;
        return RTX_ERR;
    }
    if (receiver_task->mailbox->total_size < input->length){
        errno = EMSGSIZE;
        return RTX_ERR;
    }
    switch (receiver_tid) {
        case TID_CON:
        {
            //special cond?
            if(input->type != DISPLAY)
            {
                //ignore messages
                return 0;
            }
            break;
        }
        case TID_KCD: 
        {   
            if (!(input->type == KCD_REG || input->type == KEY_IN)) {
                //ignore messages
                return 0;
            }
            break;
            // check if msg is of type KCD_REG or KEY_IN then run, else ignore
        }
    }

    TCB* tmpVar = peak_head_waiting_list(receiver_tid);
    if (receiver_task->mailbox->size_left < input->length || (tmpVar && gp_current_task->prio >= tmpVar->prio)) {
        errno = ENOSPC;
        return RTX_ERR;
    } else {
        write_to_queue(receiver_tid, buf);
        if(receiver_task->state == BLK_RECV){
            //receiving task gets added to back of ready-prio-queue
            insert_front_queue(get_index_waiting_list(gp_current_task->prio), gp_current_task);
            if (receiver_tid == TID_KCD || receiver_tid == TID_CON)
            {
                insert_front_queue(get_index_waiting_list(receiver_task->prio), receiver_task);
            }
            else
            {
                insert_back_queue(get_index_waiting_list(receiver_task->prio), receiver_task);
            }
            //state changes from BLK_RECV to READY
            receiver_task->state = READY;
            k_tsk_run_new();
        }
        //write to queue using function Ahsan will implement where you can pass in receiver function TID
    }
    return 0;
}

int k_recv_msg(void *buf, size_t len) {
 
#ifdef DEBUG_0
    printf("k_recv_msg: buf=0x%x, len=%d\r\n", buf, len);
#endif

    if (buf == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    if (!gp_current_task->mailbox_exists) {
        errno = ENOENT;
        return RTX_ERR;
    }
    
    if ( is_empty(gp_current_task->tid) ) { //MAILBOX IS EMPTY (confirm this!)
         //change state of current_receiving_task
        gp_current_task->state = BLK_RECV;
        //New scheduling decision
        k_tsk_run_new();
    }

    int message = get_length_of_next_msg(gp_current_task->tid);
    if (len < message) {
        errno = ENOSPC;
        return RTX_ERR;
    }
        #ifdef DEBUG_0
    printf("k_recv_msg_nb: msg_len=%d\r\n", message);
#endif /* DEBUG_0 */

    read_from_queue((char*) buf, message, gp_current_task->tid);
    
    //If there exists a non-empty task-waiting prio-queue -> transfer as many messages from task-waiting prio-queue to mailbox (in prio order) → fix state of tasks / ready prio-queue accordingly
    if(!is_waiting_list_empty(gp_current_task->tid)){
        TCB *peaked_tcb;
        TCB *new_peaked_tcb = peak_head_waiting_list(gp_current_task->tid);
        do
        {
            peaked_tcb = new_peaked_tcb;
            contextSwitchTcb(peaked_tcb);
            new_peaked_tcb = peak_head_waiting_list(gp_current_task->tid);

            if (new_peaked_tcb == peaked_tcb)
            {
                //could not add it
                break;
            }
            //Update its waiting_list_tid
            peaked_tcb->waiting_list_tid = 255;
            if (peaked_tcb != NULL) {
                insert_back_queue(get_index_waiting_list(peaked_tcb->prio), peaked_tcb);
            }
        } while (new_peaked_tcb != NULL);
        
        //would only get a higher prio task if weve unblocked something in the waiting queue
        insert_front_queue(get_index_waiting_list(gp_current_task->prio), gp_current_task);
        //New scheduling decision
        k_tsk_run_new();
    }
    return 0;
}

int k_recv_msg_nb(void *buf, size_t len) {
#ifdef DEBUG_0
    printf("k_recv_msg_nb: buf=0x%x, len=%d\r\n", buf, len);
#endif

    if (buf == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    if (!gp_current_task->mailbox_exists) {
        errno = ENOENT;
        return RTX_ERR;
    }

    if (is_empty(gp_current_task->tid)){
        errno = ENOMSG;
        return RTX_ERR;
    }
    
    int message = get_length_of_next_msg(gp_current_task->tid);

    if (len < message) {
        errno = ENOSPC;
        return RTX_ERR;
    }
    #ifdef DEBUG_0
    printf("k_recv_msg_nb: msg_len=%d\r\n", message);
#endif /* DEBUG_0 */

    read_from_queue((char* )buf, message, gp_current_task->tid);

    //If there exists a non-empty task-waiting prio-queue -> transfer as many messages from task-waiting prio-queue to mailbox (in prio order) → fix state of tasks / ready prio-queue accordingly
    if(!is_waiting_list_empty(gp_current_task->tid)){
        TCB *peaked_tcb;
        TCB *new_peaked_tcb = peak_head_waiting_list(gp_current_task->tid);
        do
        {
            peaked_tcb = new_peaked_tcb;
            contextSwitchTcb(peaked_tcb);
            new_peaked_tcb = peak_head_waiting_list(gp_current_task->tid);

            if (new_peaked_tcb == peaked_tcb)
            {
                //could not add it
                break;
            }
            //Update its waiting_list_tid
            peaked_tcb->waiting_list_tid = 255;
            if (peaked_tcb != NULL) {
                insert_back_queue(get_index_waiting_list(peaked_tcb->prio), peaked_tcb);
            }
        } while (new_peaked_tcb != NULL);
        
        //would only get a higher prio task if weve unblocked something in the waiting queue
        insert_front_queue(get_index_waiting_list(gp_current_task->prio), gp_current_task);
        //New scheduling decision
        k_tsk_run_new();
    }
    return 0;
}

int k_uart_recv_nb(void *buf, size_t len) {
    //printf("ENTER UART RECV NB \n");
    TCB* uart_tcb = &g_tcbs[TID_UART_MBX];
    if (buf == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    //printf("a\n");
    if (!uart_tcb->mailbox_exists) {
        errno = ENOENT;
        return RTX_ERR;
    }
		//printf("b\n");
    if (is_empty(TID_UART_MBX)){
        //printf("\t\t\tEMPTY\n");
        errno = ENOMSG;
        return RTX_ERR;
    }
    //printf("c\n");
    int message = get_length_of_next_msg(TID_UART_MBX);
		//printf("d\n");
    if (len < message) {
        errno = ENOSPC;
        return RTX_ERR;
    }
#ifdef DEBUG_0
    printf("k_recv_msg_nb: msg_len=%d\r\n", message);
#endif /* DEBUG_0 */

    read_from_queue((char* )buf, message, TID_UART_MBX);

    //If there exists a non-empty task-waiting prio-queue -> transfer as many messages from task-waiting prio-queue to mailbox (in prio order) → fix state of tasks / ready prio-queue accordingly
    if(!is_waiting_list_empty(TID_UART_MBX)){
        TCB *peaked_tcb;
        TCB *new_peaked_tcb = peak_head_waiting_list(TID_UART_MBX);
        do
        {
            peaked_tcb = new_peaked_tcb;
            contextSwitchTcb(peaked_tcb);
            new_peaked_tcb = peak_head_waiting_list(TID_UART_MBX);

            if (new_peaked_tcb == peaked_tcb)
            {
                //could not add it
                break;
            }
            //Update its waiting_list_tid
            peaked_tcb->waiting_list_tid = 255;
            if (peaked_tcb != NULL) {
                insert_back_queue(get_index_waiting_list(peaked_tcb->prio), peaked_tcb);
            }
        } while (new_peaked_tcb != NULL);
        
        //would only get a higher prio task if weve unblocked something in the waiting queue
        insert_front_queue(get_index_waiting_list(gp_current_task->prio), gp_current_task);
        //New scheduling decision
        k_tsk_run_new();
    }
    return 0;
}


int k_mbx_ls(task_t *buf, size_t count) {
#ifdef DEBUG_0
    printf("k_mbx_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */

    if (buf == NULL || count == 0) {
        errno = EFAULT;
        return RTX_ERR;
    }

    int mailboxes = 0;
    int index = 0;

    while (index != MAX_TASKS && mailboxes < count) {
        if (g_tcbs[index].state != DORMANT && g_tcbs[index].tid == index && g_tcbs[index].mailbox_exists) {
            buf[mailboxes] = g_tcbs[index].tid;
            mailboxes++;
        }

        index++;
    }

    return mailboxes;
}

int k_mbx_get(task_t tid)
{
#ifdef DEBUG_0
    printf("k_mbx_get: tid=%u\r\n", tid);
#endif /* DEBUG_0 */
    if (tid >= MAX_TASKS) {
        errno = ENOENT;
        return RTX_ERR;
    }

    if (g_tcbs[tid].mailbox_exists){
        return g_tcbs[tid].mailbox->size_left;
    }

    errno = ENOENT;
    return RTX_ERR;
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

