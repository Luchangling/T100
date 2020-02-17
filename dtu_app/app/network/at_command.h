
#ifndef _AT_COMMAND_H_
#define _AT_COMMAND_H_

#include "sys.h"

typedef enum
{
    AT_RES_OK,
    AT_RES_ERR,
    AT_RES_CME_ERR,
    AT_RES_SOCKET,
    AT_SIM_RDY,
    AT_RES_UNDEF,
    AT_RES_CONNECTED,
    AT_RES_NORESP
    
}CmdExeResultEnum;


typedef enum
{
    AT_INVALID,
    AT_TEST,
    AT_CLOSE_ECHO,
    AT_SIM_READ,
    AT_IMEI,
    AT_ICCID,
    AT_CIMI,
    AT_CSQ,
    AT_CREG,
    AT_CGREG,
    AT_PDPACT,
    AT_PDPDEACT,
    AT_GPS_POWER,
    AT_GPS_NMEA,
    AT_GET_DNS,
    AT_OPEN_SOCKET,
    AT_SOCKET_CLOSE,
    AT_CFG_APN,
    AT_GET_CLOCK,
    AT_READ_SOCKET,
    AT_SOCKET_SEND,
    AT_QFILE,
    AT_MAX
}at_command_enum;

typedef enum
{
    CMD_NORESP = -2,
    
    CMD_ERROR = -1,
    
    CMD_SUCCESS = 0
}CmdExecuteErrCode;


typedef struct
{

    u8 is_opened;

    at_command_enum cmdid;

    u8 *resp;

    u16 resp_len;
    
}model_control_struct;


extern s32 at_single_step_exe(at_command_enum cmdId, u32 noresp_timeout ,void *arg,const char * format, ... );

extern void at_task(void *pdata);

extern void mod_channel_read_to_buff(u8 *dest,u16 len);

extern s32 at_socket_data_send(s32 socket_id , u8 *data , u16 len);

s32 at_socket_data_recv(s32 socket_id , u8 *recv , u16 read_len);

extern void lock_at_channel(void);

extern void release_at_channel(void);

extern INT8U wait_at_resp(u32 timeout);

extern u8 *get_at_resp_addr(void);

extern s32 at_transver_command(const char * format, ... );

extern u8 *find_first_0d0a(u8 *data);

extern u8 *find_first_num(u8 *pdata);

extern INT8U wait_http_get_result(u32 timeout);

extern ErrorStatus ril_exe_command(u32 timeout,u8 *need_resp,u8 *para,u16 max_len,const char * format, ... );
#endif

