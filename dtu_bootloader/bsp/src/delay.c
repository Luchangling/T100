/*
    Delay function BSP Support
    creator : chris.lu
*/

#include "system_stm32f10x.h"
#include "stm32f10x.h"
#include "sys.h"
static u32 g_us_count_multiplier = 0;
static u32 g_ms_count_multiplier = 0;

void delay_init()	 
{
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//选择外部时钟  HCLK/8
	
	g_us_count_multiplier = SystemCoreClock/8000000;	//为系统时钟的1/8  
	 
	g_ms_count_multiplier = (u16)g_us_count_multiplier*1000;//非ucos下,代表每个ms需要的systick时钟数   
}								    


//延时nus
//nus为要延时的us数.		    								   
void delay_us(u32 nus)
{		
	u32 temp;
    
	SysTick->LOAD = nus*g_us_count_multiplier; //时间加载	
	
	SysTick->VAL = 0x00;        //清空计数器
	
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk ;          //开始倒数	
	
	do
	{
		temp = SysTick->CTRL;
        
	}while(temp&0x01&&!(temp&(1<<16)));//等待时间到达
	
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //关闭计数器
	
	SysTick->VAL =0X00;       //清空计数器	 
}
//延时nms
//注意nms的范围
//SysTick->LOAD为24位寄存器,所以,最大延时为:
//nms<=0xffffff*8*1000/SYSCLK
//SYSCLK单位为Hz,nms单位为ms
//对72M条件下,nms<=1864 
void delay_ms(u16 nms)
{	 		  	  
	u32 temp;	
    
	SysTick->LOAD = (u32)nms*g_ms_count_multiplier;//时间加载(SysTick->LOAD为24bit)
	
	SysTick->VAL = 0x00;           //清空计数器
	
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk ;          //开始倒数  
	
	do
	{
		temp=SysTick->CTRL;
	}while(temp&0x01&&!(temp&(1<<16)));//等待时间到达  
	
	SysTick->CTRL&=~SysTick_CTRL_ENABLE_Msk;       //关闭计数器
	
	SysTick->VAL = 0X00;       //清空计数器	  	    
} 




