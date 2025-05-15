/** 
 * @brief The KCD Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */
#include <stdint.h>
#include "rtx.h"
#include "common.h"
#include "kcd_task.h"
#include "uart_polling.h"

commandLL_t* registeredCommands = NULL;

//called in tsk_exit()
void removeTidRegisteredCommands(task_t tid)
{
    commandLL_t dummy;
    dummy.next = registeredCommands;
    
    commandLL_t* runner = &dummy;

    while (runner != NULL && runner->next != NULL)
    {
        if (runner->next->tid == tid)
        {
            k_mpool_dealloc(MPID_IRAM2, runner->next->commandName);
            k_mpool_dealloc(MPID_IRAM2, runner->next);
            runner->next = runner->next->next;
        }
        else
        {
            runner = runner->next;
        }
    }
}

task_t findTidFromCommand(char* commandName)
{
    commandLL_t* runner = registeredCommands;

    while(runner)
    {
        if (strcmp(runner->commandName, commandName) == 0)
        {
            return runner->tid;
        }
        runner = runner->next;
    }
    return 255; //No tid registered to the command
}

void updateCommandTid(char* commandName, task_t tid)
{
    commandLL_t* runner = registeredCommands;

    while (runner)
    {
        if (strcmp(runner->commandName, commandName) == 0)
        {
            runner->tid = tid;
            return;
        }

        runner = runner->next;
    }

    //add to the end of the list
    runner = (commandLL_t*) k_mpool_alloc(MPID_IRAM2, sizeof(commandLL_t));
    runner->commandName = commandName;
    runner->next = NULL;
    runner->tid = tid;
}

/*
List of supported commands
    %LT - print the taskid and stat of all non-dormant tasks to the console
    %LM - Print the task ID and state of all non dormant tasks with a mailbox to the console along with bytes left in each mailbox
    %[user defined string] - Will send the request to the last task that registered that string (can be overridden). This will use the KCD_CMD message

Edge case behavior
    - If the task that was registered to the task has exited then print "Command not found"
    - If the string doesnt start with % or length is longer that what is allowed the print "Invalid command"
*/

void task_kcd(void)
{
    uint8_t maxInputBufferSize = sizeof(struct rtx_msg_hdr) + KCD_CMD_BUF_SIZE + 1; 
    //+KCD_CMD_BUF_SIZE because the only messages that I am getting is either a single keystroke (could be more than one char) from the interrupt or a register command which can be a string of size KCD_CMD_BUF_SIZE and +1 for null char
    uint8_t inputBuffer[maxInputBufferSize];

    char bufferedInput[KCD_CMD_BUF_SIZE + 1]; //+1 for null char
    bufferedInput[KCD_CMD_BUF_SIZE] = '\0';
    uint8_t sizeOfBufferedInput = 0;
    uint8_t overflowInput = 0;


    struct rtx_msg_hdr* recvMessage = NULL;
    while(1)
    {
        if (recv_msg(inputBuffer, maxInputBufferSize) == -1)
        {
            //TODO: handle recv msg failed
        }
        recvMessage = (struct rtx_msg_hdr*) inputBuffer;
        uint32_t lenCommandInput = recvMessage->length - sizeof(struct rtx_msg_hdr);
        char* commandInput = ((char*)inputBuffer) + sizeof(struct rtx_msg_hdr);
        switch (recvMessage->type)
        {
            case KCD_REG:
            {
                char* commandName = k_mpool_alloc(MPID_IRAM2, lenCommandInput + 1); // +1 for null char
                strncpy(commandName, commandInput, lenCommandInput);
                commandName[lenCommandInput] = '\0';
                updateCommandTid(commandName, recvMessage->sender_tid);
                break;  
            } 
            case KEY_IN:
            {
                for(uint8_t b = 0; b < lenCommandInput; ++b)
                {
                    uart0_put_char(inputBuffer[b]);
                    k_send_msg_nb()
                    
                }
                    
                //TODO verify carriage return works
                if(*commandInput == '\r')
                {
                    //Process input
                    if (bufferedInput[0] != '%' || overflowInput == 1)
                    {
                        char message[] = "Invalid command\n";
                        k_send_msg_nb(TID_CON, (void*) message);
                        overflowInput = 0;
                        sizeOfBufferedInput = 0; 
                        //discard the input bc invalid 
                        continue;
                    }


                    //I am doing this because it is reserved
                    if(sizeOfBufferedInput == 3 && bufferedInput[1] == 'L')
                    {
                        if (bufferedInput[2] == 'T')
                        {
                            overflowInput = 0;
                            sizeOfBufferedInput = 0; 
                            //TODO: %LT
                            continue;
                        }
                        else if (bufferedInput[2] == 'M')
                        {
                            overflowInput = 0;
                            sizeOfBufferedInput = 0; 
                            //TODO: %LM
                            continue;
                        }
                    }

                    //the request is sent to a task
                    recvMessage->length = sizeof(struct rtx_msg_hdr) + strlen(bufferedInput) - 1; //-1 for the %
                    recvMessage->sender_tid = TID_KCD;
                    recvMessage->type = KCD_CMD;

                    //TODO: Make sure that this doesnt overflow
                    memcpy(commandInput, bufferedInput+1, sizeOfBufferedInput-1); //add/sub 1 for the %
                    
                    task_t found_tid = findTidFromCommand(bufferedInput+1);

                    //if it cant find the command
                    if (found_tid == 255)
                    { 
                        char message[] = "Command not found\n";
                        k_send_msg_nb(TID_CON, (void*) message);
                    }
                    else
                    {
                        //TODO: double check to see if there is behavior for what happens if this fails
                        send_msg_nb(found_tid, inputBuffer); //+1 bc dont want to include the %
                    }
                    overflowInput = 0;
                    sizeOfBufferedInput = 0; 
                }
                else if (overflowInput == 1)
                {
                    //discard the input
                    continue;
                }
                else if (sizeOfBufferedInput + lenCommandInput > KCD_CMD_BUF_SIZE) //overflow the buffered input
                {
                    overflowInput = 1;
                }
                else
                {
                    //add to buffered input
                    for(uint8_t a = 0; a < lenCommandInput; ++a)
                    {
                        bufferedInput[sizeOfBufferedInput + a] = inputBuffer[a];
                    }
                    
                    sizeOfBufferedInput += lenCommandInput;
                }
                break;
            }
            default:
                //unexpected input message (in manual says this should be ignored)
                continue;

        }

    }
}

/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

