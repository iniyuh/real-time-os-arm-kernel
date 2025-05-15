#ifndef UART_H
#define UART_H

int uart_irq_init(int n_uart);
void UART0_IRQHandler(void);

#endif //UART_H