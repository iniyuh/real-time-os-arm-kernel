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
 * @file        k_msg.h
 * @brief       kernel message passing header file
 *
 * @version     V1.2021.06
 * @authors     Yiqing Huang
 * @date        2021 JUN
 *****************************************************************************/

 
#ifndef K_MSG_H_
#define K_MSG_H_

#include "k_task.h"
#include "k_inc.h"

int k_mbx_create    (size_t size);
int k_send_msg      (task_t receiver_tid, const void *buf);
int k_send_msg_nb   (task_t receiver_tid, const void *buf);
int k_recv_msg      (void *buf, size_t len);
int k_recv_msg_nb   (void *buf, size_t len);
int k_uart_recv_nb  (void *buf, size_t len);
int k_mbx_ls        (task_t *buf, size_t count);
int k_mbx_get       (task_t tid);

int init_ring_buffer(size_t size);
int write_to_queue(task_t tid, const void *msg);
void read_from_queue(char* buff, int length, task_t tid);
int get_size_left_tid(task_t tid);
U8 is_full(task_t tid);
U8 is_empty(task_t tid);
int get_length_of_next_msg(task_t tid);

U32 get_index_waiting_list(U32 priority);
uint8_t insert_back_waiting_list(task_t tid, U32 prio, struct tcb *new_tcb);
struct tcb *pop_tid_waiting_list(task_t tid_mailbox, task_t tid_to_pop, U32 prio);
struct tcb *pop_head_waiting_list(task_t tid, U32 prio);
struct tcb *peak_head_waiting_list(task_t tid);

U8 is_waiting_list_empty(task_t tid);

#endif // ! K_MSG_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

