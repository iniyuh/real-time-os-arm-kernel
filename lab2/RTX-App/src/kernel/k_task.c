/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *                          All rights reserved.
 *
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
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_task.c
 * @brief       task management C file
 * @version     V1.2021.05
 * @authors     Yiqing Huang
 * @date        2021 MAY
 *
 * @attention   assumes NO HARDWARE INTERRUPTS
 * @details     The starter code shows one way of implementing context switching.
 *              The code only has minimal sanity check.
 *              There is no stack overflow check.
 *              The implementation assumes only three simple tasks and
 *              NO HARDWARE INTERRUPTS.
 *              The purpose is to show how context switch could be done
 *              under stated assumptions.
 *              These assumptions are not true in the required RTX Project!!!
 *              Understand the assumptions and the limitations of the code before
 *              using the code piece in your own project!!!
 *
 *****************************************************************************/


#include "k_inc.h"
#include "k_rtx.h"

/*
 *==========================================================================
 *                            GLOBAL VARIABLES
 *==========================================================================
 */

TCB             *gp_current_task = NULL;    // the current RUNNING task
TCB             g_tcbs[MAX_TASKS];          // an array of TCBs
U32             g_num_active_tasks = 0;     // number of non-dormant tasks

PRIO_Q priority_queue[4];

/*---------------------------------------------------------------------------
The memory map of the OS image may look like the following:
                   RAM1_END-->+---------------------------+ High Address
                              |                           |
                              |                           |
                              |       MPID_IRAM1          |
                              |   (for user space heap  ) |
                              |                           |
                 RAM1_START-->|---------------------------|
                              |                           |
                              |  unmanaged free space     |
                              |                           |
&Image$$RW_IRAM1$$ZI$$Limit-->|---------------------------|-----+-----
                              |         ......            |     ^
                              |---------------------------|     |
                              |      PROC_STACK_SIZE      |  OS Image
              g_p_stacks[2]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[1]-->|---------------------------|     |
                              |      PROC_STACK_SIZE      |     |
              g_p_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |                           |  OS Image
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                
    g_k_stacks[MAX_TASKS-1]-->|---------------------------|     |
                              |                           |     |
                              |     other kernel stacks   |     |                              
                              |---------------------------|     |
                              |      KERN_STACK_SIZE      |  OS Image
              g_k_stacks[2]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |                      
              g_k_stacks[1]-->|---------------------------|     |
                              |      KERN_STACK_SIZE      |     |
              g_k_stacks[0]-->|---------------------------|     |
                              |   other  global vars      |     |
                              |---------------------------|     |
                              |        TCBs               |  OS Image
                      g_tcbs->|---------------------------|     |
                              |        global vars        |     |
                              |---------------------------|     |
                              |                           |     |          
                              |                           |     |
                              |        Code + RO          |     |
                              |                           |     V
                 IRAM1_BASE-->+---------------------------+ Low Address
    
---------------------------------------------------------------------------*/ 


/*
 *===========================================================================
 *                           HELPER FUNCTIONS
 *===========================================================================
 */

TCB *getAvailableTCB () {
    for (int i = 1; i < MAX_TASKS; i++) {
        if (g_tcbs[i].state == DORMANT) {
            return &(g_tcbs[i]);
        }
    }

    return NULL;
}

// Helper function for finding priority level from queue
U32 get_index(U32 priority){
    return (priority - QUEUE_OFFSET);
}

/*
 *===========================================================================
 *                          QUEUE FUNCTIONS
 *===========================================================================
 */

void insert_front_queue(U32 index, TCB *new_tcb){
    // If this is the first TCB added to the queue
    if(priority_queue[index].head == NULL){
        priority_queue[index].head = new_tcb;
        priority_queue[index].tail = new_tcb;
    }else{
        new_tcb->next = priority_queue[index].head;
        new_tcb->prev = NULL;

        priority_queue[index].head->prev = new_tcb;
        priority_queue[index].head = new_tcb;
    }
}

void insert_back_queue(U32 index, TCB *new_tcb){
    // If this is the first TCB added to the queue
    if(priority_queue[index].head == NULL){
        priority_queue[index].head = new_tcb;
        priority_queue[index].tail = new_tcb;
    }else{
        new_tcb->prev = priority_queue[index].tail;
        new_tcb->next = NULL;

        priority_queue[index].tail->next = new_tcb;
        priority_queue[index].tail = new_tcb;
    }
}

