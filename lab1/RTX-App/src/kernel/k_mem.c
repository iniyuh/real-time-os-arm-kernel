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
 * @file        k_mem.c
 * @brief       Kernel Memory Management API C Code
 *
 * @version     V1.2021.01.lab2
 * @authors     Yiqing Huang
 * @date        2021 JAN
 *
 * @note        skeleton code
 *
 *****************************************************************************/

#include "k_inc.h"
#include "k_mem.h"

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
                              |                           |     |
                              |---------------------------|     |
                              |                           |     |
                              |      other data           |     |
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
             g_k_stacks[15]-->|---------------------------|     |
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
 *                            GLOBAL VARIABLES
 *===========================================================================
 */
// kernel stack size, referred by startup_a9.s
const U32 g_k_stack_size = KERN_STACK_SIZE;
// task proc space stack size in bytes, referred by system_a9.c
const U32 g_p_stack_size = PROC_STACK_SIZE;

// task kernel stacks
U32 g_k_stacks[MAX_TASKS][KERN_STACK_SIZE >> 2] __attribute__((aligned(8)));

// task process stack (i.e. user stack) for tasks in thread mode
// remove this bug array in your lab2 code
// the user stack should come from MPID_IRAM2 memory pool
//U32 g_p_stacks[MAX_TASKS][PROC_STACK_SIZE >> 2] __attribute__((aligned(8)));
U32 g_p_stacks[NUM_TASKS][PROC_STACK_SIZE >> 2] __attribute__((aligned(8)));

typedef struct mpool_handler {
    U8* bitArray;
    LIST* arr_freeLists;
} MPOOL_HANDLER;

U8 bitArray_1[32];
U8 bitArray_2[256];

LIST arr_freeLists_1[RAM1_SIZE_LOG2 - MIN_BLK_SIZE_LOG2 + 1];
LIST arr_freeLists_2[RAM2_SIZE_LOG2 - MIN_BLK_SIZE_LOG2 + 1];

MPOOL_HANDLER mpools[2];

/*
 *===========================================================================
 *                         FREE LIST FUNCTIONS
 *===========================================================================
 */
void add_node(mpool_t mpid, knx coords, void * address){
    NODE *new_node = address;
		NODE *head = mpools[mpid].arr_freeLists[coords.k].head;
	
    //Setting up new node
    new_node->x = coords.x;
    new_node->prev = NULL;
    new_node->next = NULL;

    //If this is the first added node
    if(head == NULL){
        mpools[mpid].arr_freeLists[coords.k].head = new_node;
        return;
    }

    //Compare Data at head node
    if(head->x > new_node->x){
        new_node->next = head;
        head->prev = new_node;
        mpools[mpid].arr_freeLists[coords.k].head = new_node;
        return;
    }

    //Traversing to sort
    NODE *current_node;
    current_node = head;
    while (current_node->next != NULL && current_node->next->x < new_node->x){
        current_node = current_node->next;
    }

    new_node->next = current_node->next;

    //Attaching new node as previous to the one ahead
    if(current_node->next != NULL){
        new_node->next->prev = new_node;
    }

    current_node->next = new_node;
    new_node->prev = current_node;
}

U32 pop_node(mpool_t mpid, knx coords){
		NODE* head = mpools[mpid].arr_freeLists[coords.k].head;

    // Check to see if head is NULL
    if(head == NULL){
        return NULL;
    }

    // Getting return value from head
		U32 x;
    x = head->x;

    // Remove head
    head = head->next;
    if(head != NULL){
        head->prev = NULL;
    }
		
		mpools[mpid].arr_freeLists[coords.k].head = head;
		
    return x;
}

void delete_node(mpool_t mpid, knx coords){
    NODE *current_node;
		NODE *head = mpools[mpid].arr_freeLists[coords.k].head;

    // If it is head node is null
    if(head == NULL){
        return;
    }

    //Traversing to find position
    current_node = head;

    while (current_node->next != NULL && current_node->x != coords.x){
        current_node = current_node->next;
    }

    if(current_node->x == coords.x){
        // if the node is the head
        if(head == current_node){
            head = current_node->next;
            if(head != NULL){
                head->prev = NULL;
            }
						mpools[mpid].arr_freeLists[coords.k].head = head;
        }

        // Assign next if not last node
        if(current_node->next != NULL){
            current_node->next->prev = current_node->prev;
        }

        // Assign next if not first node
        if(current_node->prev != NULL){
            current_node->prev->next = current_node->next;
        }
    }
}

