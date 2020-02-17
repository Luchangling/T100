
#include "sensor_service.h"
#include "MPU6050.h"
#include "sys.h"
#include "i2c_driver.h"
#include "stm32f10x_gpio.h"
#include "usart.h"
#define  SENEOR_POWER PBout(2)

typedef struct
{
    u8 sensor_type;

    volatile u32 enit_time;
    
}GsensorControlStruct;

GsensorControlStruct s_sensor;


void sensor_service_create(void)
{
    GPIO_InitTypeDef GPIO_InitStructure; 

    SENEOR_POWER = 1;
    
    Sensor_I2C_Init();

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    SENEOR_POWER = 0;

    memset((u8 *)&s_sensor,0,sizeof(GsensorControlStruct));
}

void sensor_service_proc(void)
{
    static u8 count = 0;

    u16 aclr[3] = {0};

    u8 config = 0;
    
    if(s_sensor.sensor_type == 0)
    {
        Sensor_I2C_BufferRead(0x31 , &s_sensor.sensor_type,0x0F,1);

        LOG(INFO,"clock(%d) Gsensor ID %x",clock(),s_sensor.sensor_type);  

        config = 0x67;
        Sensor_I2C_ByteWrite(0x30,&config,0x20);

	    //  高精度模式 //2G 
        config = 0x88;
        Sensor_I2C_ByteWrite(0x30,&config,0x23);
	    //  高精度模式
        config = 0x31;
        Sensor_I2C_ByteWrite(0x30,&config,0x21);
        config = 0x40;
        Sensor_I2C_ByteWrite(0x30,&config,0x22);
        config = 0x00;
        Sensor_I2C_ByteWrite(0x30,&config,0x25);
        config = 0x00;
        Sensor_I2C_ByteWrite(0x30,&config,0x24);
        config = 0x2A;
        Sensor_I2C_ByteWrite(0x30,&config,0x30);
        config = 1;
        Sensor_I2C_ByteWrite(0x30,&config,0x32);
        config = 0;
        Sensor_I2C_ByteWrite(0x30,&config,0x33);
    }
    else
    {
        if(++count > 100)
        {
            Sensor_I2C_BufferRead(0x30,(u8 *)aclr,0xA8, 6);

            //LOG(INFO,"clock(%d) Gsensor %x  %x   %x",clock(),aclr[0],aclr[1],aclr[2]);
        }
    }
}

void sensor_service_destory(void)
{
    
}

