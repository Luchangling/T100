/*守护进程,看门狗和串口日志*/

#include "daemon_task.h"
#include "task_config.h"
#include "fifo.h"
#include "usart.h"
#include "at_command.h"

void daemon_task(void *pdata)
{
    //u32 get_frame_len = 0;

    u8 state = 0;

    while(1)
    {
        /*200ms调用一次*/
        OSTimeDly(200);
        
        switch(state)
        {
            case 0:
            {
               //at_single_step_exe(AT_GPS_POWER,200,NULL,"=1");

                state = 1;

                break;
            }

            case 1:
            {
                //at_single_step_exe(AT_GPS_NMEA,200,NULL,"=\"%s\"","GSV");

                OSTimeDly(20);

                //at_single_step_exe(AT_GPS_NMEA,200,NULL,"=\"%s\"","RMC");
                
                break;
            }
            
        }
               
    }
}

