#include "usart.h"	  
#include "sys.h"
#include "fifo.h"
#include "stm32f10x_dma.h"
#include <stdarg.h>
#include "fifo.h"


#define DBG_UART UART4

#define MOD_UART USART1

#define USART1_DMA_TX_CHANNEL DMA1_Channel4

#define DBG_UART_DMA_CHANNEL DMA2_Channel5


//#define __SUPPORT_MOD_DMA__

u16 dbg_com_write(u8 *data , u16 len);

//char s_uart1_buff[UART1_DMA_SIZE] = {0};


u8 *log_level_string[ATSTR + 1] = {
    "DEBUG",
    "INFO",
    "WARN",
    "ERROR",
    "FATAL",
    NULL,
};

u8 g_log_level = 0;

#define MAX_DBG_FIFO_SIZE 2048

#define MAX_DBG_RD_FIFO_LEN 500
char s_dbg_buff[MAX_DBG_FIFO_SIZE] = {0};



//char s_dbg_rd_buff[MAX_DBG_RD_FIFO_LEN] = {0};

OS_EVENT *s_log_sem = NULL;

void cl_log_hex(u8 *data,u16 len, const char * format, ...)
{  
    //static u16 send_len  = 0;

    //INT8U err;
    
    u16 i,jval = 0; 
    
    va_list args;  
    
    va_start (args, format);

    if(s_log_sem == NULL)
    {
        s_log_sem = OSSemCreate(1);
    }

    //OSSemPend(s_log_sem, 0, &err);
    

    while(DMA_GetCurrDataCounter(DBG_UART_DMA_CHANNEL));

    DMA_Cmd(DBG_UART_DMA_CHANNEL, DISABLE);

    //jval = DMA_GetCurrDataCounter(DBG_UART_DMA_CHANNEL);

    while(DMA_GetCurrDataCounter(DMA1_Channel7));

    jval += vsprintf(&s_dbg_buff[jval],format, args);  

    va_end (args);

    for(i = 0 ; i < len; i++)
    {
        jval += sprintf(&s_dbg_buff[jval],"%02x",data[i]);
    }
   

    jval += sprintf(&s_dbg_buff[jval],"\r\n");


    //send_len = jval;

    DMA_SetCurrDataCounter(DBG_UART_DMA_CHANNEL,jval);
    
    DMA_Cmd(DBG_UART_DMA_CHANNEL, ENABLE);  // 启动DMA发送

    //OSSemPost(s_log_sem);
}