TCB *pop_tid_tcb(U32 index, U8 tid){
    // If queue is empty
    if(priority_queue[index].head == NULL){
        return NULL;
    }

    // Temp TCB
    TCB * temp = priority_queue[index].head;

    while(temp != NULL){
        // Check if the current tid is the one we are looking for
        if(temp->tid == tid){
            // Check if head
            if(temp == priority_queue[index].head){
                return pop_head_tcb(index);
            }

            // Check if tail
            if(temp == priority_queue[index].tail){
                temp->prev->next = NULL;
                priority_queue[index].tail = temp->prev;

                temp->next = NULL;
                temp->prev = NULL;

                return temp;
            }

            // if not head or tail
            temp->prev->next = temp->next;
            temp->next->prev = temp->prev;

            temp->next = NULL;
            temp->prev = NULL;

            return temp;
        }
        
        temp = temp->next;
    }

    return NULL;
}

TCB *pop_head_tcb(U32 index){
    // If queue is empty
    if(priority_queue[index].head == NULL){
        return NULL;
    }

    // Temp TCB
    TCB * temp = priority_queue[index].head;

    // If only tcb in queue
    if(priority_queue[index].head == priority_queue[index].tail){
        priority_queue[index].head = NULL;
        priority_queue[index].tail = NULL;
    }else{
        priority_queue[index].head = priority_queue[index].head->next;
        priority_queue[index].head->prev = NULL;

        temp->next = NULL;
        temp->prev = NULL;
    }

    return temp;
}

void print_queue(void){
	//printf("\t\tPRINTING CURRENT QUEUE\r\n");
	TCB * temp;
	
	for(U32 i = 0; i < NUM_PRIO; i++){
		temp = priority_queue[i].head;
		//printf("\t\tPRIO %u\r\n", i);
		while(temp != NULL){
			//printf("\tTCB 0x%x\r\n", temp);
			temp = temp->next;
		}
	}
	//printf("\tFINISHED CURRENT QUEUE\r\n");
}

/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */


/**************************************************************************//**
 * @brief   scheduler, pick the TCB of the next to run task
 *
 * @return  TCB pointer of the next to run task
 * @post    gp_curret_task is updated
 * @note    you need to change this one to be a priority scheduler
 *
 *****************************************************************************/

TCB *scheduler(void){
    gp_current_task = NULL;
    // Searches each priority list for a TCB
    for(U32 i = 0; i < NUM_PRIO; i++){
        gp_current_task = pop_head_tcb(i);
        if(gp_current_task != NULL){
            return gp_current_task;
        }
    }

    // Return NULL task if no tasks found
    gp_current_task = &g_tcbs[TID_NULL];
    return gp_current_task;
}


/**
 * @brief initialzie the first task in the system
 */
void k_tsk_init_first(TASK_INIT *p_task)
{
    p_task->prio         = PRIO_NULL;
    p_task->priv         = 0;
    p_task->tid          = TID_NULL;
    p_task->ptask        = &task_null;
    p_task->u_stack_size = PROC_STACK_SIZE;
}

/**************************************************************************//**
 * @brief       initialize all boot-time tasks in the system,
 *
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       task_info   boot-time task information structure pointer
 * @param       num_tasks   boot-time number of tasks
 * @pre         memory has been properly initialized
 * @post        none
 * @see         k_tsk_create_first
 * @see         k_tsk_create_new
 *****************************************************************************/