U32 print_list(mpool_t mpid, knx coords){
    NODE *current_node;
		NODE *head = mpools[mpid].arr_freeLists[coords.k].head;
    U32 count = 0;
		
		// current node to travers list
    current_node = head;
    while (current_node != NULL){
				// printing and incrementing return value
        printf("0x%x: 0x%x\r\n", current_node, compute_block_size(coords, get_size(mpid)));
        count++;
        current_node = current_node->next;
				
    }
    return count;
}
/*
 *===========================================================================
 *                            FUNCTIONS
 *===========================================================================
 */

U32 get_max_k (mpool_t mpid){
    return mpid ? (RAM2_SIZE_LOG2 - MIN_BLK_SIZE_LOG2 + 1) : (RAM1_SIZE_LOG2 - MIN_BLK_SIZE_LOG2 + 1);
}

U32 get_start (mpool_t mpid){
    return mpid ? RAM2_START : RAM1_START;
}

U32 get_size (mpool_t mpid){
    return mpid ? RAM2_SIZE : RAM1_SIZE;
}

/* note list[n] is for blocks with order of n */
mpool_t k_mpool_create (int algo, U32 start, U32 end)
{
    mpool_t mpid = MPID_IRAM1;

#ifdef DEBUG_0
    printf("k_mpool_init: algo = %d\r\n", algo);
    printf("k_mpool_init: RAM range: [0x%x, 0x%x].\r\n", start, end);
#endif /* DEBUG_0 */    
    
    if (algo != BUDDY ) {
        errno = EINVAL;
        return RTX_ERR;
    }
		
    if ( start == RAM1_START) {
        
        if(end - start > get_size(mpid)){
            errno = ENOMEM;
            return RTX_ERR;
        }

        mpools[mpid].bitArray = bitArray_1;
        mpools[mpid].arr_freeLists = arr_freeLists_1;
				
				for (U32 i = 0; i < get_max_k(mpid); i++){
					mpools[mpid].arr_freeLists[i].head = NULL;
				}
				
				knx coords;
				coords.k = 0;
				coords.x = 0;
				
				add_node(mpid, coords, (void*) start);
				
        for (U32 i = 0; i < 32; i++){
            mpools[mpid].bitArray[i] = 0;
        }

    } else if ( start == RAM2_START) {
        mpid = MPID_IRAM2;

        if(end - start > get_size(mpid)){
            errno = ENOMEM;
            return RTX_ERR;
        }
        
        mpools[mpid].bitArray = bitArray_2;
        mpools[mpid].arr_freeLists = arr_freeLists_2;
				
				for (U32 i = 0; i < get_max_k(mpid); i++){
					mpools[mpid].arr_freeLists[i].head = NULL;
				}
				
				knx coords;
				coords.k = 0;
				coords.x = 0;
				
				add_node(mpid, coords, (void*) start);
				
        for (U32 i = 0; i < 256; i++){
            mpools[mpid].bitArray[i] = 0;
        }

    } else {
        errno = EINVAL;
        return RTX_ERR;
    }
    
    return mpid;
}

void *k_mpool_alloc (mpool_t mpid, size_t size)
{
#ifdef DEBUG_0
    printf("\t ALLOC%d SIZE 0X%x \r\n", mpid, size);
#endif /* DEBUG_0 */

    if(size == 0){
        return NULL;
    }
		
		// Calculating level to alloc memory to
		U32 tree_level_to_alloc = log_ceil(get_size(mpid)) - log_ceil(size);
		
		tree_level_to_alloc = (tree_level_to_alloc >= get_max_k(mpid)) ? (get_max_k(mpid)-1) : tree_level_to_alloc;
		
		// Settig Data Type
    knx coords;
    coords.k = tree_level_to_alloc;
    coords.x = 0;
		
    while(1){
				// Checks to see if level in free list is null
        if(mpools[mpid].arr_freeLists[coords.k].head == NULL){
						// If Null, check if root node
            if(coords.k == 0){
                errno = ENOMEM;
                return NULL;
            }
						// If not root, go to one level above
            coords.k--;
        } else {
						// If not null, there are nodes on this level
						// Pop first node from free list and set it as allocated
            coords.x = pop_node(mpid, coords);
            set_bit_on(mpools[mpid].bitArray, coords);
						
						// If it is on level to alloc, we are done
            if(coords.k == tree_level_to_alloc){
                return (void*)compute_block_address(coords, get_size(mpid), get_start(mpid));
            }
						
						// If not, find children and add to free list
						coords = find_left_child(coords);
						add_node(mpid, coords, (void*)compute_block_address(coords, get_size(mpid), get_start(mpid)));
						coords.x++;
						add_node(mpid, coords, (void*)compute_block_address(coords, get_size(mpid), get_start(mpid)));
						coords.x--;
        }
    }
}

