/** 
 * @brief The KCD Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */
#include "rtx.h"
#include "k_inc.h"

// ---------------- LINKED LIST FOR COMMANDS ----------------
typedef struct command_node{
    U8* cmd;
    task_t tid;
    struct command_node *next;
}CMD_NODE;

uint8_t command_buffer[ KCD_CMD_BUF_SIZE ];
size_t command_length = 0;
uint8_t print_flag = 0;

CMD_NODE *head_node = NULL;

int kcd_strcmp(const U8* str1, const U8* str2){
    while(*str1 && (*str1 == *str2)){
        str1++;
        str2++;
    }
    return *(const unsigned char*)str1 - *(const unsigned char*)str2;
}

void insert_command(U8* str, task_t tid){
    CMD_NODE *new_node = NULL;
    new_node = (CMD_NODE*) mem_alloc(sizeof(CMD_NODE));

    // TODO: handle out of memory error
    if(new_node == NULL){
        printf("OUT OF MEMORY\r\n");
    }

    CMD_NODE *current_node = NULL;
    current_node = head_node;

    // Deleting the command if it already exists
    while(current_node != NULL){
        if(kcd_strcmp(current_node->cmd, str) == 0){
            CMD_NODE *temp_node = NULL;
            temp_node = current_node->next;

            mem_dealloc(current_node->cmd);
            mem_dealloc(current_node);
            
            current_node = temp_node;
        }else{
            current_node = current_node->next;
        }
    }

    new_node->cmd = str;
    new_node->tid = tid;

    new_node->next = head_node;
    head_node = new_node;
}

task_t search_command(U8* str){
    CMD_NODE* current_node = NULL;
    current_node = head_node;

    while(current_node != NULL){
        if(kcd_strcmp(current_node->cmd, str) == 0){
            return current_node->tid;
        }else{
            current_node = current_node->next;
        }
    }

    return 255;
}

// ----------------------------------------------------------

void send_echo ( uint8_t receive_buffer[] ) {
    U8* send_buffer = (U8*) mem_alloc(MSG_HDR_SIZE+2);
    //uint8_t send_buffer[ MSG_HDR_SIZE + 2 ];
    RTX_MSG_HDR *send_header = (void*) send_buffer;

    send_header->length = MSG_HDR_SIZE + 2;
    send_header->sender_tid = TID_KCD;
    send_header->type = DISPLAY;

    send_buffer[ MSG_HDR_SIZE ] = receive_buffer[ MSG_HDR_SIZE ];
    send_buffer[ MSG_HDR_SIZE + 1 ] = '\0';

    send_msg(TID_CON, send_buffer);

    mem_dealloc(send_buffer);
	
	return;
}

void print_string ( char *string , size_t string_length ) {
  U8* send_buffer = (U8*) mem_alloc(MSG_HDR_SIZE+string_length+2);
	RTX_MSG_HDR *send_header = (void*) send_buffer;
	
	send_header->length = MSG_HDR_SIZE + string_length + 2;
	send_header->sender_tid = TID_KCD;
	send_header->type = DISPLAY;
	
	for (int i = 0; i < string_length; i++) {
		send_buffer[ MSG_HDR_SIZE + i ] = *(string + i);
	}
	send_buffer[ MSG_HDR_SIZE + string_length ] = '\n';
	send_buffer[ MSG_HDR_SIZE + string_length + 1 ] = '\0';
	
	send_msg( TID_CON, send_buffer );
	
  mem_dealloc(send_buffer);

	return;
}


