
#include "task_config.h"
#include "usart.h"
#include "timer.h"
#include "sys.h"
#include "net_task.h"
#include "daemon_task.h"
#include "at_command.h"
#include "gprs.h"
#include "bsp.h"
#include "config_service.h"
#include "sensor_service.h"
#include "led_service.h"
#include "device.h"
//OS_STK START_TASK_STK[START_STK_SIZE];

OS_STK NET_TASK_STK[NET_STK_SIZE];

OS_STK AT_TASK_STK[AT_STK_SIZE];

OS_STK EXT_DEV_TASK_STK[EXT_DEVICE_STK_SIZE];

int main(void)
{    
    bsp_init();

    led_service_create();

    config_service_create();

    gprs_create();

    sensor_service_create();

    OSInit();  	 			//初始化UCOSII
			  
    OSTaskCreate(model_task,(void *)0,(OS_STK *)&NET_TASK_STK[NET_STK_SIZE-1],NET_TASK_PRIO );//

    OSTaskCreate(at_task,(void *)0,(OS_STK *)&AT_TASK_STK[AT_STK_SIZE-1],AT_TASK_PRIO );// AT任务会pending其他任务。优先级最高

    OSTaskCreate(ext_device_task,(void *)0,(OS_STK *)&EXT_DEV_TASK_STK[EXT_DEVICE_STK_SIZE-1],EXT_DEVICE_TASK_PRIO );//
    
    OSStart();
}





