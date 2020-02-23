
#include "sys.h"
#include "bms_service.h"
#include "command.h"
#include "bsp.h"

static u8 s_water_level = 0;

void read_water_box_level_proc(void)
{
    static u8 timeout = 0;

    u8 level = bsp_read_water_level();

    if(s_water_level != level)
    {
        timeout = 30;
    }
    else if(timeout == 0)
    {
        s_water_level = level;

        timeout = 30;
    }

    if(timeout)timeout--;
    
}

u8 get_water_level_res(void)
{
    u8 level[5] = {0,20,50,80,100};

    if(s_water_level < 5)
    {
        return level[s_water_level];
    }

    return 0;
}

void ext_device_task(void *pdata)
{
    pdata = pdata;

    bms_service_create();

    s_water_level = bsp_read_water_level();

    //bms_service_data_que_init();

    while(1)
    {
        debug_port_command_process();
            
        bms_service_timer_proc();

        read_water_box_level_proc();

        OSTimeDly(2);
    }
}

