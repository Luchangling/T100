/*  BSP */

#include "delay.h"
#include "usart.h"
#include "sys.h"
#include "bsp.h"
#include "stm32f10x_flash.h"
#include "bms_service_bsp.h"
#include "stm32f10x_iwdg.h"
#include "spi_flash_bsp.h"
void RCC_Configuration(void)
{

  RCC_DeInit();

  RCC_HSEConfig(RCC_HSE_ON);   //RCC_HSE_ON——HSE晶振打开(ON)

  if(RCC_WaitForHSEStartUp() == SUCCESS)        //SUCCESS：HSE晶振稳定且就绪
  {   
    RCC_HCLKConfig(RCC_SYSCLK_Div1);  //RCC_SYSCLK_Div1——AHB时钟 = 系统时钟

    RCC_PCLK2Config(RCC_HCLK_Div1);   //RCC_HCLK_Div1——APB2时钟 = HCLK

    RCC_PCLK1Config(RCC_HCLK_Div2);   //RCC_HCLK_Div2——APB1时钟 = HCLK / 2

    FLASH_SetLatency(FLASH_Latency_2);    //FLASH_Latency_2  2延时周期

    FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);       // 预取指缓存使能

    RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);    

   // PLL的输入时钟 = HSE时钟频率；RCC_PLLMul_9——PLL输入时钟x 9

    RCC_PLLCmd(ENABLE);

    while(RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET) ;    

    RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);

   //RCC_SYSCLKSource_PLLCLK——选择PLL作为系统时钟

    while(RCC_GetSYSCLKSource() != 0x08);        //0x08：PLL作为系统时钟

  }

}

void model_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOC, ENABLE);

    GPIO_DeInit(GPIOA);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0; //.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    GPIO_SetBits(GPIOA,  GPIO_Pin_0);

    //Model Reset Pin
    GPIO_DeInit(GPIOB);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_ResetBits(GPIOB,GPIO_Pin_9);
    //GPIO_SetBits(GPIOB,GPIO_Pin_9);

    //Model Power Key
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8; //.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_ResetBits(GPIOB,GPIO_Pin_8);

    GPIO_DeInit(GPIOC);
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4; //.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
    GPIO_Init(GPIOC, &GPIO_InitStructure);
    //GPIO_ResetBits(GPIOC,GPIO_Pin_4);
    

    //Model Power Key
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5; //.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
    GPIO_Init(GPIOC, &GPIO_InitStructure);

    //GPIO_ResetBits(GPIOC,GPIO_Pin_5);

    //GPIO_SetBits(GPIOC,GPIO_Pin_5);
    TRANS_TTL2MODEL_OE1(0);
    TRANS_MODEL2TTL_OE2(0);
    
}

static u8 s_iwdg_enable = 0;

void bsp_iwdg_init(u8 prer,u16 rlr)
{
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable); /* 使能对寄存器IWDG_PR和IWDG_RLR的写操作*/
    IWDG_SetPrescaler(prer);    /*设置IWDG预分频值:设置IWDG预分频值*/
    IWDG_SetReload(rlr);     /*设置IWDG重装载值*/
    IWDG_ReloadCounter();    /*按照IWDG重装载寄存器的值重装载IWDG计数器*/
    IWDG_Enable();        /*使能IWDG*/
    s_iwdg_enable = 1;
}


void iwdg_feed(void)
{
    if(s_iwdg_enable)
    IWDG_ReloadCounter();
}

void ResetSystem(u8 *string)
{
    s_iwdg_enable = 0;

    cl_log(WARN,"(clock %d)Reset System(%s)",clock(),string);

    //while(1);
}

void bsp_init(void)
{
    RCC_Configuration();
    
    delay_init();

    NVIC_Configuration();

    TIM3_Int_Init();

    model_init();
    
    uart_init(115200,DBG_COM);

    uart_init(115200,MOD_COM);

    bms_uart_init(115200);
    
    bsp_iwdg_init(4,1250);//初始化独立看门狗，分频数为64，重装载值为625，溢出时间计算为：64*1250/40=1000ms=2s

    #if USE_INTERNAL_FLASH == 0
    spi_flash_init();
    #endif
    //uart_init(115200,BMS_COM);
}


