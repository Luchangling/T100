#include "delay.h"
#include "sys.h"
#include "led_service.h"
#include "includes.h"					//ucos

	 
static u32 g_us_count_multiplier = 0;
static u32 g_ms_count_multiplier = 0;

//systick中断服务函数,使用ucos时用到
void SysTick_Handler(void)
{				   
	OSIntEnter();		//进入中断
    OSTimeTick();       //调用ucos的时钟服务程序               
    OSIntExit();        //触发任务切换软中断
    led_serivce_time_proc();
}


//初始化延迟函数
//当使用ucos的时候,此函数会初始化ucos的时钟节拍
void delay_init()	 
{

    #ifdef OS_CRITICAL_METHOD 	//如果OS_CRITICAL_METHOD定义了,说明使用ucosII了.
	u32 reload;
    #endif
	SysTick_CLKSourceConfig(SysTick_CLKSource_HCLK_Div8);	//选择外部时钟  HCLK/8
	
	g_us_count_multiplier = SystemCoreClock/8000000;	//为系统时钟的1/8  
	 
	reload = SystemCoreClock/8000000;		//每秒钟的计数次数 单位为K	   
	reload *= 1000000/OS_TICKS_PER_SEC;//根据OS_TICKS_PER_SEC设定溢出时间
							//reload为24位寄存器,最大值:16777216,在72M下,约合1.86s左右	
	g_ms_count_multiplier = 1000/OS_TICKS_PER_SEC;//代表ucos可以延时的最少单位	   
	SysTick->CTRL |= SysTick_CTRL_TICKINT_Msk;   	//开启SYSTICK中断
	SysTick->LOAD  = reload; 	//每1/OS_TICKS_PER_SEC秒中断一次	
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;   	//开启SYSTICK    
}								    

//延时nus
//nus为要延时的us数.		    								   
void delay_us(u32 nus)
{		
	u32 ticks;
	u32 told,tnow,tcnt=0;
	u32 reload=SysTick->LOAD;	//LOAD的值	    	 
	ticks=nus*g_us_count_multiplier; 			//需要的节拍数	  		 
	tcnt=0;
	told=SysTick->VAL;        	//刚进入时的计数器值
	while(1)
	{
		tnow=SysTick->VAL;	
		if(tnow!=told)
		{	    
			if(tnow<told)tcnt+=told-tnow;//这里注意一下SYSTICK是一个递减的计数器就可以了.
			else tcnt+=reload-tnow+told;	    
			told=tnow;
			if(tcnt>=ticks)break;//时间超过/等于要延迟的时间,则退出.
		}  
	}; 									    
}
//延时nms
//nms:要延时的ms数
void delay_ms(u32 nms)
{	
	if(OSRunning == true)//如果os已经在跑了	    
	{		  
		if(nms>=g_ms_count_multiplier)//延时的时间大于ucos的最少时间周期 
		{
   			OSTimeDly(nms/g_ms_count_multiplier);//ucos延时
		}
		nms%=g_ms_count_multiplier;				//ucos已经无法提供这么小的延时了,采用普通方式延时    
	}
	delay_us((u32)(nms*1000));	//普通方式延时,此时ucos无法启动调度.
}



