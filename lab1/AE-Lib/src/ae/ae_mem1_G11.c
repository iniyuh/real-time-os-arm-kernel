#include "ae_tasks.h"
#include "uart_polling.h"
#include "printf.h"
#include "ae.h"
#include "ae_util.h"
#include "ae_tasks_util.h"

/*
 *===========================================================================
 *                             MACROS
 *===========================================================================
 */
    
#define NUM_TESTS       1       // number of tests
#define NUM_INIT_TASKS  1       // number of tasks during initialization

/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT    g_init_tasks[NUM_INIT_TASKS];
const char   PREFIX[]      = "G11-TS100";
const char   PREFIX_LOG[]  = "G11-TS0-LOG ";
const char   PREFIX_LOG2[] = "G11-TS0-LOG2";

AE_XTEST     g_ae_xtest;                // test data, re-use for each test
AE_CASE      g_ae_cases[NUM_TESTS];
AE_CASE_TSK  g_tsk_cases[NUM_TESTS];

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num)
{
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num)
{
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = HIGH + i;
        tasks[i].priv = 1;
    }
    tasks[0].priv  = 1;
    tasks[0].ptask = &priv_task1;
    
    init_ae_tsk_test();
}

void init_ae_tsk_test(void)
{
    g_ae_xtest.test_id = 0;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests = NUM_TESTS;
    g_ae_xtest.num_tests_run = 0;
    
    for ( int i = 0; i< NUM_TESTS; i++ ) {
        g_tsk_cases[i].p_ae_case = &g_ae_cases[i];
        g_tsk_cases[i].p_ae_case->results  = 0x0;
        g_tsk_cases[i].p_ae_case->test_id  = i;
        g_tsk_cases[i].p_ae_case->num_bits = 0;
        g_tsk_cases[i].pos = 0;  // first avaiable slot to write exec seq tid
        // *_expt fields are case specific, deligate to specific test case to initialize
    }
    printf("%s: START\r\n", PREFIX);
}

void update_ae_xtest(int test_id)
{
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

void gen_req0(int test_id)
{
    g_tsk_cases[test_id].p_ae_case->num_bits = 9;  
    g_tsk_cases[test_id].p_ae_case->results = 0;
    g_tsk_cases[test_id].p_ae_case->test_id = test_id;
    g_tsk_cases[test_id].len = 16; // assign a value no greater than MAX_LEN_SEQ
    g_tsk_cases[test_id].pos_expt = 0; // N/A for P1 tests
       
    g_ae_xtest.test_id = test_id;
    g_ae_xtest.index = 0;
    g_ae_xtest.num_tests_run++;
}

/**
 * @brief a basic memory test on IRAM2
 * ALL TEST ALLOCATION SIZES ARE BASED ON THE ASSUMPTION THAT IRAM2 HAS 0x8000 BITS OF SPACE
 */

#ifdef ECE350_P1
int test0_start(int test_id)
{
    static void *p[7];
    int ret_val    = 10;
    U8 *p_index    = &(g_ae_xtest.index);
    int sub_result = 0;
    
    gen_req0(test_id);
     
    for ( int i = 0; i < 7; i++ ) {
        p[i] = (void *)0x1;
    }

    //test 0-[0]
    p[0] = mem2_alloc(0x4000);
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "Allocating the first half of the RAM, check non-NULL return");
    sub_result = (p[0] == NULL) ? 0 : 1;
    process_sub_result(test_id, *p_index, sub_result);  
    
    //test 0-[1]
    p[1] = mem2_alloc(0x2000);
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "Allocating a quarter of the RAM (50% of the remainder), check not null, not the same as p[0]");
    sub_result = (p[1] != p[0] && p[1] != NULL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[2]
    p[2] = mem2_alloc(0x1000);
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "Allocating an eighth of the RAM (50% of the remainder), check not null, not the same as p[1] and therefore p[0]");
    sub_result = (p[2] != p[1] && p[2] != NULL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[3]
    p[3] = mem2_alloc(0x800);
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "Allocating a sixteenth of the RAM (50% of the remainder), check not null, not the same as p[2], and therefore p[1] and p[0]");
    sub_result = (p[3] != p[2] && p[3] != NULL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[4]
    (*p_index)++;
    ret_val = mem2_dump();
    strcpy(g_ae_xtest.msg, "Check mem2_dump return value is one (1 block available)");
    sub_result = (ret_val == 1) ? 1: 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[5]
    p[4] = mem2_alloc(0x800);
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "Allocating a sixteenth of the RAM (100% of the remainder), check not null, not the same as p[3], and therefore p[2], p[1] and p[0]");
    sub_result = (p[4] != p[3] && p[4] != NULL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[6]
    p[5] = mem2_alloc(0x800);
    (*p_index)++;
    strcpy(g_ae_xtest.msg, "Allocating when there is no free memory, check NULL return");
    sub_result = (p[5] == NULL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[7]
    (*p_index)++;
    ret_val = mem2_dump();
    strcpy(g_ae_xtest.msg, "Check mem2_dump return value is zero");
    sub_result = (ret_val == 0) ? 1: 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[8]
    (*p_index)++;
    mem2_dealloc(p[0]);
    p[0] = NULL;
    ret_val = mem2_dump();
    strcpy(g_ae_xtest.msg, "Free p[0], checking mem2_dump return val is one");
    sub_result = (ret_val == 1) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    //test 0-[9]
    p[0] = mem2_alloc(0x1000);
    *p_index = 0;
    strcpy(g_ae_xtest.msg, "Allocating an eighth of the RAM (25% of the remainder), check not null, not the same as p[5], and therefore p[4], p[3], p[2] and p[1]");
    sub_result = (p[0] != p[5] && p[0] != NULL) ? 1 : 0;
    process_sub_result(test_id, *p_index, sub_result);

    return RTX_OK;
}
#else 
int test0_start(int test_id)
{
    return RTX_ERR;
}
#endif // ECE350_P1


/**************************************************************************//**
 * @brief       a task that prints AAAAA, BBBBB on each line.
 *              it yields the cpu after each line
 *****************************************************************************/

void priv_task1(void)
{
    int test_id = 0;
    
    printf("%s: priv_task1: basic memory test on IRAM1\r\n", PREFIX_LOG2);
#ifdef ECE350_P1    
    test0_start(test_id);
#endif // ECE350_P1
    test_exit();
}




/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */
