
//led service
#include "led_service.h"
#include "stm32f10x_gpio.h"

#define GPS_LED_RCC_APB RCC_APB2Periph_GPIOA
#define GPS_LED_BASE GPIOA
#define GPS_LED_PIN  GPIO_Pin_9
#define GPS_LED      PAout(9)

#define NET_LED_RCC_APB RCC_APB2Periph_GPIOA
#define NET_LED_BASE GPIOA
#define NET_LED_PIN  GPIO_Pin_8
#define NET_LED      PAout(8)

#define LED_LIGHT    1

#define LED_TIMER_FQ  5
#define LED_FAST_FLASH_INTERVAL 200/LED_TIMER_FQ
#define LED_SLOW_FLASH_INTERVAL 1000/LED_TIMER_FQ

typedef struct
{
    LedFlashTypeEnum gps_flash;

    LedFlashTypeEnum net_flash;

    u16 gps_led_timer;

    u16 net_led_timer;
    
}LedStateControlStruct;

LedStateControlStruct s_led;

void led_service_create(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(GPS_LED_RCC_APB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPS_LED_PIN; //.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
    GPIO_Init(GPS_LED_BASE, &GPIO_InitStructure);
    GPS_LED = !LED_LIGHT;

    GPIO_InitStructure.GPIO_Pin = NET_LED_PIN; //.2
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;	
    GPIO_Init(NET_LED_BASE, &GPIO_InitStructure);
    NET_LED = !LED_LIGHT;

    s_led.gps_flash = LED_OFF;
    s_led.net_flash = LED_OFF;
    s_led.gps_led_timer = 0;
    s_led.net_led_timer = 0;
    
}


//此函数在中断中调用5ms一次
void led_serivce_time_proc(void)
{
    
    if(s_led.gps_flash == LED_FAST_FLASH)
    {
        if(++s_led.gps_led_timer >= LED_FAST_FLASH_INTERVAL)
        {
            s_led.gps_led_timer = 0;
            GPS_LED = !GPS_LED;
        }
    }
    else if(s_led.gps_flash == LED_SLOW_FLASH)
    {
        if(++s_led.gps_led_timer >= LED_SLOW_FLASH_INTERVAL)
        {
            s_led.gps_led_timer = 0;
            GPS_LED = !GPS_LED;
        }
    }

    if(s_led.net_flash == LED_FAST_FLASH)
    {
        if(++s_led.net_led_timer >= LED_FAST_FLASH_INTERVAL)
        {
            s_led.net_led_timer = 0;
            NET_LED = !NET_LED;
        }
    }
    else if(s_led.net_flash == LED_SLOW_FLASH)
    {
        if(++s_led.net_led_timer >= LED_SLOW_FLASH_INTERVAL)
        {
            s_led.net_led_timer = 0;
            NET_LED = !NET_LED;
        }
    }
}

void led_service_gps_set(LedFlashTypeEnum type)
{
    if(s_led.gps_flash ^ type)
    {
        s_led.gps_flash = type;
        
        if(s_led.gps_flash == LED_OFF)
        {
            GPS_LED = !LED_LIGHT;
        }
        else if(s_led.gps_flash == LED_ON)
        {
            GPS_LED = LED_LIGHT;
        }
    }
}

void led_service_net_set(LedFlashTypeEnum type)
{
    if(s_led.net_flash ^ type)
    {
        s_led.net_flash = type;
        
        if(s_led.net_flash == LED_OFF)
        {
            NET_LED = !LED_LIGHT;
        }
        else if(s_led.net_flash == LED_ON)
        {
            NET_LED = LED_LIGHT;
        }
    }
}


void led_service_destory(void)
{
    
    s_led.gps_flash = LED_OFF;
    s_led.net_flash = LED_OFF;
    s_led.gps_led_timer = 0;
    s_led.net_led_timer = 0;

    NET_LED = !LED_LIGHT;
    GPS_LED = !LED_LIGHT;
}

