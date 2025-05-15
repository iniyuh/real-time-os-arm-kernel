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
    
#define NUM_TESTS       3       // number of tests
#define NUM_INIT_TASKS  2       // number of tasks during initialization
/*
 *===========================================================================
 *                             GLOBAL VARIABLES 
 *===========================================================================
 */

TASK_INIT   g_init_tasks[NUM_INIT_TASKS];
const char   PREFIX[]      = "G11-TS1";
const char   PREFIX_LOG[]  = "G11-TS1-LOG";
const char   PREFIX_LOG2[] = "G11-TS1-LOG2";

int result = 0;

void test_0(void);
void test_1(void);
void test_2(void);

void task1(void);
void task2(void);

void set_ae_init_tasks (TASK_INIT **pp_tasks, int *p_num) {
    *p_num = NUM_INIT_TASKS;
    *pp_tasks = g_init_tasks;
    set_ae_tasks(*pp_tasks, *p_num);
}

// initial task configuration
void set_ae_tasks(TASK_INIT *tasks, int num) {
    for (int i = 0; i < num; i++ ) {                                                 
        tasks[i].u_stack_size = PROC_STACK_SIZE;    
        tasks[i].prio = HIGH;
        tasks[i].priv = 1;
    }

    tasks[0].ptask = &task1;

    tasks[1].ptask = &task2;
    tasks[1].priv  = 0;
}

void task1 (void) {
    task_t tid = tsk_gettid();
    printf("%s: START\r\n", PREFIX);

    test_0();
    test_1();

    tsk_exit();
}

void task2(void)
{   
    test_2();

    printf("%s: %d/5 tests PASSED\r\n", PREFIX, result);
    printf("%s: %d/5 tests FAILED\r\n", PREFIX, 5 - result);
    printf("%s: END\r\n", PREFIX);
    
    return;
}


void test_0(void)
{   
    static RTX_TASK_INFO task_info;

    if (tsk_get(1,&task_info) == RTX_OK) {
        result++;
    }

    if (tsk_get(0, &task_info) == RTX_OK) {
        result++;
    }
		
		return;
}

void test_1(void)
{  
    static RTX_TASK_INFO task_info;

    if (tsk_get((task_t)1000,&task_info) == RTX_ERR) {
        if (errno == EINVAL) {
            result++;
        }
    }

    if (tsk_get((task_t) 1, NULL) == RTX_ERR) {
        if (errno == EFAULT) {
            result++;
        }
    }
		
		return;
}

void test_2(void)
{  
    RTX_TASK_INFO task_info;

    if (tsk_set_prio(2, LOW) == RTX_OK) {
        result++;
    }
		
		return;
}
