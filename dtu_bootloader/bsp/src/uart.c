/*
    UART BSP Support
    creator :chris.lu 
*/
#include "stm32f10x_usart.h"
#include "stm32f10x_rcc.h"
#include "stdio.h"

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
	USART_SendData(UART4, (uint8_t) ch);

	while (USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}

void USART1_IRQHandler(void)
{
	u8 res;	
	res = res;
 	if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)//接收到数据
	{	 
		res = USART_ReceiveData(UART4); 
	}
    else
    {
        res = USART_ReceiveData(UART4);
    }
 } 


void uart_write(u8 *str , u16  len)
{
    u16  i = 0;

    for(i = 0; i < len ; i++)
    {
        USART_SendData(UART4, str[i]);

	    while (USART_GetFlagStatus(UART4, USART_FLAG_TC) == RESET);
    }
}


void uart_init(u32 bound)
{
    u8 data;
    
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
    
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_UART4, ENABLE);

    //USART2_TX   PA.2
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; 
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    //USART2_RX   PA.3
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOC, &GPIO_InitStructure);  


    NVIC_InitStructure.NVIC_IRQChannel = UART4_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;      
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;         
    NVIC_Init(&NVIC_InitStructure); 

    //USART 

    USART_InitStructure.USART_BaudRate = bound;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx; 

    USART_Init(UART4, &USART_InitStructure); //
    USART_ITConfig(UART4, USART_IT_RXNE, ENABLE);//
    USART_ITConfig(UART4, USART_IT_ORE, ENABLE);//

    data = data;

    data = UART4->DR;
    data = UART4->SR;
    
    USART_Cmd(UART4, ENABLE);

}


void UART4_IRQHandler(void)                	//
{
	u8 Res;
	Res = Res;
	if(USART_GetITStatus(UART4, USART_IT_RXNE) != RESET)  //
	{

        USART_ClearITPendingBit(UART4,USART_IT_RXNE);
        
		Res =USART_ReceiveData(UART4);//(USART1->DR);	//

        //fifo_insert(&s_dbg.fifo,&Res,1);
    }
    else if(USART_GetITStatus(UART4, USART_IT_ORE) != RESET)
    {
        USART_ClearITPendingBit(UART4,USART_IT_ORE);

        Res =USART_ReceiveData(UART4);//(USART1->DR);	//
    }
    else
    {
        Res =USART_ReceiveData(UART4);
    }
}



