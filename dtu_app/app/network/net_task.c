
/*Net work task*/
#include "config_service.h"
#include "net_task.h"
#include "task_config.h"
#include "at_command.h"
#include "usart.h"
#include "bsp.h"
#include "delay.h"
#include "command.h"
#include "gps.h"
#include "bms_service.h"
#include "sensor_service.h"
#include "led_service.h"
OS_EVENT *g_net_q = NULL;

void *g_net_msg[MAX_NET_TASK_QUE];

nw_manager_struct g_nw;

char *s_stream_param[] = {"TCP","UDP"};

enum
{
    PS_CB_DNS_PRASE,
        
    PS_CB_SOCKET,

    MAX_PS_CB_FUN,
};

PsFuncPtr s_pscallback[MAX_PS_CB_FUN] ={NULL,NULL};

GM_ERRCODE mm_get_iccid(u8 *iccid)
{
    if(strlen((const char*)g_nw.iccid) > 0)
    {
        memcpy(iccid,g_nw.iccid,strlen((const char*)g_nw.iccid));

        return GM_SUCCESS;
    }

    return GM_UNKNOWN;
}

GM_ERRCODE mm_get_imsi(u8 *imsi)
{
    if(strlen((const char *)g_nw.imsi) > 0)
    {
        memcpy(imsi,g_nw.imsi,strlen((const char *)g_nw.imsi));

        return GM_SUCCESS;
    }

    return GM_UNKNOWN;
}

#define DEFAULT_BEARS 1

/*分配socket id给应用程序*/
s32 SocketCreate(int bears ,u8 accesid, StreamType type)
{
    //u8 i = 0;

    bears_info_struct *info = &g_nw.bears[bears];
    
    /*激活的状态使用*/
    if(info->actived)
    {
        if(info->soc[accesid].inuse != true)
        {
            info->soc[accesid].inuse = true;

            info->soc[accesid].type = type;

            return accesid;
        }
    }
    
    return -1;
}

s32 SocketConnect(s32 sock_id, char *ip, u16 port, s32 bears,s32 access_id,StreamType type)
{
    bears_info_struct *info = &g_nw.bears[bears];

    if(info->actived &&\
       sock_id >= 0 && sock_id < MAX_SOCKETS_PER_BEAR && \
       info->soc[sock_id].inuse && !info->soc[sock_id].connected)
    {
        //当前的承载是激活的
        cl_log(INFO,"clock(%d) Satrt Open socket!",clock());
        //if(at_single_step_exe(AT_OPEN_SOCKET,200,NULL,"=%d,%d,\"%s\",\"%s\",%d,0,0",bears,sock_id,s_stream_param[type],ip,port) == AT_RES_OK)
        if(ril_exe_command(6000,"+QIOPEN",NULL,0,"AT+QIOPEN=%d,%d,\"%s\",\"%s\",%d,0,0\r\n",bears,sock_id,s_stream_param[type],ip,port) == SUCCESS)
        {
             cl_log(INFO,"clock(%d)Socket Open success!",clock());
            info->soc[sock_id].connected = 1;
            return 1;
        }

        cl_log(INFO,"clock(%d)Socket Open fail!",clock());
        
    }
    
    return 0;
}

s32 SocketSend(s32 sock_id , u8 *data , u16 len)
{
    s32 res = 0;


    cl_log(INFO,"clock(%d) Start Socket send",clock());
    
    res = at_socket_data_send(sock_id,data,len);

    cl_log(INFO,"clock(%d) Start Socket compliete(res %d)",clock(),res);
    //if(ril_exe_command(600,"OK",NULL,0,"AT+QISEND=%d,%d\r\n",sock_id,len) == SUCCESS)
    //{
        
    //}

    if(res == AT_RES_OK)
    return len;
    else
    return 0;
}


s32 SocketRecv(s32 sock_id , u8 *recv_buff , u16 max_len)
{
    s32 res;

    res = at_socket_data_recv(sock_id , recv_buff , max_len);

    return res;
}

