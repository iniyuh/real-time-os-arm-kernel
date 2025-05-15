#include <stdint.h>
#include <string.h>
#include "rtx.h"
#include "k_mem.h"

#ifndef KCD_TASK_H
#define KCD_TASK_H

/*
List of supported commands
    %LT - print the taskid and stat of all non-dormant tasks to the console
    %LM - Print the task ID and state of all non dormant tasks with a mailbox to the console along with bytes left in each mailbox
    %[user defined string] - Will send the request to the last task that registered that string (can be overridden). This will use the KCD_CMD message

Edge case behavior
    - If the task that was registered to the task has exited then print "Command not found"
    - If the string doesnt start with % or length is longer that what is allowed the print "Invalid command"
*/

typedef struct commandLinkedList
{
    char* commandName;               //string
    task_t tid;                         //the task id that the command is registered to 
    struct commandLinkedList* next;     
} commandLL_t;

extern char* TaskStateLut[];

task_t findTidFromCommand(char* commandName);

void task_kcd(void);

#endif //KCD_TASK_H
/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

