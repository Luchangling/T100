#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h"
#include "fifo.h"

typedef enum
{
  DBG_COM,
  BMS_COM,
  MOD_COM,
  MAX_COM
}UART_TYPE;

typedef enum
{
    DEBUG,
    INFO,
    WARN,
    ERR,
    FATAL,
    ATSTR
}LogLevelEnum;

#define UART1_DMA_SIZE 1600

typedef struct
{
    u8 inited;
    
}Uart_Profile_Struct;

typedef enum
{
    READ_COMPLETE,

    READ_ERROR
    
}UartReadFlagEnum;

typedef struct
{
    volatile u8 bus_is_idle;
    volatile u32 last_get_time;
    //u8 buff[UART1_DMA_SIZE];
    fifo_TypeDef fifo;
}Uart_Read_Struct;

extern u8 g_log_level;

extern void USART1_IRQHandler(void);

extern void uart_init(u32 baundrate,UART_TYPE uart);

extern int printf(const char * format, ... );

extern void mod_channel_clear(void);

extern u8* mod_channel_recv_dma_addr(void);

extern u16 mod_channel_rec_data_len(void);

extern u16 mod_com_write(u8 *data , u16 len);

extern u8 is_mod_channel_read_complete(void);

extern void cl_log(LogLevelEnum level, const char * format, ... );

extern void cl_log_hex(u8 *data,u16 len, const char * format, ...);

extern void clear_mod_channel_read_complete(void);

extern void mod_channel_read_stoped(void);

extern fifo_TypeDef *mod_channel_fifo_addr(void);

extern fifo_TypeDef *dbg_channel_fifo_addr(void);



#endif


