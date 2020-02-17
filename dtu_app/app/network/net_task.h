/*net task */
#ifndef _NET_TASK_H_
#define _NET_TASK_H_

#include "sys.h"
#include "socket.h"
#include "gprs.h"

#define GM_ICCID_LEN 20

typedef enum
{
    NW_INIT,
    NW_CLOSE_ECHO,
    NW_IMEI_READ,
    NW_SIM_READ,
    NW_ICCID_READ,
    NW_REGISTER,
    NW_GET_IMSI,
    NW_CONFIG_APN,
    NW_PDP_ACTIVE,
    NW_READ_LOCALIP,
    NW_PDP_DECTIVE,
    NW_AT_PROCESS,
    NW_PDP_DEACTIVE,
    NW_MODEL_RESTART
}nw_state_enum;

#define MAX_SOCKETS_PER_BEAR 11

#define MAX_SUPPORT_BEARS 6

typedef struct
{
    u8 type;
    u8 ip[4];
    u8 inuse;
    u8 connected;
    
}socket_struct;

typedef struct
{
    u8 context_id;
    
    u8 actived;

    u8 context_type; // 1:IPV4 2: IPV6 

    StreamType stream;
    
    u8 local_ip[4];

    socket_struct soc[MAX_SOCKETS_PER_BEAR]; //当承载通道建立的socket
}bears_info_struct;

typedef struct
{
    u8 bears;
    u8 socket_id;
    u8 err_type;
}ntq_socket_notify_struct;

typedef enum
{
    NTQ_MODEL_READY,
    
    NTQ_SOCKET_OPEN,

    NTQ_SOCKET_CLOSE,

    NTQ_SOCKET_RECV,

    NTQ_SOCKET_CONNECT,

    NTQ_DNS_PRASE,

    NTQ_COMMING_CALL,

    NTQ_COMMING_SMS,

    
}NetTaskMsgEnum;

typedef struct
{
    nw_state_enum sta;
    u32 start_time;
    u8 opened;
    u8 modem_ready;
    u8 sim_rdy;
    u8 iccid[20];
    u8 imsi[20];
    u8 csq;
    u8 creg;
    u8 cgreg;
    u8 cops[8];
    u8 at_retry;
    bears_info_struct bears[MAX_SUPPORT_BEARS];
    char imei[16];
    
}nw_manager_struct;

typedef struct
{
    u16 msg_id;

    u8  msg_len;

    u8  msg[20];
    
}net_task_q_struct;

typedef struct
{
    u8 is_open;
    
}gps_control_struct;

typedef struct
{
    u8 *buff;
    u16 len;
}socket_to_at_param_struct;

#pragma anon_unions 
typedef union
{
    u8 fmt;
    struct
    {
        u8 mod : 2;
        u8 len : 6;
    };
}TestUnion;

typedef struct
{
	int  mcc;
	int  mnc;

    int  lac;
    int  cellid;
    int  rssi;
}CellInfoStruct;



extern CellInfoStruct cell;



typedef void (*FuncPtr) (void);
typedef void (*PsFuncPtr) (void *);

extern nw_manager_struct g_nw;

extern OS_EVENT *g_atsend_sem;

extern OS_EVENT *g_net_q;

#define MAX_NET_TASK_QUE 200

extern void model_task(void *pdata);

/*分配socket id给应用程序*/
extern s32 SocketCreate(int bears ,u8 accesid, StreamType type);

extern s32 SocketConnect(s32 sock_id, char *ip, u16 port, s32 bears,s32 access_id,StreamType type);

extern s32 SocketSend(s32 sock_id , u8 *data , u16 len);

extern s32 SocketRecv(s32 sock_id , u8 *recv_buff , u16 max_len);

extern s32 GetHostByName(char *addr, s32 bears, u8 *result);

extern s32 DnsParserCallback(PsFuncPtr cb_fun);

extern s32 ApnConfig(ST_GprsConfig *apn, s32 *bears);

extern s32 SocketClose(s32 sock_id);

extern u8 CheckSimValid(void);

extern u8 GetServiceAvailability(void);

extern s32 AccountIdClose(void);

extern void SocketRegisterCallBack(PsFuncPtr func);

extern void GetBearerStatus(s32 *bears_status);

extern GM_ERRCODE mm_get_iccid(u8 *iccid);

extern GM_ERRCODE mm_get_imsi(u8 *imsi);

extern void net_work_close(void);

extern bool is_model_ready(void);

extern bool is_at_chan_ready(void);

extern s32 cclk_cmd_parse(const char *rsp , u16 len);

extern void net_get_iccid(u8 *iccid);

extern u8 net_get_csq(void);

#endif