void cl_log(LogLevelEnum level, const char * format, ... )
{  
   // static u16 send_len  = 0;

    // err;
    
    u16 jval = 0; 
    
    va_list args;  

    if(level < g_log_level) return;

    if(s_log_sem == NULL)
    {
        s_log_sem = OSSemCreate(1);
    }
    //OSSemPend(s_log_sem, 0, &err);
 
    while(DMA_GetCurrDataCounter(DBG_UART_DMA_CHANNEL));

    DMA_Cmd(DBG_UART_DMA_CHANNEL, DISABLE);

    va_start (args, format);

    //while(DMA_GetCurrDataCounter(DMA1_Channel7));

    if(log_level_string[level] != NULL)
    jval += sprintf(&s_dbg_buff[jval],"[%s]",log_level_string[level]);

    jval += vsprintf(&s_dbg_buff[jval],format, args);  

    if(log_level_string[level] != NULL)
    jval += sprintf(&s_dbg_buff[jval],"\r\n");

    va_end (args);

    s_dbg_buff[jval] = 0;

    //printf(s_dbg_buff);

    //dbg_com_write(s_dbg_buff,strlen(s_dbg_buff));

    DMA_SetCurrDataCounter(DBG_UART_DMA_CHANNEL,jval);
    
    DMA_Cmd(DBG_UART_DMA_CHANNEL, ENABLE);  // 启动DMA发送

    //OSSemPost(s_log_sem);
}
   
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{
	USART_SendData(DBG_UART, (uint8_t) ch);

	while (USART_GetFlagStatus(DBG_UART, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
#endif 


u16 mod_com_write(u8 *data , u16 len)
{
    u16 timeout = 0 , i = 0;

    for(i = 0; i < len ; i++)
    {
        USART_SendData(MOD_UART, data[i]);

        timeout = 0;
        
        while (USART_GetFlagStatus(MOD_UART, USART_FLAG_TC) == RESET)
        {
            timeout++;

            if(timeout >= 0xFFFD)break;
        }

        if(timeout >= 0xFFFD)break;
    }

    return i;
}

u16 dbg_com_write(u8 *data , u16 len)
{
    u16 timeout = 0 , i = 0;

    for(i = 0; i < len ; i++)
    {
        USART_SendData(DBG_UART, data[i]);

        timeout = 0;
        
        while (USART_GetFlagStatus(DBG_UART, USART_FLAG_TC) == RESET)
        {
            timeout++;

            if(timeout >= 0xFFFD)break;
        }

        if(timeout >= 0xFFFD)break;
    }

    return i;
}


Uart_Read_Struct s_uart1;
Uart_Read_Struct s_dbg;



static void _dbg_uart_tx_dma_configuration()
{
  DMA_InitTypeDef DMA_InitStructure;
 
  /* DMA1 Channel6 (triggered by USART1 Rx event) Config */
  RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA2 ,ENABLE);
 
  /* DMA1 Channel5 (triggered by USART1 Rx event) Config */
  DMA_DeInit(DMA2_Channel5);
  DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)(&UART4->DR);
  DMA_InitStructure.DMA_MemoryBaseAddr =(u32)s_dbg_buff;
  DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;
  DMA_InitStructure.DMA_BufferSize = MAX_DBG_FIFO_SIZE;
  DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  
  DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
  DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  
  DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
  DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;
  DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
  DMA_InitStructure.DMA_M2M = DMA_M2M_Disable; 
  DMA_Init(DMA2_Channel5, &DMA_InitStructure);

  DMA_SetCurrDataCounter(DBG_UART_DMA_CHANNEL,0);
 
  DMA_Cmd(DMA2_Channel5, ENABLE);

  USART_DMACmd(UART4, USART_DMAReq_Tx, ENABLE);

  
  
}



u16 mod_channel_rec_data_len(void)
{
   #ifdef __SUPPORT_MOD_DMA__
   return (u16)(UART1_DMA_SIZE - DMA_GetCurrDataCounter(DMA1_Channel5));
   #endif

   return fifo_GetLen(&s_uart1.fifo);
}

u8* mod_channel_recv_dma_addr(void)
{
    #ifdef __SUPPORT_MOD_DMA__
    return s_uart1.buff;
    #endif
    return NULL;
}


u8 is_mod_channel_read_complete(void)
{
    #ifdef __SUPPORT_MOD_DMA__
    return ((s_uart1.readflag&(1<<READ_COMPLETE))?1:0);
    #endif

    return s_uart1.bus_is_idle;
    //return (OSTimeGet() - s_uart1.last_get_time > 1?1:0);
}

void clear_mod_channel_read_complete(void)
{
    #ifdef __SUPPORT_MOD_DMA__
    //return ((s_uart1.readflag&READ_COMPLETE)?1:0);
    s_uart1.readflag = 0;
    #endif
}

void mod_channel_read_stoped(void)
{
    #ifdef __SUPPORT_MOD_DMA__
    DMA_Cmd(DMA1_Channel5, DISABLE);
    #endif
}

void mod_channel_clear(void)
{
    #ifdef __SUPPORT_MOD_DMA__
    memset(s_uart1.buff,0,UART1_DMA_SIZE);
    DMA_Cmd(DMA1_Channel5, DISABLE);
    DMA1_Channel5->CNDTR=UART1_DMA_SIZE ;
    DMA_Cmd(DMA1_Channel5, ENABLE);
    s_uart1.readflag = 0;
    #endif
}

u32 mod_channel_last_get_time(void)
{
   return s_uart1.last_get_time; 
}

fifo_TypeDef *mod_channel_fifo_addr(void)
{
    return (fifo_TypeDef *)&s_uart1.fifo;
}

fifo_TypeDef *dbg_channel_fifo_addr(void)
{
    return (fifo_TypeDef *)&s_dbg.fifo;
}


void uart_init(u32 baundrate,UART_TYPE uart)
{
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
    u8 data;
    switch(uart)
    {
        case DBG_COM:
            {
                //USART_Cmd(UART4,DISABLE);
                //USART_ITConfig(UART4, USART_IT_ORE|USART_IT_RXNE, DISABLE);//
                //delay_ms(10);
                RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
                RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

                //USART2_TX   PA.2
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //.2
                GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	
                GPIO_Init(GPIOC, &GPIO_InitStructure);

                //USART2_RX	  PA.3
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
                GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
                GPIO_Init(GPIOC, &GPIO_InitStructure);  


                NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
                NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;
                NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		
                NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			
                NVIC_Init(&NVIC_InitStructure);	

                //USART 

                USART_InitStructure.USART_BaudRate = baundrate;
                USART_InitStructure.USART_WordLength = USART_WordLength_8b;
                USART_InitStructure.USART_StopBits = USART_StopBits_1;
                USART_InitStructure.USART_Parity = USART_Parity_No;
                USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
                USART_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;	

                USART_Init(UART4, &USART_InitStructure); //
                USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//
                //USART_ITConfig(UART4, USART_IT_ORE, ENABLE);//

                data = data;
                //data = UART4->DR;
                //data = UART4->SR;

                //s_dbg.fifo.baseAddr = (u8 *)s_dbg_rd_buff;

                fifo_Init(&s_dbg.fifo, MAX_DBG_RD_FIFO_LEN);
                
                USART_Cmd(UART4, ENABLE);

                _dbg_uart_tx_dma_configuration();
                break;
            }
    
        case MOD_COM:
            {
                RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);	//使能USART1，GPIOB时钟以及复用功能时钟
                GPIO_PinRemapConfig(GPIO_Remap_USART1,ENABLE);

                //USART1_TX   PB.6
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6; //PA.6
                GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
                GPIO_Init(GPIOB, &GPIO_InitStructure);

                //USART1_RX	  PB.7
                GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
                GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
                GPIO_Init(GPIOB, &GPIO_InitStructure);  

                //Usart1 NVIC 配置

                NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
                NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
                NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;		//子优先级3
                NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
                NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化  NVIC寄存器

                //USART 初始化设置

                USART_InitStructure.USART_BaudRate = baundrate;
                USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
                USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
                USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
                USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
                USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

                USART_Init(USART1, &USART_InitStructure); //初始化串口
                USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断

                data = data;
                data = USART1->DR;
                data = USART1->SR;

                fifo_Init(&s_uart1.fifo, UART1_DMA_SIZE);
                
                USART_Cmd(USART1, ENABLE);

                //s_uart1.fifo.baseAddr = (u8 *)s_uart1_buff;           

                //_uart1_dma_configuration();
                break;
            }
        default:
            break;
    }
}


