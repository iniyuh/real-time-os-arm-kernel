/*
 ****************************************************************************
 *
 *                  UNIVERSITY OF WATERLOO ECE 350 RTOS LAB
 *
 *                     Copyright 2020-2022 Yiqing Huang
 *
 *          This software is subject to an open source license and
 *          may be freely redistributed under the terms of MIT License.
 ****************************************************************************
 */

/**************************************************************************//**
 * @file        k_mem.h
 * @brief       Kernel Memory Management API Header File
 *
 * @version     V1.2021.04
 * @authors     Yiqing Huang
 * @date        2021 APR 
 *
 * @note        skeleton code
 *
 *****************************************************************************/


#ifndef K_MEM_H_
#define K_MEM_H_
#include "k_inc.h"
#include "lpc1768_mem.h"        // board memory map
#include "log.h"
#include "tree.h"
#include "printf.h"

/*
 * ------------------------------------------------------------------------
 *                             FUNCTION PROTOTYPES
 * ------------------------------------------------------------------------
 */
// kernel API that requires mpool ID
mpool_t k_mpool_create  (int algo, U32 start, U32 end);
void   *k_mpool_alloc   (mpool_t mpid, size_t size);
int     k_mpool_dealloc (mpool_t mpid, void *ptr);
int     k_mpool_dump    (mpool_t mpid);

int     k_mem_init      (int algo);
U32    *k_alloc_k_stack (task_t tid);
U32    *k_alloc_p_stack (task_t tid);
// declare newly added functions here

typedef struct node{
    U32 x;
    struct node *next;
    struct node *prev;
}NODE;

typedef struct list{
    NODE *head;
}LIST;

void add_node(mpool_t mpid, knx coords, void *address);
U32 pop_node(mpool_t mpid, knx coords);
void delete_node(mpool_t mpid, knx coords);
U32 print_list(mpool_t mpid, knx coords);

U32 get_max_k (mpool_t mpid);
U32 get_start (mpool_t mpid);
U32 get_size (mpool_t mpid);

/*
 * ------------------------------------------------------------------------
 *                             FUNCTION MACROS
 * ------------------------------------------------------------------------
 */

#endif // ! K_MEM_H_

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