s32 SocketClose(s32 sock_id)
{
    s32 res;

    //res = at_single_step_exe(AT_SOCKET_CLOSE, 0, NULL,"=%d",sock_id);
    res = ril_exe_command(150000,"OK",NULL,0,"AT+QICLOSE=%d\r\n",sock_id);

    if(res == SUCCESS)
    {
        
        //memset(&g_nw.bears[DEFAULT_BEARS].soc,0,sizeof(socket_struct));
    }

    if(sock_id < MAX_SOCKETS_PER_BEAR)
    {
        memset(&g_nw.bears[DEFAULT_BEARS].soc[sock_id].type,0,sizeof(socket_struct));
    }
    
    return 0;
}


s32 GetHostByName(char *addr, s32 bears, u8 *result)
{
    s32 res;
    
    //res = at_single_step_exe(AT_GET_DNS,0,NULL,"=1,\"%s\"",addr);
    res = ril_exe_command(600,"OK",NULL,0,"AT+QIDNSGIP=1,\"%s\"\r\n",addr);

    *result = res;

    return 0;
}

s32 DnsParserCallback(PsFuncPtr cb_fun)
{
    s_pscallback[PS_CB_DNS_PRASE] = cb_fun;
   
    return 0;
}

s32 ApnConfig(ST_GprsConfig *apn, s32 *bears)
{
    s32 res;
    //AT+QICSGP

    //res = at_single_step_exe(AT_CFG_APN,0,NULL,"=1,\"%s\",\"%s\",\"\",%d",apn->apnName,apn->apnUserId,apn->apnPasswd,apn->authtype);
    res = ril_exe_command(600,"OK",NULL,0,"AT+QICSGP=1,1,\"%s\",\"%s\",\"%s\",%d\r\n",apn->apnName,apn->apnUserId,apn->apnPasswd,apn->authtype);

    if(res == AT_RES_OK)
    {
        *bears = 1;
    }
    
    return res;
}

u8 CheckSimValid(void)
{
    return g_nw.sim_rdy;
}

u8 GetServiceAvailability(void)
{
    static u32 pre_time = 0;

    s32 s_value = 0;

    u8 temp[50] = {0};

    if((clock() - pre_time >= 1)&&\
      ((g_nw.cgreg != 1 && g_nw.cgreg != 5)))
    {
        pre_time = clock();

        //cl_log(DEBUG,"clock(%d) send creg..",clock());

        if(ril_exe_command(1000,"+CGREG:", temp, sizeof(temp),"AT+CGREG?\r\n") == SUCCESS)
        {
            //cl_log(DEBUG,"clock(%d) rev creg %s",clock(),temp);
            
            sscanf((const char*)temp,"%*[^,],%d",&s_value);

            g_nw.cgreg = s_value;
        }
        else
        {
            //cl_log(DEBUG,"clock(%d) creg rev error %s",clock(),temp);
        }
        
        //at_single_step_exe(AT_CGREG,200,NULL,"?");
    }

    if(g_nw.cgreg == 1 || g_nw.cgreg == 5)
    return 1;
    else
    return 0;
   
}


//关闭所有激活的BEARS
s32 AccountIdClose(void)
{
    s32 res = 0;
    //AT+QIDEACT
   //at_single_step_exe(AT_PDPDEACT,200,NULL,"=%d",DEFAULT_BEARS);
   res = ril_exe_command(3000,"OK",NULL,0,"AT+QIDEACT=1\r\n");
   
   return res; 
}

//注册socket通知函数
void SocketRegisterCallBack(PsFuncPtr func)
{
    s_pscallback[PS_CB_SOCKET] = func;
}

//default bears status!
void GetBearerStatus(s32 *bears_status)
{
    u32 pre_time = 0;
    #if 1
    if(clock() - pre_time >= 1)
    {
        pre_time = clock();
        
        if(ril_exe_command(200*150,"OK",NULL,0,"AT+QIACT=%d\r\n", DEFAULT_BEARS) == SUCCESS)
        {
            g_nw.bears[DEFAULT_BEARS].actived = 1;
        }
    }
    #endif
    
    if(g_nw.bears[DEFAULT_BEARS].actived == 1)
    {
        *bears_status = 4;

        return;
    }

    *bears_status = 0;
    //4    :激活IPV4
}