void USART1_IRQHandler(void)                	//
{
	u8 Res;
    #ifdef OS_TICKS_PER_SEC	 
	OSIntEnter();    
    #endif
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //
	{
		Res =USART_ReceiveData(USART1);//(USART1->DR);	//

        USART_ITConfig(USART1,USART_IT_IDLE,ENABLE);

        //s_uart1.last_get_time = OSTime;//OSTimeGet();

        s_uart1.bus_is_idle = 0;

        fifo_insert(&s_uart1.fifo, &Res, 1);
    }
    else if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
    {
        Res = USART1->DR;
        //Res = USART1->SR;
        USART_ITConfig(USART1,USART_IT_IDLE,DISABLE);

        fifo_insert(&s_uart1.fifo, &Res, 1);

        s_uart1.bus_is_idle = 1;
    }
    else
    {
        Res =USART_ReceiveData(USART1);
    }
    #ifdef OS_TICKS_PER_SEC	 	
	OSIntExit();  											 
    #endif
} 


void UART4_IRQHandler(void)                	//
{
    //U32 reg;
	u8 Res;
    #ifdef OS_TICKS_PER_SEC	 
	OSIntEnter();    
    #endif
    #if 1

   
   if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)  //
	{
        //reg = UART4->CR1;

        //reg = UART4->SR;
        Res =USART_ReceiveData(UART4);//(USART1->DR);	//

       // reg = UART4->CR1;

       // reg = UART4->SR;

        
        
        //USART_ClearFlag(UART4,USART_IT_RXNE);
        
        //USART_ClearITPendingBit(UART4,USART_IT_RXNE);

        fifo_insert(&s_dbg.fifo,&Res,1);
        
		
    }
    else  if(USART_GetITStatus(UART4, USART_IT_ORE) != RESET)
    {
        

        Res =USART_ReceiveData(UART4);//(USART1->DR);	//

        USART_ClearITPendingBit(UART4,USART_IT_ORE);

        USART_ClearFlag(UART4, USART_IT_ORE);

        
    }
    else
    {
        Res =USART_ReceiveData(UART4);
    }
    #else
    
    if (USART_GetFlagStatus(UART4, USART_FLAG_PE) != RESET)
    {
        USART_ReceiveData(UART4);
        USART_ClearFlag(UART4, USART_FLAG_PE);
    }
           
    if (USART_GetFlagStatus(UART4, USART_FLAG_ORE) != RESET)
    {
        USART_ReceiveData(UART4);
        USART_ClearFlag(UART4, USART_FLAG_ORE);
    }
           
    if (USART_GetFlagStatus(UART4, USART_FLAG_FE) != RESET)
    {
        USART_ReceiveData(UART4);
        USART_ClearFlag(UART4, USART_FLAG_FE);
    }

    if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)  //
	{
        reg = UART4->CR1;

        reg = UART4->SR;
        
        USART_ClearFlag(UART4,USART_IT_RXNE);
        
        USART_ClearITPendingBit(UART4,USART_IT_RXNE);
        
		Res =USART_ReceiveData(UART4);//(USART1->DR);	//

        reg = UART4->CR1;

        reg = UART4->SR;

        fifo_insert(&s_dbg.fifo,&Res,1);
    }
    
    #endif
    #ifdef OS_TICKS_PER_SEC	 	
	OSIntExit();  											 
    #endif
}