int k_tsk_init(TASK_INIT *task, int num_tasks)
{
    if (num_tasks > MAX_TASKS - 1) {
        errno = EINVAL;
        return RTX_ERR;
    }

    for (int i = 0; i < NUM_PRIO; i++) {
        priority_queue[i].head = NULL;
        priority_queue[i].tail = NULL;
    }

    TASK_INIT taskinfo;
    
    k_tsk_init_first(&taskinfo);
    
    if ( k_tsk_create_new(&taskinfo, &g_tcbs[TID_NULL], TID_NULL) == RTX_OK ) {
        g_num_active_tasks = 1;
        gp_current_task = &g_tcbs[TID_NULL];
        gp_current_task->u_sp = null_usp;
		gp_current_task->state = RUNNING;
    } else {
        g_num_active_tasks = 0;
        return RTX_ERR;
    }
		
    // create the rest of the tasks
    for ( int i = 1; i < MAX_TASKS; i++ ) {
        if (i-1 < num_tasks) {
            TCB *p_tcb = &g_tcbs[i];

            if (k_tsk_create_new(&task[i - 1], p_tcb, i) == RTX_OK) {
								insert_back_queue(get_index(p_tcb->prio), p_tcb);
								g_num_active_tasks++;
            }else{
								return RTX_ERR;
						}
        }
        else {
            g_tcbs[i].state = DORMANT;
            g_tcbs[i].tid = i;
        }
    }

    return RTX_OK;
}
/**************************************************************************//**
 * @brief       initialize a new task in the system,
 *              one dummy kernel stack frame, one dummy user stack frame
 *
 * @return      RTX_OK on success; RTX_ERR on failure
 * @param       p_taskinfo  task initialization structure pointer
 * @param       p_tcb       the tcb the task is assigned to
 * @param       tid         the tid the task is assigned to
 *
 * @details     From bottom of the stack,
 *              we have user initial context (xPSR, PC, SP_USR, uR0-uR3)
 *              then we stack up the kernel initial context (kLR, kR4-kR12, PSP, CONTROL)
 *              The PC is the entry point of the user task
 *              The kLR is set to SVC_RESTORE
 *              20 registers in total
 * @note        YOU NEED TO MODIFY THIS FILE!!!
 *****************************************************************************/
int k_tsk_create_new(TASK_INIT *p_taskinfo, TCB *p_tcb, task_t tid)
{
    extern U32 SVC_RTE;

    U32 *usp;
    U32 *ksp;

    if (p_taskinfo == NULL || p_tcb == NULL)
    {
        return RTX_ERR;
    }

    p_tcb->tid   = tid;
    p_tcb->state = READY;
    p_tcb->prio  = p_taskinfo->prio;
    p_tcb->priv  = p_taskinfo->priv;
    p_tcb->ptask = p_taskinfo->ptask;
    p_tcb->u_stack_size = p_taskinfo->u_stack_size;
    
    /*---------------------------------------------------------------
     *  Step1: allocate user stack for the task
     *         stacks grows down, stack base is at the high address
     * ATTENTION: you need to modify the following three lines of code
     *            so that you use your own dynamic memory allocator
     *            to allocate variable size user stack.
     * -------------------------------------------------------------*/
    
		if(tid != TID_NULL){
			usp = k_mpool_alloc(MPID_IRAM2, p_taskinfo->u_stack_size);
			if (usp == NULL) {
					errno = ENOMEM;
					return RTX_ERR;
			}
			p_tcb->alloc_address = (U32)usp;
			usp += (p_taskinfo->u_stack_size)/4;
			if ((U32)usp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
					usp--;               // adjust it to 8B aligned
			}
		}else{
			usp = (U32*) null_usp;
		}
		
    p_tcb->u_sp_base = (U32)usp;
    p_tcb->u_sp = (U32)usp;
		
    /*-------------------------------------------------------------------
     *  Step2: create task's thread mode initial context on the user stack.
     *         fabricate the stack so that the stack looks like that
     *         task executed and entered kernel from the SVC handler
     *         hence had the exception stack frame saved on the user stack.
     *         This fabrication allows the task to return
     *         to SVC_Handler before its execution.
     *
     *         8 registers listed in push order
     *         <xPSR, PC, uLR, uR12, uR3, uR2, uR1, uR0>
     * -------------------------------------------------------------*/

    // if kernel task runs under SVC mode, then no need to create user context stack frame for SVC handler entering
    // since we never enter from SVC handler in this case
    
    *(--usp) = INITIAL_xPSR;             // xPSR: Initial Processor State
    *(--usp) = (U32) (p_taskinfo->ptask);// PC: task entry point
        
    // uR14(LR), uR12, uR3, uR3, uR1, uR0, 6 registers
    for ( int j = 0; j < 6; j++ ) {
        
#ifdef DEBUG_0
        *(--usp) = 0xDEADAAA0 + j;
#else
        *(--usp) = 0x0;
#endif
    }
    
    // allocate kernel stack for the task
    ksp = k_alloc_k_stack(tid);
    if ( ksp == NULL ) {
        return RTX_ERR;
    }

    p_tcb->k_sp_base = (U32)ksp;

    /*---------------------------------------------------------------
     *  Step3: create task kernel initial context on kernel stack
     *
     *         12 registers listed in push order
     *         <kLR, kR4-kR12, PSP, CONTROL>
     * -------------------------------------------------------------*/
    // a task never run before directly exit
    *(--ksp) = (U32) (&SVC_RTE);
    // kernel stack R4 - R12, 9 registers
#define NUM_REGS 9    // number of registers to push
      for ( int j = 0; j < NUM_REGS; j++) {        
#ifdef DEBUG_0
        *(--ksp) = 0xDEADCCC0 + j;
#else
        *(--ksp) = 0x0;
#endif
    }
        
    // put user sp on to the kernel stack
    *(--ksp) = (U32) usp;
    
    // save control register so that we return with correct access level
    if (p_taskinfo->priv == 1) {  // privileged 
        *(--ksp) = __get_CONTROL() & ~BIT(0); 
    } else {                      // unprivileged
        *(--ksp) = __get_CONTROL() | BIT(0);
    }

    p_tcb->msp = ksp;
    p_tcb->u_sp = (U32)usp;
		
    return RTX_OK;
}