void task_kcd (void) {
    mbx_create(KCD_MBX_SIZE);
    //uint8_t receive_buffer[ sizeof(struct rtx_msg_hdr) + KCD_CMD_BUF_SIZE + 1 ];
    U8* receive_buffer = (U8*) mem_alloc( sizeof(struct rtx_msg_hdr) + KCD_CMD_BUF_SIZE );
    RTX_MSG_HDR *receive_header = (void*) receive_buffer;

    while (TRUE) {
		if ( recv_msg( receive_buffer, sizeof(struct rtx_msg_hdr) + KCD_CMD_BUF_SIZE ) == RTX_OK ) {
			switch (receive_header->type) {
				case KEY_IN:{								
					if ( receive_buffer[ MSG_HDR_SIZE ] != '\r' ) {
						if ( command_length < KCD_CMD_BUF_SIZE ) {
							command_buffer[ command_length ] = receive_buffer[ MSG_HDR_SIZE ];
						}
						
						command_length++;
					}
					else {
						if ( command_length > KCD_CMD_BUF_SIZE || command_buffer[ 0 ] != '%' ) {
							printf("KCD INVALID COMMAND LENGTH %u\n", command_length);
							uint8_t string[] = "Invalid command";
							//print_string( string, 15 );
						}
						else {
							size_t i;
							for (i = 0; i < command_length; i++) {
								if ( command_buffer[ i ] == ' ') {
									break;
								}
							}
							
							//printf("\tsize of command %d\r\n", i);
							
							U8 *command_string = (U8 *) mem_alloc( i - 1 );
							for (int j = 1; j < i; j++) {
								*(command_string + j - 1) = command_buffer[ j ];
							}
							
							
							task_t tid = search_command(command_string);
              mem_dealloc(command_string);
							
							if ( tid == 255 ) {
								printf("KCD COMMAND NOT FOUND\n");
								//print_string( "Command not found", 17 );
								print_flag = 1;
							}
							else {
								if (g_tcbs[tid].state == DORMANT) {
									printf("KCD COMMAND NOT FOUND\n");
									uint8_t string[] = "Command not found";
									//print_string( string, 17 );
								}
								else {
									U8* send_buffer = (U8*) mem_alloc( MSG_HDR_SIZE + command_length - 1 );
                                    RTX_MSG_HDR *send_header = (void*) send_buffer;
                                    
                                    send_header->length = MSG_HDR_SIZE + command_length - 1;
                                    send_header->sender_tid = TID_KCD;
                                    send_header->type = KCD_CMD;

                                    for (int i = 0; i < command_length - 1; i++) {
                                        *( send_buffer + MSG_HDR_SIZE + i ) = command_buffer[ i + 1 ];
                                    }

                                    send_msg( tid, send_buffer );
                                    mem_dealloc(send_buffer);
								}
							}
						}
            receive_buffer[ MSG_HDR_SIZE ] = '\n';
						command_length = 0;
					}
					
					send_echo(receive_buffer);
					
					if (print_flag == 1) {
						char *out_string = "Command not found";
						U8* send_buffer = (U8*) mem_alloc(MSG_HDR_SIZE+17+2);
						RTX_MSG_HDR *send_header = (void*) send_buffer;
						
						send_header->length = MSG_HDR_SIZE + 17 + 2;
						send_header->sender_tid = TID_KCD;
						send_header->type = DISPLAY;
						
						for (int i = 0; i < 17; i++) {
							send_buffer[ MSG_HDR_SIZE + i ] = *(out_string + i);
						}
						send_buffer[ MSG_HDR_SIZE + 17 ] = '\n';
						send_buffer[ MSG_HDR_SIZE + 17 + 1 ] = '\0';
						
						printf("KCD SEND BUFFER SIZE %u\n", send_header->length);
						send_msg( TID_CON, send_buffer );
						
						mem_dealloc(send_buffer);
						
						print_flag = 0;
					}
					break;
                }
				
				case KCD_REG:{
                    U8* command_string = (U8*) mem_alloc( receive_header->length - MSG_HDR_SIZE );
					for (int i = 0; i < receive_header->length - MSG_HDR_SIZE; i++) {
						*(command_string + i) = *(receive_buffer + MSG_HDR_SIZE + i);
					}
					insert_command( command_string, receive_header->sender_tid );
					
					// NEED TO GET RID OF THE COMMANDS WE REGISTERED WHEN TASK RIPS
                    //mem_dealloc(command_string);
					break;
                }
                default:
					printf("\t\t\tKCD FOUND AN UNKOWN TYPE\r\n");
            }
        }
        else {
            printf("\t\t\tKCD FAILED RECV\r\n");
        }
    }
    //mem_dealloc(receive_buffer);
}


/*
 *===========================================================================
 *                             END OF FILE
 *===========================================================================
 */

