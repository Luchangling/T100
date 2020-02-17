
#ifndef __GPRS_H__
#define __GPRS_H__

#include "errorcode.h"
#include "sys.h"


#define MAX_GPRS_USER_NAME_LEN 32
#define MAX_GPRS_PASSWORD_LEN  32
#define MAX_GPRS_APN_LEN       100
#define MAX_GPRS_MESSAGE_LEN       1500
#define MAX_GPRS_PART_MESSAGE_TIMEOUT    3

#define  SW_DNS_REQUEST_ID	         0x055A5121

typedef struct {
    u8 apnName[MAX_GPRS_APN_LEN];
    u8 apnUserId[MAX_GPRS_USER_NAME_LEN]; 
    u8 apnPasswd[MAX_GPRS_PASSWORD_LEN]; 
    u8 authtype; // pap or chap

} ST_GprsConfig;



/* event */
typedef enum
{
    GM_SOC_READ    = 0x01,  /* Notify for read */
    GM_SOC_WRITE   = 0x02,  /* Notify for write */
    GM_SOC_ACCEPT  = 0x04,  /* Notify for accept */
    GM_SOC_CONNECT = 0x08,  /* Notify for connect */
    GM_SOC_CLOSE   = 0x10   /* Notify for close */
} gm_soc_event_enum;

/* Socket return codes, negative values stand for errors */
typedef enum
{
    GM_SOC_SUCCESS           = 0,     /* success */
    GM_SOC_ERROR             = -1,    /* error */
    GM_SOC_WOULDBLOCK        = -2,    /* not done yet */
    GM_SOC_LIMIT_RESOURCE    = -3,    /* limited resource */
    GM_SOC_INVALID_SOCKET    = -4,    /* invalid socket */
    GM_SOC_INVALID_ACCOUNT   = -5,    /* invalid account id */
    GM_SOC_NAMETOOLONG       = -6,    /* address too long */
    GM_SOC_ALREADY           = -7,    /* operation already in progress */
    GM_SOC_OPNOTSUPP         = -8,    /* operation not support */
    GM_SOC_CONNABORTED       = -9,    /* Software caused connection abort */
    GM_SOC_INVAL             = -10,   /* invalid argument */
    GM_SOC_PIPE              = -11,   /* broken pipe */
    GM_SOC_NOTCONN           = -12,   /* socket is not connected */
    GM_SOC_MSGSIZE           = -13,   /* msg is too long */
    GM_SOC_BEARER_FAIL       = -14,   /* bearer is broken */
    GM_SOC_CONNRESET         = -15,   /* TCP half-write close, i.e., FINED */
    GM_SOC_DHCP_ERROR        = -16,   /* DHCP error */
    GM_SOC_IP_CHANGED        = -17,   /* IP has changed */
    GM_SOC_ADDRINUSE         = -18,   /* address already in use */
    GM_SOC_CANCEL_ACT_BEARER = -19,    /* cancel the activation of bearer */
    
    GM_LOG_LOGIN_FAIL = -115,  // Dec-08-2016  登录无应答       5 
    GM_LOG_CONFIG_PARAM = -116,  // 4  参数配置                4
    GM_LOG_ACK_ERR = -117,  // 3 +120                         3
    GM_LOG_HEART_ERROR = -118, // 2,
    GM_LOG_SIM_DROP = -119, //  1,
} gm_soc_error_enum;


typedef struct
{
   u8      ref_count;
   u8      msg_len;	
   u8      socket_id;    /* socket ID */
   gm_soc_event_enum  event_type;   /* soc_event_enum */
   bool    result;       /* notification result. KAL_TRUE: success, KAL_FALSE: error */
   gm_soc_error_enum  error_cause;  /* used only when EVENT is close/connect */
   u32     detail_cause; /* refer to ps_cause_enum if error_cause is
                                  * SOC_BEARER_FAIL */
} gm_soc_notify_ind_struct;


/**
 * Function:   创建gprs模块
 * Description:创建gprs模块
 * Input:	   无
 * Output:	   无
 * Return:	   GM_SUCCESS——成功；其它错误码——失败
 * Others:	   使用前必须调用,否则调用其它接口返回失败错误码
 */
GM_ERRCODE gprs_create(void);

/**
 * Function:   销毁gprs模块
 * Description:销毁gprs模块
 * Input:	   无
 * Output:	   无
 * Return:	   GM_SUCCESS——成功；其它错误码——失败
 * Others:	   
 */
GM_ERRCODE gprs_destroy(void);

/**
 * Function:   gprs模块定时处理入口
 * Description:gprs模块定时处理入口
 * Input:	   无
 * Output:	   无
 * Return:	   GM_SUCCESS——成功；其它错误码——失败
 * Others:	   1秒钟调用1次
 */
GM_ERRCODE gprs_timer_proc(void);

/**
 * Function:   判断GPRS是否正常
 * Description:
 * Input:	   无
 * Output:	   无
 * Return:	   true——正常；false——不正常
 * Others:	   
 */
bool gprs_is_ok(void);


/**
 * Function:   判断是否需要重启
 * Description:
 * Input:	   无
 * Output:	   无
 * Return:	   true——重启；false——不重启
 * Others:	   
 */
bool gprs_check_need_reboot(u32 check_time);

u32 gprs_get_last_good_time(void);
u32 gprs_get_call_ok_count(void);

void gprs_config_apn(void);


//#define 

#endif


