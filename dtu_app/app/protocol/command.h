

#ifndef __COMMAND_H__
#define __COMMAND_H__

#include "sys.h"

#define CMD_MAX_LEN  1024


typedef enum
{
    COMMAND_UART = 0,
    COMMAND_SMS = 1,
    COMMAND_GPRS = 2,
}CommandReceiveFromEnum;


GM_ERRCODE command_on_receive_data(CommandReceiveFromEnum from, char* p_cmd, u16 src_len, char* p_rsp, void * pmsg);

void debug_port_command_process(void);

u8 is_nmea_log_enable(void);


#endif


