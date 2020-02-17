
#include "sys.h"
#include "at_command.h"
#include "config_service.h"
#include "net_task.h"
#include "utility.h"
typedef enum
{
    AGPS_INIT,

    AGPS_CONFIG,

    AGPS_ACTIVED_PDP,

    AGPS_REQ_TIME,

    AGPS_TEST_FILE,

    AGPS_DATA_GET,

    AGPS_DATA_READ,

    AGPS_FILE_WRITE,

    AGPS_STRAT_GPS,

    AGPS_TIMER_PROC,

    AGPS_CLOSE,
    
}AgpsServStatusEnum;


typedef struct
{
	//是否gps模块申请了gps
    bool required_agps;   

	 //是否正在请求校时
    bool timing;   

    bool pdp_actived;

    AgpsServStatusEnum sta;

    // epo/agps 文件信息
    // 消息包总大小
    u32 data_len;  

	// 请求文件时,数据起始位置
    u32 data_start;

    u32 start_time;

    u8 http_get_flg;

    u8 http_read_flg;

    u8 gps_restart_flg;
}SocketTypeAgpsExtend;

static SocketTypeAgpsExtend s_agps_socket_extend;

#define AGPS_DEFAULT_CONTEXT_ID 2

#define AGPS_FILE_NAME "RAM:xtra.bin"
#define AGPS_URL "http://xtrapath1.izatcloud.net/xtra3grc.bin"

static void agps_service_close(void);

GM_ERRCODE agps_service_create(void)
{
    memset(&s_agps_socket_extend,0,sizeof(SocketTypeAgpsExtend));

    return GM_SUCCESS;
}

GM_ERRCODE agps_service_destroy(void)
{
    agps_service_close();
    
    memset(&s_agps_socket_extend,0,sizeof(SocketTypeAgpsExtend));
    
	return GM_SUCCESS;
}

void trans_agps_serv_status(AgpsServStatusEnum state)
{
    s_agps_socket_extend.start_time = clock();

    s_agps_socket_extend.sta = state;
}

static void agps_service_close(void)
{
    if(s_agps_socket_extend.pdp_actived)
    {
        ril_exe_command(6000,"OK",NULL,0,"AT+QIDEACT=%d\r\n",AGPS_DEFAULT_CONTEXT_ID);

        s_agps_socket_extend.pdp_actived = false;
    }
}

static void agps_service_init_proc(void)
{
    if(GetServiceAvailability())
    {
        trans_agps_serv_status(AGPS_CONFIG);
    }
}

static void agps_service_config_proc(void)
{
    ril_exe_command(600,"OK",NULL,0,"AT+QHTTPCFG=\"contextid\",%d\r\n",AGPS_DEFAULT_CONTEXT_ID);

    ril_exe_command(600,"OK",NULL,0,"AT+QGPSCFG=\"gpsnmeatype\",%d\r\n",31);

    //ril_exe_command(600,"OK",NULL,0,"AT+QICSGP=%d,1,\"%s\",\"%s\",\"%s\",1\r\n",AGPS_DEFAULT_CONTEXT_ID,config_service_get_pointer(CFG_APN_NAME),\
       // config_service_get_pointer(CFG_APN_USER),config_service_get_pointer(CFG_APN_PWD));
    
    trans_agps_serv_status(AGPS_ACTIVED_PDP);
}

static void agps_service_pdp_active_proc(void)
{
    //s32 context_id = 0;

    #if 0

    if(ril_exe_command(600,"+QIACT: 2",NULL,0,"AT+QIACT?\r\n") != SUCCESS)
    {
        if(ril_exe_command(6000,"OK",NULL,0,"AT+QIACT=%d\r\n",AGPS_DEFAULT_CONTEXT_ID) == SUCCESS)
        {
            s_agps_socket_extend.pdp_actived = true;
            
            trans_agps_serv_status(AGPS_REQ_TIME);
        }
    }
    else
    {
        s_agps_socket_extend.pdp_actived = true;
        
        trans_agps_serv_status(AGPS_REQ_TIME);
    }
    #else
    if(ril_exe_command(6000,"OK",NULL,0,"AT+QIACT=%d\r\n",AGPS_DEFAULT_CONTEXT_ID) == SUCCESS)
    {
        s_agps_socket_extend.pdp_actived = true;
        
        trans_agps_serv_status(AGPS_REQ_TIME);
    }
    #endif
}

