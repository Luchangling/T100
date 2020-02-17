#include "stm32f10x_rcc.h"
#include "core_cm3.h"
#include "uart.h"
#include "delay.h"
#include "spi_flash_bsp.h"
#include "sys.h"
#include "iap.h"

typedef  void (*iapfun)(void);	

iapfun jump2app;

__asm void MSR_MSP(uint32_t addr)
{
    MSR MSP, r0
    BX r14
}

void jump_app(uint32_t addr)
{
    uint8_t i = 0;

    printf("READ data %x\r\n",(*(volatile uint32_t *)addr));
    
    if(((*(volatile uint32_t *)addr) & 0x2FFE0000) == 0x20000000) 
    {
        
        jump2app=(iapfun)*(volatile uint32_t *)(addr + 4); // 
        
        MSR_MSP(*(volatile uint32_t *)addr); //
        
        for(i = 0; i < 8; i++) 
        {
            NVIC->ICER[i] = 0xFFFFFFFF; //
            NVIC->ICPR[i] = 0xFFFFFFFF; //
        }
        RCC_DeInit();
        USART_DeInit(UART4);
        // DeInit();
        SysTick->CTRL = 0;
        SysTick->LOAD = 0;
        SysTick->VAL = 0;
        jump2app(); // APP
    }
}

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
#define BOOT_VERSION "BootLoader v1.0.0"

int main(void) 
{ 
    u32 count = 0;

    u16 id;

    //u8 ii = 0;

    RCC_Configuration();

    NVIC_Configuration();

    uart_init(115200);

    delay_init();


    bsp_iwdg_init(4,1250);//初始化独立看门狗，分频数为64，重装载值为625，溢出时间计算为：64*1250/40=1000ms=2s

    while(1)
    {

        iwdg_feed();

        printf("=====Bye bye %s===\r\n",BOOT_VERSION);

        iwdg_feed();

        jump_app(APP_ENTRY_ADDR);
    }
	//jump_app(APP_ENTRY_ADDR);
} 