/**************************************************************************//**
 * @brief       switching kernel stacks of two TCBs
 * @param       p_tcb_old, the old tcb that was in RUNNING
 * @return      RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre         gp_current_task is pointing to a valid TCB
 *              gp_current_task->state = RUNNING
 *              gp_current_task != p_tcb_old
 *              p_tcb_old == NULL or p_tcb_old->state updated
 * @note        caller must ensure the pre-conditions are met before calling.
 *              the function does not check the pre-condition!
 * @note        The control register setting will be done by the caller
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *
 *****************************************************************************/
__asm void k_tsk_switch(TCB *p_tcb_old)
{
        PRESERVE8
        EXPORT  K_RESTORE
        
        PUSH    {R4-R12, LR}                // save general pupose registers and return address
        MRS     R4, CONTROL                 
        MRS     R5, PSP
        PUSH    {R4-R5}                     // save CONTROL, PSP
        STR     SP, [R0, #TCB_MSP_OFFSET]   // save SP to p_old_tcb->msp
K_RESTORE
        LDR     R1, =__cpp(&gp_current_task)
        LDR     R2, [R1]
        LDR     SP, [R2, #TCB_MSP_OFFSET]   // restore msp of the gp_current_task
        POP     {R4-R5}
        MSR     PSP, R5                     // restore PSP
        MSR     CONTROL, R4                 // restore CONTROL
        ISB                                 // flush pipeline, not needed for CM3 (architectural recommendation)
        POP     {R4-R12, PC}                // restore general purpose registers and return address
}


__asm void k_tsk_start(void)
{ 
        PRESERVE8
        B K_RESTORE
}

/**************************************************************************//**
 * @brief       run a new thread. The caller becomes READY and
 *              the scheduler picks the next ready to run task.
 * @return      RTX_ERR on error and zero on success
 * @pre         gp_current_task != NULL && gp_current_task == RUNNING
 * @post        gp_current_task gets updated to next to run task
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 * @attention   CRITICAL SECTION
 * !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
 *****************************************************************************/
int k_tsk_run_new(void)
{
    TCB *p_tcb_old = NULL;
    
    if (gp_current_task == NULL) {
        return RTX_ERR;
    }

    p_tcb_old = gp_current_task;
    gp_current_task = scheduler();
    
    if ( gp_current_task == NULL  ) {
        gp_current_task = p_tcb_old;        // revert back to the old task
        return RTX_ERR;
    }

    // at this point, gp_current_task != NULL and p_tcb_old != NULL
    if (gp_current_task != p_tcb_old) {
        gp_current_task->state = RUNNING;   // change state of the to-be-switched-in  tcb
        p_tcb_old->state = READY;           // change state of the to-be-switched-out tcb
        p_tcb_old->u_sp = __get_PSP();  // TODO: DOUBLE CHECK THIS
        k_tsk_switch(p_tcb_old);            // switch kernel stacks       
    }

    return RTX_OK;
}

 
/**************************************************************************//**
 * @brief       yield the cpu
 * @return:     RTX_OK upon success
 *              RTX_ERR upon failure
 * @pre:        gp_current_task != NULL &&
 *              gp_current_task->state = RUNNING
 * @post        gp_current_task gets updated to next to run task
 * @note:       caller must ensure the pre-conditions before calling.
 *****************************************************************************/
int k_tsk_yield(void)
{
    if(gp_current_task == NULL || gp_current_task->state != RUNNING){
        return RTX_ERR;
    }
		
		if(gp_current_task->tid != TID_NULL){
			insert_back_queue(get_index(gp_current_task->prio), gp_current_task);
		}

    return k_tsk_run_new();
}

/**
 * @brief   get task identification
 * @return  the task ID (TID) of the calling task
 */
task_t k_tsk_gettid (void)
{
    return gp_current_task->tid;
}

/*
 *===========================================================================
 *                             TO BE IMPLEMETED IN LAB2
 *===========================================================================
 */

int k_tsk_create(task_t *task, void (*task_entry)(void), U8 prio, U32 stack_size)
{
#ifdef DEBUG_0
    printf("k_tsk_create: entering...\n\r");
    printf("task = 0x%x, task_entry = 0x%x, prio=%d, stack_size = %d\n\r", task, task_entry, prio, stack_size);
#endif /* DEBUG_0 */

    if ( prio != HIGH && prio != MEDIUM && prio != LOW && prio != LOWEST && prio != PRIO_NULL ) {
        errno = EINVAL;
        return RTX_ERR;
    }

    if ( g_num_active_tasks >= MAX_TASKS ) {
        errno = EAGAIN;
        return RTX_ERR;
    }

    TASK_INIT taskinfo;
    taskinfo.prio = prio;
    taskinfo.priv = UNPRIVILEGED;
    taskinfo.ptask = task_entry;


    TCB *my_tcb = getAvailableTCB();
    taskinfo.tid = my_tcb->tid;

    taskinfo.u_stack_size = (stack_size > PROC_STACK_SIZE) ? stack_size : PROC_STACK_SIZE;

    if (k_tsk_create_new(&taskinfo, my_tcb, taskinfo.tid) == RTX_OK) {
        g_num_active_tasks++;
    }
    else {
        return RTX_ERR;
    }

    *task = taskinfo.tid;
		
		// QUEUE OPERATIONS
		insert_back_queue(get_index(my_tcb->prio), my_tcb);

		if (my_tcb->prio < gp_current_task->prio){
				if (gp_current_task->tid != TID_NULL) {
						insert_front_queue(get_index(gp_current_task->prio), gp_current_task);
				}
				k_tsk_run_new();
		}

    return RTX_OK;
}

void k_tsk_exit(void) 
{
#ifdef DEBUG_0
    printf("k_tsk_exit: entering...\n\r");
#endif /* DEBUG_0 */

    if (gp_current_task == NULL || gp_current_task->tid == TID_NULL) {
        printf("UH OH SPAGHETTI OH\r\n");
        return;
    }

    TCB *p_tcb_old = gp_current_task;

    gp_current_task = scheduler();

    p_tcb_old->state = DORMANT;
    g_num_active_tasks -= 1;
    gp_current_task->state = RUNNING;
    p_tcb_old->u_sp = __get_PSP();
    k_tsk_switch(p_tcb_old);                     

    // dealloc p_tcb_old if not null task
     k_mpool_dealloc(MPID_IRAM2, (void*) p_tcb_old->alloc_address);

    return;
}

int k_tsk_set_prio(task_t task_id, U8 prio) 
{
#ifdef DEBUG_0
    printf("k_tsk_set_prio: entering...\n\r");
    printf("task_id = %d, prio = %x.\n\r", task_id, prio);
#endif /* DEBUG_0 */

    //'queue_queue' is the name of the queue array (which holds HIGHEST, MEDIUM, LOW, LOWEST queues) /*SHOULD BE IMPLEMENTED ABOVE*/
    //'gp_current_task' is the name of the pointer to the running TCB
    //'g_tcbs' is the name of the tcb array (which holds all TCBs)

    //exit with error if invalid priority level
    if (prio != HIGH && prio != MEDIUM && prio != LOW && prio != LOWEST) {
        errno = EINVAL;
        return RTX_ERR;
    }

    if (task_id >= MAX_TASKS) {
        errno = EINVAL;
        return RTX_ERR;
    }

    TCB *tcb = NULL;

    // get tcb from given task_id 
    for(U32 i = 1; i < MAX_TASKS; i++){
        if(g_tcbs[i].tid == task_id){
            tcb = &g_tcbs[i];
            break;
        }
    }
		
    //if tcb is still null, no tcb was found for given task_id
    if(tcb == NULL){
        errno = EINVAL;
        return RTX_ERR;
    }

    //After this if statement: Priviledge is no longer a concern
    if(gp_current_task->priv < tcb->priv){
        errno = EPERM;
        return RTX_ERR;
    }

    //if dormant task_id -> do nothing & return 0
    if(tcb->state == DORMANT){
        return RTX_OK;
    }

    //Change queue structure accordingly (/**/ comments are psuedo code [need to be replaced])
    if(gp_current_task->tid == tcb->tid){
        //Changing own prio (not in any queue atm)
        if(tcb->prio < prio){
            //changing to lower prio
            tcb->prio = prio;
            insert_back_queue(get_index(prio), tcb);
            //calling scheduler
            k_tsk_run_new();
        }else{ //ELSE changing to greater or equal prio -> do nothing
            tcb->prio = prio;
        }
    } else {
        //Changing other prio
        if(tcb->prio != prio){
            //changing to lower OR higher prio
            pop_tid_tcb(get_index(tcb->prio), task_id);
            tcb->prio = prio;
            insert_back_queue(get_index(prio), tcb);
            //if gp_current_task has lower prio than changes tcb: put gp_current_task on front of its queue
            if(gp_current_task->prio > prio){
                insert_front_queue(get_index(gp_current_task->prio), gp_current_task);
                k_tsk_run_new();
            }
        }
        //ELSE unchanged (equal) prio -> do nothing
    }

    return RTX_OK;    
}

/**
 * @brief   Retrieve task internal information 
 * @note    this is a dummy implementation, you need to change the code
 */
int k_tsk_get(task_t tid, RTX_TASK_INFO *buffer) {
#ifdef DEBUG_0
    printf("k_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    
    if (buffer == NULL) {
        errno = EFAULT;
        return RTX_ERR;
    }
    else if (MAX_TASKS <= tid) {
        errno = EINVAL;
        return RTX_ERR;
    }
    /* The code fills the buffer with some fake task information. 
       You should fill the buffer with correct information    */



    TCB *my_tcb;

    my_tcb = &g_tcbs[tid];
    
    if (my_tcb != NULL && my_tcb->tid == tid) {
        buffer->ptask         = my_tcb->ptask;

        buffer->k_sp          = (U32)my_tcb->msp;
        buffer->k_sp_base     = my_tcb->k_sp_base;
        buffer->k_stack_size  = KERN_STACK_SIZE;
        
        buffer->u_sp          = my_tcb->u_sp;
        buffer->u_sp_base     = my_tcb->u_sp_base;
        buffer->u_stack_size  = my_tcb->u_stack_size;

        buffer->priv          = my_tcb->priv;
        buffer->tid           = my_tcb->tid;
        buffer->prio          = my_tcb->prio;
        buffer->state         = my_tcb->state;
        
        if (k_tsk_gettid() == tid) {
            buffer->k_sp = __get_MSP();
            buffer->u_sp = __get_PSP();
        }

        return RTX_OK;
    }
    else {
        errno = EINVAL;
        return RTX_ERR;
    }   
}

int k_tsk_ls(task_t *buf, size_t count){
#ifdef DEBUG_0
    printf("k_tsk_ls: buf=0x%x, count=%u\r\n", buf, count);
#endif /* DEBUG_0 */

    if (buf == NULL || count == 0) {
        errno = EFAULT;
        return RTX_ERR;
    }

    int non_dormants = 0;
    int index = 0;

    while (index != MAX_TASKS && non_dormants < count) {
        if (g_tcbs[index].state != DORMANT && g_tcbs[index].tid == index) {
            buf[non_dormants] = g_tcbs[index].tid;
            non_dormants++;
        }

        index++;
    }

    // int out = (non_dormants < count) ? non_dormants : count;
    // return out;

    return non_dormants;
}

int k_rt_tsk_set(TIMEVAL *p_tv)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_set: p_tv = 0x%x\r\n", p_tv);
#endif /* DEBUG_0 */
    return RTX_OK;   
}

int k_rt_tsk_susp(void)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_susp: entering\r\n");
#endif /* DEBUG_0 */
    return RTX_OK;
}

int k_rt_tsk_get(task_t tid, TIMEVAL *buffer)
{
#ifdef DEBUG_0
    printf("k_rt_tsk_get: entering...\n\r");
    printf("tid = %d, buffer = 0x%x.\n\r", tid, buffer);
#endif /* DEBUG_0 */    
    if (buffer == NULL) {
        return RTX_ERR;
    }   
    
    /* The code fills the buffer with some fake rt task information. 
       You should fill the buffer with correct information    */
    buffer->sec  = 0xABCD;
    buffer->usec = 0xEEFF;
    
    return RTX_OK;
}
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