static void agps_service_req_time_proc(void)
{
    ril_exe_command(600,"OK",NULL,0,"AT+QNTP=%d,\"ntp1.aliyun.com\",123,0\r\n",AGPS_DEFAULT_CONTEXT_ID);

    trans_agps_serv_status(AGPS_TEST_FILE);
}

static void agps_service_test_file_proc(void)
{
    if(util_get_utc_time() > 1572451503ul)
    {
        trans_agps_serv_status(AGPS_DATA_GET);

        
        if(ril_exe_command(600,"CONNECT",NULL,0,"AT+QHTTPURL=%d,%d\r\n",strlen(AGPS_URL),80) == SUCCESS)
            {
                if(ril_exe_command(600,"OK",NULL,0,"%s",AGPS_URL) == SUCCESS)
                {
                    if(ril_exe_command(600,"OK",NULL,0,"AT+QHTTPGET=%d\r\n",120) == SUCCESS)
                    {
                        trans_agps_serv_status(AGPS_DATA_GET);
                        //if(wait_http_get_result(120*200) == OS_ERR_NONE)
                        #if 0
                        {
                            if(ril_exe_command(120*1000,"+QHTTPREADFILE",NULL,0,"AT+QHTTPREADFILE=\"%s\",%d\r\n",AGPS_FILE_NAME,120) == SUCCESS)
                            {
                                cl_log(INFO,"clock(%d) Agps service status trans to AGPS_FILE_WRITE",clock());
                                
                                trans_agps_serv_status(AGPS_FILE_WRITE);
        
                                return;
                            }
                        }
                        #endif
                    }
                }
            }
    }
   
}
s32 qi_http_get_urc_recv_cb(u8 *urc_str, u16 len) 
{
    s32 err_code,result,rlen = 0;

    if(sscanf((const char*)urc_str,"%*[^ ]%d,%d,%d",&err_code,&result,&rlen) == 3)
    {
        cl_log(INFO,"http get : %s",urc_str);
        if(err_code == 0)
        {
            s_agps_socket_extend.http_get_flg = 1;
            //release_http_get_single();
            //trans_agps_serv_status(AGPS_DATA_READ);
        }
    }

    return 0;
}
//+QHTTPREADFILE
s32 qi_http_read_urc_recv_cb(u8 *urc_str, u16 len)
{
    s32 err_code;

    if(sscanf((const char*)urc_str,"%*[^ ]%d",&err_code) == 1)
    {
        cl_log(INFO,"http read : %s",urc_str);
        if(err_code == 0)
        {
            s_agps_socket_extend.http_read_flg = 1;
            //release_http_get_single();
            //trans_agps_serv_status(AGPS_DATA_READ);
        }
    }

    return 0;
}


static void agps_service_data_get_proc(void)
{

    if(s_agps_socket_extend.http_get_flg)
    {
        s_agps_socket_extend.http_get_flg = 0;

        if(ril_exe_command(600,"OK",NULL,0,"AT+QHTTPREADFILE=\"%s\",%d\r\n",AGPS_FILE_NAME,80) == SUCCESS)
        {
            cl_log(INFO,"clock(%d) Agps service status trans to AGPS_FILE_WRITE",clock());
            
            trans_agps_serv_status(AGPS_DATA_READ);

            return;
        }
    }
    else
    {
        if(clock() - s_agps_socket_extend.start_time >= 120)
        {
            trans_agps_serv_status(AGPS_TEST_FILE);
        }
    }
}

