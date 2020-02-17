
/*BMS BSP Support*/

#include "bms_service_bsp.h"
#include "stm32f10x_dma.h"


#define BMS_UART USART2

#define MAX_BMS_INFO_SIZE 1024

//char s_bms_buff[MAX_BMS_INFO_SIZE] = {0};

u8 s_bms_rx_buf[100] = {0};

Uart_Read_Struct s_bms;

fifo_TypeDef *bms_channel_fifo_addr(void)
{
    return (fifo_TypeDef *)&s_bms.fifo;
}

static void _bms_uart_rx_dma_configration(void)
{
  DMA_InitTypeDef DMA_InitStructure;
 
  /* DMA1 Channel6 (triggered by USART1 Rx event) Config */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1 ,ENABLE);
 
  /* DMA1 Channel5 (triggered by USART1 Rx event) Config */
  DMA_DeInit(DMA1_Channel6);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&USART2->DR);
  DMA_InitStructure.DMA_MemoryBaseAddr =(u32)s_bms_rx_buf;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
  DMA_InitStructure.DMA_BufferSize = sizeof(s_bms_rx_buf);//MAX_BMS_INFO_SIZE;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 
  DMA_Init(DMA1_Channel6, &DMA_InitStructure);
 
  DMA_Cmd(DMA1_Channel6, ENABLE);

  USART_DMACmd(USART2, USART_DMAReq_Rx, ENABLE);
  
}


void bms_uart_init(u32 baundrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
    

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    //USART2_TX   PA.2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    //USART2_RX	  PA.3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//
    GPIO_Init(GPIOA, &GPIO_InitStructure);  

    //Usart2 NVIC

    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//
    NVIC_Init(&NVIC_InitStructure);	//

    //USART 

    USART_InitStructure.USART_BaudRate = baundrate;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;//
    USART_InitStructure.USART_StopBits = USART_StopBits_1;//
    USART_InitStructure.USART_Parity = USART_Parity_No;//
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//

    USART_Init(BMS_UART, &USART_InitStructure); //
    USART_ClearFlag(BMS_UART, USART_FLAG_RXNE);
    USART_ITConfig(BMS_UART, USART_IT_RXNE, ENABLE);//

    //_bms_uart_rx_dma_configration();
    
    //s_bms.fifo.baseAddr = (u8 *)s_bms_buff;

    fifo_Init(&s_bms.fifo,MAX_BMS_INFO_SIZE);
    
    USART_Cmd(BMS_UART, ENABLE);
    
    
}

u16 bms_com_write(const u8 *data , u16 len)
{
    u16 timeout = 0 , i = 0;

    for(i = 0; i < len ; i++)
    {
        timeout = 0;
        
        while (USART_GetFlagStatus(BMS_UART, USART_FLAG_TC) == RESET)
        {
            timeout++;

            if(timeout >= 0xFFFD)break;
        }

        if(timeout >= 0xFFFD)break;

        USART_SendData(BMS_UART, data[i]);
    }

    return i;
}


void USART2_IRQHandler(void)                	//
{
	u8 Res;
    U16 len  = 0;
    
    #ifdef OS_TICKS_PER_SEC	 
	OSIntEnter();    
    #endif
    if(USART_GetITStatus(BMS_UART, USART_IT_RXNE) != RESET)
    {
        Res =USART_ReceiveData(BMS_UART);//(USART1->DR);	

        fifo_insert(&s_bms.fifo,&Res,1);

    }
    #if 1
    else if(USART_GetITStatus(USART2, USART_IT_IDLE) != RESET)
    {
        Res = USART2->DR;
        Res = USART2->SR;
        
        Res = Res;

        DMA_Cmd(DMA1_Channel6, DISABLE);

        len  = 100 - DMA_GetCurrDataCounter(DMA1_Channel6);

        if(len  > 0)
        {
            //bms_service_data_que_insert_one((u8 *)s_bms_buff,len);
            fifo_insert(&s_bms.fifo,s_bms_rx_buf,len);
        }

        DMA1_Channel6->CNDTR = 100;

        DMA_Cmd(DMA1_Channel6, ENABLE);

        
    }
    #endif
    else
    {
        Res =USART_ReceiveData(BMS_UART);
    }
    #ifdef OS_TICKS_PER_SEC	 	
	OSIntExit();  											 
    #endif
}



