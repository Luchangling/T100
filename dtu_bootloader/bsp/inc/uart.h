#ifndef __UART_H__
#define __UART_H__

extern void USART1_IRQHandler(void);

extern uart_init(u32 bound);

extern int printf(const char * format, ... );


void uart_write(u8 *str , u16  len);


#endif