static void agps_service_file_read_proc(void)
{
    if(s_agps_socket_extend.http_read_flg)
    {
        s_agps_socket_extend.http_read_flg = 0;

        trans_agps_serv_status(AGPS_FILE_WRITE);
    }
    else
    {
        if(clock() - s_agps_socket_extend.start_time >= 120)
        {
            trans_agps_serv_status(AGPS_TEST_FILE);
        }
    }
    
}

static void agps_service_file_write_proc(void)
{
   // AT+QGPSXTRADATA="RAM:xtra.bin"
    u8 temp[100] = {0};

    time_t sec = 0;//get_local_utc_sec();

    struct tm *tm_t;

    if(gps_is_fixed())
    {
        trans_agps_serv_status(AGPS_TIMER_PROC);
        return;
    }

    if(ril_exe_command(600,"+CCLK:",temp,sizeof(temp),"AT+CCLK?\r\n") == SUCCESS)
    {
        cclk_cmd_parse((const char *)temp, strlen((const char *)temp));
    }

    sec = get_local_utc_sec();

    

    if(sec < 4128684207ul && sec > 1572540207ul)
    {
        tm_t = localtime(&sec);

        gps_power_off();

        //gps_power_on();

        ril_exe_command(600,"OK",NULL,0,"AT+QGPSXTRA=1\r\n");
        
        //AT+QGPSXTRATIME=0,"2019/10/29,16:34:30",1,1,3500
        ril_exe_command(600,"OK",NULL,0,"AT+QGPSXTRATIME=0,\"%d/%02d/%02d,%02d:%02d:%02d\",1,1,3500\r\n",\
        tm_t->tm_year+1900,tm_t->tm_mon+1,tm_t->tm_mday,tm_t->tm_hour,tm_t->tm_min,tm_t->tm_sec);
       
        ril_exe_command(600,"OK",NULL,0,"AT+QGPSXTRADATA=\"%s\"\r\n",AGPS_FILE_NAME);

        //ril_exe_command(600,"OK",NULL,0,"AT+QGPSXTRA=1\r\n");

        trans_agps_serv_status(AGPS_STRAT_GPS);
    }
    
}

static void agps_service_start_gps_proc(void)
{
    //agps_service_close();
    //ril_exe_command(600,"OK",NULL,0,"AT+QGPS=1\r\n");
    gps_power_on();

    trans_agps_serv_status(AGPS_TIMER_PROC);
}

GM_ERRCODE agps_service_timer_proc(void)
{
    switch(s_agps_socket_extend.sta)
    {
        case AGPS_INIT:
            agps_service_init_proc();
            break;
        case AGPS_CONFIG:
            agps_service_config_proc();
            break;
        case AGPS_ACTIVED_PDP:
            agps_service_pdp_active_proc();
            break;
        case AGPS_REQ_TIME:
            agps_service_req_time_proc();
            break;
        case AGPS_TEST_FILE:
            agps_service_test_file_proc();
            break;
        case AGPS_DATA_GET:
            agps_service_data_get_proc();
            break;
        case AGPS_DATA_READ:
            agps_service_file_read_proc();
            break;
        case AGPS_FILE_WRITE:
            agps_service_file_write_proc();
            break;
        case AGPS_STRAT_GPS:
            agps_service_start_gps_proc();
            break;
        case AGPS_TIMER_PROC:
            //
            if(s_agps_socket_extend.gps_restart_flg)
            {
                s_agps_socket_extend.gps_restart_flg = 0;
                trans_agps_serv_status(AGPS_FILE_WRITE);
            }
            break;
        //case AGPS_CLOSE:
        default:  
            break;
    }

    return GM_SUCCESS;
}

void agps_service_rewite_data(void)
{
    s_agps_socket_extend.gps_restart_flg = 1;
}


