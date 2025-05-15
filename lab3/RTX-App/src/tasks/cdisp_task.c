/**
 * @brief The Console Display Task Template File 
 * @note  The file name and the function name can be changed
 * @see   k_tasks.h
 */

#include "rtx.h"
#include "k_inc.h"

extern uint8_t g_tx_irq;
extern uint8_t setup_string;

void task_cdisp(void)
{
  mbx_create(CON_MBX_SIZE);
  //uint8_t receive_buffer[ MSG_HDR_SIZE + KCD_CMD_BUF_SIZE + 1 ];
  U8* receive_buffer = (U8*) mem_alloc(MSG_HDR_SIZE+KCD_CMD_BUF_SIZE+1);
  RTX_MSG_HDR *receive_header = (RTX_MSG_HDR *) &receive_buffer[ 0 ];
	
  LPC_UART_TypeDef *pUart = (LPC_UART_TypeDef *) LPC_UART0;
	

	while (TRUE) {
		if ( recv_msg( receive_buffer, MSG_HDR_SIZE + KCD_CMD_BUF_SIZE + 1 ) == RTX_OK ) {
			switch (receive_header->type) {
				case DISPLAY:
					//printf("sending to UART\r\n");
					receive_header->sender_tid = TID_CON;
					//printf("CON RECEIVE BUFFER SIZE %u\n", receive_header->length);
					send_msg(TID_UART_MBX, receive_buffer);
				
					pUart->THR = (uint8_t) receive_buffer[ MSG_HDR_SIZE ]; 
					setup_string = TRUE;
					g_tx_irq = 1;
					pUart->IER |= IER_THRE;
					break;
				
				default:
					printf("\t\t\tCON FOUND AN UNKNOWN TYPE\r\n");
					break;
			}
		}
		else {
			printf("\t\t\tCON FAILED RECV\r\n");
		}
	}

	//mem_dealloc(receive_buffer);
}