int k_mpool_dealloc(mpool_t mpid, void *ptr)
{
#ifdef DEBUG_0
    printf("k_mpool_dealloc: mpid = %d, ptr = 0x%x\r\n", mpid, ptr);
#endif /* DEBUG_0 */
    // If they passed in invalid mpids
    if(mpid != MPID_IRAM1 && mpid != MPID_IRAM2){
        errno = EINVAL;
        return RTX_ERR;
    }

    // If ptr is NULL, do nothing
    if(ptr == NULL){
        return RTX_OK;
    }
		
    // Cast address from pointer, create knx data type to the bottom of the list
    U32 address_to_dealloc = (U32) ptr;
    knx coords;
    coords.k = get_max_k(mpid) - 1;
    coords.x = (address_to_dealloc - get_start(mpid))/(compute_block_size(coords, get_size(mpid)));

    // If out of memory bound, return error
    if(get_start(mpid) > address_to_dealloc || get_start(mpid) + get_size(mpid) < address_to_dealloc) {
        errno = EFAULT;
        return RTX_ERR;
    }
		
    while(1){
         //if we in correct spot to deallocate
        if(get_bit(mpools[mpid].bitArray, coords)){
            // If we are then add the node to the free list and turn it to 0 in bit array
						add_node(mpid, coords, ptr);  
						set_bit_off(mpools[mpid].bitArray, coords);
					
            // This checks the buddy and repeats the process
            while(1){
                // If buddy is set, no need to merge, exit
                if(get_bit(mpools[mpid].bitArray, find_buddy(coords))){
                    return RTX_OK;
                } else {
                    // no buddy is set, therefore we need to combine
                    // remove both from freelist
										delete_node(mpid, coords);
										coords = find_buddy(coords);
										delete_node(mpid, coords);
									
                    // Update the coords to parents, and add to free list
                    coords = find_parent(coords);
                    set_bit_off(mpools[mpid].bitArray, coords);
                    add_node(mpid, coords, (void*)compute_block_address(coords, get_size(mpid), get_start(mpid)));

                    // We have reached root
                    if(coords.k == 0){
                        return RTX_OK;
                    }
                }
            }
        } else { //we need to find its parent to check for deallocation
						// We have reached root
            if(coords.k == 0){
                return RTX_OK;
            }
						
            coords = find_parent(coords);
        }
    }
    //return RTX_OK; 
}

int k_mpool_dump (mpool_t mpid)
{
#ifdef DEBUG_0
    printf("k_mpool_dump: mpid = %d\r\n", mpid);
#endif /* DEBUG_0 */
    
    // If they passed in invalid mpids
    if(mpid != MPID_IRAM1 && mpid != MPID_IRAM2){
        return RTX_ERR;
    }
		
		// Data types, count keeps tracks of number of prints
    knx coords;
    U32 count = 0;
		
    for(U32 i = 0; i < get_max_k(mpid); i++){
        coords.k = i;
        count += print_list(mpid, coords);
    }
    printf("%u free memory block(s) found\n", count);

    return count;
}
 
int k_mem_init(int algo)
{
#ifdef DEBUG_0
    printf("k_mem_init: algo = %d\r\n", algo);
#endif /* DEBUG_0 */
        
    if ( k_mpool_create(algo, RAM1_START, RAM1_END) < 0 ) {
        return RTX_ERR;
    }
    
    if ( k_mpool_create(algo, RAM2_START, RAM2_END) < 0 ) {
        return RTX_ERR;
    }
    
    return RTX_OK;
}

/**
 * @brief allocate kernel stack statically
 */
U32* k_alloc_k_stack(task_t tid)
{
    
    if ( tid >= MAX_TASKS) {
        errno = EAGAIN;
        return NULL;
    }
    U32 *sp = g_k_stacks[tid+1];
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}

/**
 * @brief allocate user/process stack statically
 * @attention  you should not use this function in your lab
 */

U32* k_alloc_p_stack(task_t tid)
{
    if ( tid >= NUM_TASKS ) {
        errno = EAGAIN;
        return NULL;
    }
    
    U32 *sp = g_p_stacks[tid+1];
    
    
    // 8B stack alignment adjustment
    if ((U32)sp & 0x04) {   // if sp not 8B aligned, then it must be 4B aligned
        sp--;               // adjust it to 8B aligned
    }
    return sp;
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

