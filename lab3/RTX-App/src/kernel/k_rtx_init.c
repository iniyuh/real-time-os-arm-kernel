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
 * @file        k_rtx_init.c
 * @brief       RTX System Initialization C file
 *              l2
 * @version     V1.2021.04
 * @authors     Yiqing Huang
 * @date        2021 ARP
 *
 * @details
 * @note
 *
 *****************************************************************************/

#include "k_rtx_init.h"
#include "k_rtx.h"
#include "k_inc.h"

int errno = 0;
U32 null_usp;

/**************************************************************************//**
 * @brief   	system set up before calling rtx_init() from thread mode  
 * @pre         processor should be in thread mode using MSP, rtx_init is not called
 * @post        PSP is intialized and points to NULL task user stack
 * @return      0 on success and non-zero on failure
 * @note        you may add extra code here
 *****************************************************************************/
int k_pre_rtx_init (void *args)
{
    if ( k_mem_init(((RTX_SYS_INFO *)(args))->mem_algo) != RTX_OK) {
        return RTX_ERR;
    }
		
    U32 *sp;
    sp = k_mpool_alloc(MPID_IRAM2, PROC_STACK_SIZE);
    sp += (PROC_STACK_SIZE/4);
    if ((U32)sp & 0x04) {
        sp--;
    }
		
		null_usp = (U32) sp;
    __set_PSP((U32) sp);
		
    return RTX_OK;
}

int k_rtx_init(RTX_SYS_INFO *sys_info, TASK_INIT *tasks, int num_tasks)
{
    errno = 0;

    if (num_tasks + 1 > MAX_TASKS) {
        errno = EINVAL;
        return RTX_ERR;
    }

    switch (sys_info->mem_algo) {
        case BUDDY:
            break;
        default:
            errno = EINVAL;
            return RTX_ERR;
    }

    switch (sys_info->sched) {
        case DEFAULT:
            break;
        default:
            errno = EINVAL;
            return RTX_ERR;
    }

    // for(int a = 0; a < MAX_TASKS; ++a)
    // {
    //     g_k_stacks[a] = k_mpool_alloc(MPID_IRAM2,KERN_STACK_SIZE >> 2);
    //     if (g_k_stacks[a] == NULL)
    //     {
    //         return RTX_ERR;
    //     }
    // }
    
    /* interrupts are already disabled when we enter here */
    if ( uart_irq_init(0) != RTX_OK ) {
        return RTX_ERR;
    }
    
    /* add timer(s) initialization code */
    if ( k_tsk_init(tasks, num_tasks) != RTX_OK ) {
        return RTX_ERR;
    }

    /* add message passing initialization code */
    k_tsk_start();        // start the first task
    return RTX_OK;
}

int k_get_sys_info(RTX_SYS_INFO *buffer)
{
    return RTX_OK;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