void net_trans_status(nw_state_enum new_state)
{
    g_nw.sta = new_state;

    g_nw.start_time = clock();

    g_nw.at_retry = 0;
}

void model_power_on(void)
{
    MODEL_POWER_ON();
    MODEL_POWER_KEY_L();
    delay_ms(50); //delay 50 ms
    MODEL_POWER_KEY_H();
    delay_ms(600); //delay 600ms 
    MODEL_POWER_KEY_L();

    //OSTimeDly(120); //delay 600ms
}

void model_power_off(void)
{
}

void net_service_destory(void)
{
    model_power_off();

    memset((u8 *)&g_nw,0,sizeof(nw_manager_struct));
}

void net_work_close(void)
{
    net_trans_status(NW_PDP_DEACTIVE);
}

void net_service_open(void)
{   
    memset((u8 *)&g_nw,0,sizeof(nw_manager_struct));

    g_nw.start_time = clock();

    model_power_on();

    delay_ms(500); //500MS

    cl_log(INFO,"system boot up!!!");
    //cl_log(INFO,"wait model ready..%d",clock());    
}

void net_service_restart(void)
{
    memset((u8 *)&g_nw,0,sizeof(nw_manager_struct));

    led_service_net_set(LED_OFF);
    
    //net_service_destory();    
    MODEL_RESET_H();
    //断电2s
    OSTimeDly(1000/2);

    MODEL_RESET_L();

    OSTimeDly(100);
}

bool is_model_ready(void)
{
    return g_nw.modem_ready>0?true:false;
}

bool is_at_chan_ready(void)
{
    return false;
}

u8 net_get_csq(void)
{
    return g_nw.csq;
}

void net_get_iccid(u8 *iccid)
{
    memcpy(iccid,g_nw.iccid,strlen(g_nw.iccid));
}

s32 cclk_cmd_parse(const char *rsp , u16 len)
{
    u8 *p = NULL;

    struct tm t_tm;

    int t_gmt = 0;

    u32 sec = 0;

    p = find_first_num((u8 *)rsp);

    if(p != NULL)
    {
        if(sscanf((const char*)p,"%d/%d/%d,%d:%d:%d+%d",&t_tm.tm_year,&t_tm.tm_mon,&t_tm.tm_mday,&t_tm.tm_hour,\
            &t_tm.tm_min,&t_tm.tm_sec,&t_gmt) == 7)
        {
            t_tm.tm_year = (t_tm.tm_year + 2000 - 1900);

            t_tm.tm_mon  = t_tm.tm_mon - 1;

            cl_log(INFO,"clock(%d) get net work time %d/%d/%d,%d:%d:%d+%d",clock(),t_tm.tm_year + 1900,t_tm.tm_mon+1,t_tm.tm_mday,t_tm.tm_hour,\
            t_tm.tm_min,t_tm.tm_sec,t_gmt);

            sec = mktime(&t_tm);

            if(t_gmt > 0)
            {
                sec = sec - (t_gmt * 15 * 60);
            }

            cl_log(INFO,"clock(%d) calac utc sec %x",clock(),sec);

            if(sec > 0)
            {
                set_local_time((u32)sec);
            }
        }
    }


    return 0;
}

CellInfoStruct cell;

void net_get_servi_cell_info(u8 *lbs)
{
    if(strstr(lbs,"4G") != NULL)
    {
        if(sscanf(lbs,"%*[^:]:%*[^,],%*[^,],%*[^,],%*[^,],%d,%d,%d,%*[^,],%*[^,],%*[^,],%*[^,],%*[^,],%d,%*[^,],%*[^,],%d",\
            &cell.mcc,&cell.mnc,&cell.cellid,&cell.lac,&cell.rssi) > 0)
        {
            
        }
    }    
}


