
#ifndef __BMS_SERVICE_BSP_H__
#define __BMS_SERVICE_BSP_H__

#include "sys.h"
#include "usart.h"
#include "stm32f10x_usart.h"
#include "fifo.h"


fifo_TypeDef *bms_channel_fifo_addr(void);

void bms_uart_init(u32 baundrate);

u16 bms_com_write(const u8 *data , u16 len);

#endif