/*10ms perioc*/
void net_service_timer_proc(void)
{   
    static u16 period = 0;

    s32 value = 0;

    u8 temp[50] = {0};

    u8 cellinfo[1024] = {0};

    u8 *p = NULL;

    if(period > 0)
    {
        period--;
        return;
    }
    else
    {
        period = 10; /*default 100ms*/
    }
    
    if(!g_nw.opened)return;
    
    switch(g_nw.sta)
    {
        case NW_INIT:
        {  
            if(ril_exe_command(200,"OK",NULL,0,"AT\r\n") == SUCCESS)
            {
                net_trans_status(NW_CLOSE_ECHO);

                g_nw.modem_ready = 1;

                led_service_net_set(LED_SLOW_FLASH);
                
                break;
            }
            else
            {
                if(clock() - g_nw.start_time >= 3)
                {
                    //10s未得到OK,要重启MODEM
                    cl_log(ERR,"net serv AT no resp! (%d)",clock());
                    //net_trans_status(NW_MODEL_RESTART);
                }
            }

            break;
        }
        case NW_CLOSE_ECHO:
        {
            if(ril_exe_command(200,"OK",NULL,0,"ATE0\r\n") == SUCCESS)
            {
                net_trans_status(NW_IMEI_READ);

                ril_exe_command(200,"OK",NULL,0,"AT+QURCCFG=\"urcport\",\"uart1\"\r\n");

                
            }
            else
            {
                if(clock() - g_nw.start_time >= 3)
                {
                    //10s未得到OK,要重启MODEM
                    net_trans_status(NW_MODEL_RESTART);
                }
            }

            break;
        }
        case NW_IMEI_READ:
        {
            memset(temp,0,sizeof(temp));
            if(ril_exe_command(200,"OK",temp,sizeof(temp),"AT+CGSN\r\n") == SUCCESS)
            {
                gps_create();
                
                p = find_first_num(temp);

                if(p != NULL)
                {
                    sscanf((const char*)p,"%s",g_nw.imei);
                    //memcpy(g_nw.imei,temp,15);

                    cl_log(INFO,"clock(%d) Imei %s",clock(),g_nw.imei);
                }
                
                net_trans_status(NW_SIM_READ);
            }
            else
            {
                if(clock() - g_nw.start_time >= 3)
                {
                    //10s未得到OK,要重启MODEM
                    net_trans_status(NW_MODEL_RESTART);
                }
            }
            
            break;
        }
        
        case NW_SIM_READ:
        {
            if(ril_exe_command(200,"READY",NULL,0,"AT+CPIN?\r\n") == SUCCESS)
            {
                 net_trans_status(NW_ICCID_READ);
            }
            else
            {
                period = 50;
            }

            if(clock() - g_nw.start_time >= 30)
            {
                //30s未得到OK,要重启MODEM
                net_trans_status(NW_MODEL_RESTART);
            }
            
            break;
        }

        case NW_ICCID_READ:
        {
            memset(temp,0,sizeof(temp));
            if(ril_exe_command(200,"OK",temp,sizeof(temp),"AT+QCCID\r\n") == SUCCESS)
            {
                sscanf((const char*)temp,"%*[^ ]%s",g_nw.iccid);

                cl_log(INFO,"clock(%d) Icid %s",clock(),g_nw.iccid);
                
                net_trans_status(NW_GET_IMSI);
                
            }
            else
            {
                if(clock() - g_nw.start_time >= 3)
                {
                    //10s未得到OK,要重启MODEM
                    net_trans_status(NW_MODEL_RESTART);
                }
            }
            break;
        }
        case NW_GET_IMSI:
        {
            memset(temp,0,sizeof(temp));
            if(ril_exe_command(200,"OK",temp,sizeof(temp),"AT+CIMI\r\n") == SUCCESS)
            {
                sscanf((const char*)temp,"%[0-9a-Z]",g_nw.imsi);
                if(strlen((char *)g_nw.imsi) > 0)
                {
                    cl_log(INFO,"clock(%d) model info imsi %s",clock(),g_nw.imsi);
                }

                gprs_config_apn();

                net_trans_status(NW_AT_PROCESS);
            }

            if(clock() - g_nw.start_time >= 3)
            {
                //10s未得到OK,要重启MODEM
                net_trans_status(NW_MODEL_RESTART);
            }
            break;
        }
        case NW_AT_PROCESS:
        {
            //at_single_step_exe(AT_CSQ,200,NULL,"");
            memset(temp,0,sizeof(temp));
            if(ril_exe_command(600,"OK",temp,sizeof(temp),"AT+CSQ\r\n") == SUCCESS)
            {
                sscanf((const char*)temp,"%*[^ ]%d",&value);
            }

            g_nw.csq = (u8)value;

            cl_log(INFO,"clock(%d) net serv csq %d",clock(),g_nw.csq);

            memset(temp,0,sizeof(temp));
            
            if(ril_exe_command(600,"OK",temp,sizeof(temp),"AT+CCLK?\r\n") == SUCCESS)
            {
                cclk_cmd_parse((const char *)temp,strlen((const char *)temp));
            }

            cl_log(INFO,"clock(%d) net serv sysclock %x ",clock(),get_local_utc_sec());

            if(ril_exe_command(600,"OK",cellinfo,sizeof(cellinfo),"AT+QENG=\"servingcell\"\r\n") == SUCCESS)
            {
                cl_log(INFO,"%s",cellinfo);
            }

            memset(cellinfo,0,sizeof(cellinfo));

            if(ril_exe_command(600,"OK",cellinfo,sizeof(cellinfo),"AT+QENG=\"neighbourcell\"\r\n") == SUCCESS)
            {
                cl_log(INFO,"%s",cellinfo);
            }

      

            period = 10000;
            
            break;
        }
        case NW_PDP_DEACTIVE:
        {
            //at_single_step_exe(AT_PDPDEACT,200,NULL,"=%d",DEFAULT_BEARS);
            if(ril_exe_command(10000,"OK",NULL,0,"AT+QIDEACT=%d\r\n",DEFAULT_BEARS) == SUCCESS)
            {
                cl_log(INFO,"net serv pdp deactive(%d)",clock());
                net_trans_status(NW_INIT); 
            }
            else
            {
                net_trans_status(NW_MODEL_RESTART);
            }
            
            break;
        }

        case NW_MODEL_RESTART:
        {
            net_service_restart();

            net_trans_status(NW_INIT);
        }

        default:
            break;
    }
}

typedef struct
{
    char cellnum[40];

    char at_resp[400];

    char recv[160];

    char resp[160];

}SmsInfoStruct;

void net_task_q_process(void)
{
    INT8U err;

    net_task_q_struct *m_net_q = NULL;

    SmsInfoStruct *sms = NULL;

    //gm_soc_notify_ind_struct soc;

    m_net_q = OSQAccept(g_net_q, &err);
    
    if(err == OS_ERR_NONE && m_net_q != NULL)
    {
        switch(m_net_q->msg_id)
        {
            case NTQ_MODEL_READY:

                g_nw.opened = 1;

                break;
            case NTQ_SOCKET_OPEN:
                //OPEN net task confrim
                break;
            case NTQ_SOCKET_CLOSE:

            case NTQ_SOCKET_CONNECT:
            case NTQ_SOCKET_RECV:

                if(s_pscallback[PS_CB_SOCKET])
                {
                    s_pscallback[PS_CB_SOCKET](m_net_q->msg);
                }
                
                break;

            case NTQ_DNS_PRASE:

                cl_log(INFO,"clock(%d) net serv get dns ip(%d.%d.%d.%d)",clock(),m_net_q->msg[0],m_net_q->msg[1],m_net_q->msg[2],m_net_q->msg[3]);
                if(s_pscallback[PS_CB_DNS_PRASE])
                s_pscallback[PS_CB_DNS_PRASE](m_net_q->msg);

                break;
           case NTQ_COMMING_CALL:
                cl_log(INFO,"clock(%d) coming call...",clock());
                ril_exe_command(300,"OK",NULL, 0, "ATA\r\n");
                break;
           case NTQ_COMMING_SMS:
                cl_log(INFO,"clock(%d) comming sms...",clock());
                ril_exe_command(300,"OK",NULL, 0, "AT+CMGF=1\r\n");
                sms = (SmsInfoStruct *)malloc(sizeof(SmsInfoStruct));
                if(sms == NULL)
                {
                    LOG(FATAL,"clock(%d) SmsInfo malloc err!",clock());
                }
                else
                {
                    ril_exe_command(300,"OK",sms->at_resp,400,"AT+CMGR=%d\r\n",m_net_q->msg[0]);

                    if(sscanf(sms->at_resp,"%*[^,],%[^,],%*[^\n]%*c%[^\n]",sms->cellnum,sms->recv))
                    {
                        LOG(INFO,"clcok(%d) recv sms info cell(%s),data(%s)",clock(),sms->cellnum,sms->recv);

                        if(command_on_receive_data(COMMAND_SMS,sms->recv,strlen(sms->recv),sms->resp,NULL) == GM_SUCCESS)
                        {
                            ril_exe_command(300,"> ",NULL,0,"AT+CMGS=%s\r\n",sms->cellnum);

                            ril_exe_command(3000,"+CMGS",NULL,0,"%s\x1A",sms->resp);
                        }
                    }

                    ril_exe_command(3000,"OK",NULL,0,"AT+CMGD=%d\r\n",m_net_q->msg[0]);

                    free(sms);
                }
                break;
        }

        free(m_net_q);
    }
}

typedef enum
{
   MAN_WORK_INIT,
   MAN_WORK_FS_READ,
   MAN_WORK_TIMER_PROC,
   MAN_WORK_CLOSE
}MainWorkStatusEnum;

typedef struct
{
    MainWorkStatusEnum sta;

    u32 start_time;
    
}Man_Work_Control_Struct;

Man_Work_Control_Struct s_man;

void trans_man_work_status(MainWorkStatusEnum state)
{
    s_man.sta = state;

    s_man.start_time = clock();
}

void man_work_init_proc(void)
{
    if(is_model_ready())
    {
        trans_man_work_status(MAN_WORK_FS_READ);
    }
    else
    {
       
    }
}

void man_work_fs_read_proc(void)
{
    
    trans_man_work_status(MAN_WORK_TIMER_PROC);
}

void main_task()
{
    switch(s_man.sta)
    {
        case MAN_WORK_INIT:
            man_work_init_proc();
        break;

        case MAN_WORK_FS_READ:
            
            trans_man_work_status(MAN_WORK_TIMER_PROC);
            //man_work_fs_read_proc();
            break;
        case MAN_WORK_TIMER_PROC:
            
            break;
    }
}


void model_task(void *pdata)
{
    pdata = pdata;

    if(g_net_q == NULL)
    g_net_q = OSQCreate((void **)&g_net_msg,MAX_NET_TASK_QUE);

    net_service_open();

    //trans_man_work_status(MAN_WORK_INIT);
    
    while(1)
    {
        main_task();
        
        net_task_q_process();

        net_service_timer_proc();

        //sensor_service_proc();

        if(is_model_ready())
        {
            gprs_timer_proc();

            gps_timer_proc();

            debug_port_command_process();

            //gps_get_process();
            bms_service_timer_proc();
        }
        else
        {
            if(clock() - s_man.start_time >= 1)
            {
                s_man.start_time = clock();
                cl_log(INFO,"Wait Model Ready..%d",clock());
            } 
        }
        
        OSTimeDly(1); //5MS
    }
}

void net_task_del(void)
{
    #if 0
    INT8U err;
    
    g_atsend_sem = NULL;

    OSSemDel(g_atsend_sem,OS_DEL_NO_PEND,&err);

    if(err == OS_ERR_TASK_WAITING)
    {
        OSSemPost(g_atsend_sem);

        OSTimeDly(200);
    }

    OSSemDel(g_atsend_sem,OS_DEL_ALWAYS,&err);
    #endif
}


