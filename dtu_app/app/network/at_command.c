/*AT commad*/

#include "at_command.h"
#include "net_task.h"
#include "usart.h"
#include "daemon_task.h"
#include <stdarg.h>
#include "bsp.h"
#include "timer.h"
#include "agps_service.h"
#define MAX_AT_CMD_LEN 200


typedef enum
{
    CMD_ATTRIBUTE_READ,
    CMD_ATTRIBYTE_SET
}CmdAttributeEnum;

model_control_struct s_model;

typedef  s32 (*ATCommandExeCb)(u8 result ,const char *rsp ,u16 len,void *arg);

typedef  s32 (*ATCommandSendCb)(at_command_enum cmdId,u8 *arg);

typedef  s32 (*URCCommandPraseCb)(u8 *rsp , u16 len);

typedef struct
{
    at_command_enum cmdId;
    
    char *cmd;

    ATCommandExeCb callback;

    u16 rsp_timeout;   //uint :10ms
    
}AT_command_entery_struct;

typedef struct
{
    char *cmd;

    URCCommandPraseCb urc_cb;
    
}URC_Command_Entry_Struct;

char *at_exe_result_string[AT_RES_UNDEF] ={"OK","ERROR","+CME ERROR","> ","READY"};

#define EC20URC_STRING "+QIURC:"
#define SOCKET_READ_MARK "+QIRD:"


#define MAX_LINES 10

typedef struct
{
    u8 count;
    u8* addr[MAX_LINES];
    u16 len[MAX_LINES];
}at_resp_line_struct;

#define MAX_AT_RESP_STRING 1500

//u8 s_at_resp[UART1_DMA_SIZE] = {0};

typedef struct
{
    u8 is_dev_opened;
    
    at_command_enum cmdId;

    char cmd_buf[MAX_AT_CMD_LEN];

    u8 send_attribute;

    u16 recived;

    char resp[MAX_AT_RESP_STRING];

    s32 cur_at_result;

    void *arg;

    
}AT_Entry_Struct;

AT_Entry_Struct s_at;




OS_EVENT *g_atsend_sem = NULL;

OS_EVENT *g_atresp_sem = NULL;

OS_EVENT *g_http_get_sem = NULL;

//s32 at_test_cmd_parse(u8 result ,const char *rsp , u16 len);
s32 at_cmd_send(at_command_enum cmdId, const char * format, ... );

s32 qi_urc_dns_prase_cb(u8 *urc_str, u16 len);

s32 qi_urc_socket_recv_cb(u8 *urc_str, u16 len);

s32 qi_urc_socket_close_cb(u8 *urc_str, u16 len);


s32 qi_urc_pdp_deact_cb(u8 *urc_str, u16 len)
{
    net_work_close();

    gprs_destroy();
    
    return 0;
}

s32 qi_urc_cgreg_cb(u8 *urc_str, u16 len)
{
    s32 creg = 0;

    if(sscanf((const char*)urc_str,"%*[^:]%d",&creg) == 1)
    {
        g_nw.creg = g_nw.cgreg = (u8)creg;
    }
    else
    {
        g_nw.creg = g_nw.cgreg = (u8)0;
    }
    
    return 0;
}

s32 ready_urc_recv_cb(u8 *urc_str, u16 len)
{
    //g_nw.opened = 1;
    net_task_q_struct *q = NULL;

    q = (net_task_q_struct *)malloc(sizeof(net_task_q_struct));

    if(q != NULL)
    {
        cl_log(INFO,"clock(%d) model READY",clock());
        
        q->msg_id = NTQ_MODEL_READY;

        if(g_net_q != NULL)
        OSQPost(g_net_q,(void *)q);
    }
    

    return 0;
}

s32 qi_open_urc_recv_cb(u8 *urc_str, u16 len)
{
    //+QI+OPEN:0,0

    net_task_q_struct *q = NULL;

    gm_soc_notify_ind_struct *soc = NULL;

    s32 id = 0, error = 0;

    if(sscanf((const char *)urc_str,"%*[^ ]%d,%d",&id,&error) == 2)
    {
        q = (net_task_q_struct *)malloc(sizeof(net_task_q_struct));

        if(q != NULL)
        {
            q->msg_id = NTQ_SOCKET_CONNECT;
            
            soc = (gm_soc_notify_ind_struct *)q->msg;

            soc->socket_id = id;

            if(error != 0)
            {
                error = -1;
            }
            soc->error_cause = (gm_soc_error_enum)error;

            soc->result = true;

            soc->event_type = GM_SOC_CONNECT;

            if(g_net_q != NULL)
            OSQPost(g_net_q,(void *)q);
        }
    }
    
    return 0;
}

s32 qi_cmti_comming_cb(u8 *urc_str, u16 len)
{
    net_task_q_struct *q = NULL;
    s32 index = 0;
    if(sscanf(urc_str,"%*[^,],%d",&index))
    {
        q = malloc(sizeof(net_task_q_struct));
        q->msg_id = NTQ_COMMING_SMS;
        q->msg_len = 1;
        q->msg[0] = index;
        cl_log(DEBUG,"clock(%d),cmti index = %d",clock(),index);
        OSQPost(g_net_q,(void *)q);
    }
    
    return 0;
}


s32 qi_comming_ring_cb(u8 *urc_str, u16 len)
{
    net_task_q_struct *q = NULL;
    q = malloc(sizeof(net_task_q_struct));
    q->msg_id = NTQ_COMMING_CALL;
    OSQPost(g_net_q,(void *)q);
		return 0;
}


INT8U wait_http_get_result(u32 timeout)
{
    INT8U err;

    OSSemPend(g_http_get_sem,timeout,&err);

    return err;
}

void release_http_get_single(void)
{
    OSSemPost(g_http_get_sem);
}
#if 0
s32 qi_http_get_urc_recv_cb(u8 *urc_str, u16 len)
{
    s32 err_code,result,rlen;

    if(sscanf((const char*)urc_str,"%*[^ ]%d,%d,%d",&err_code,&result,&rlen) == 3)
    {
        if(result == 200)
        {
            release_http_get_single();
        }
    }

    return 0;
}
#endif
//+QHTTPREADFILE
//s32 qi_http_read_urc_recv_cb(u8 *urc_str, u16 len)

u8 *find_first_num(u8 *pdata)
{
    u8 *p = pdata;

    while(*p != '\0')
    {
        if(*p >= '0' && *p <= '9')
        {
            return p;
        }

        p++;
    }

    return p;
}

s32 qi_urc_ntp_recv_cb(u8 *urc_str, u16 len)
{
    return 1;
}


static URC_Command_Entry_Struct s_urc[] = {
    {"+QIURC: \"recv\","     ,qi_urc_socket_recv_cb},
    {"+QIURC: \"close\","    ,qi_urc_socket_close_cb},
    {"+QIURC: \"dnsgip\","   ,qi_urc_dns_prase_cb },
    {"+QIURC: \"pdpdeact\"," ,qi_urc_pdp_deact_cb },
    //{"+CGREG:"               ,qi_urc_cgreg_cb},
    {"RDY"                   ,ready_urc_recv_cb},
    {"+QIOPEN:"              ,qi_open_urc_recv_cb},
    {"+QHTTPGET:"            ,qi_http_get_urc_recv_cb},
    {"+QHTTPREADFILE:"       ,qi_http_read_urc_recv_cb},
    {"RING"                  ,qi_comming_ring_cb},
    {"+CMTI"                 ,qi_cmti_comming_cb},
    {"+QNTP"                 ,qi_urc_ntp_recv_cb},
    {NULL, NULL}
};



s32 qi_urc_socket_recv_cb(u8 *urc_str, u16 len)
{
    s32 socket_id = -1;
        
    net_task_q_struct *q = NULL;

    gm_soc_notify_ind_struct *soc = NULL;

    u8 *temp = NULL;

    temp = (u8 *)strstr((const char*)urc_str,(const char*)s_urc[0].cmd);

    if(temp)
    {
        temp += strlen(s_urc[0].cmd);

        socket_id = atoi((const char*)temp);

        q = (net_task_q_struct *)malloc(sizeof(net_task_q_struct));

        q->msg_id = NTQ_SOCKET_RECV;
        
        soc = (gm_soc_notify_ind_struct *)q->msg;

        soc->socket_id = socket_id;

        soc->event_type = GM_SOC_READ;

        soc->result = true;
        
        if(g_net_q != NULL)
        OSQPost(g_net_q,(void *)q);
    }


    return 0;
}



s32 qi_urc_socket_close_cb(u8 *urc_str, u16 len)
{
    s32 socket_id = -1;
        
    net_task_q_struct *q = NULL;

    gm_soc_notify_ind_struct *soc = NULL;

    u8 *temp = NULL;

    temp = (u8 *)strstr((const char*)urc_str,(const char*)s_urc[1].cmd);

    q = (net_task_q_struct *)malloc(sizeof(net_task_q_struct));
    
    if(temp)
    {
        temp += strlen(s_urc[1].cmd);

        socket_id = atoi((const char*)temp);

        q = (net_task_q_struct *)malloc(sizeof(net_task_q_struct));

        q->msg_id = NTQ_SOCKET_CLOSE;
            
        soc = (gm_soc_notify_ind_struct *)q->msg;

        soc->socket_id = socket_id;

        soc->event_type = GM_SOC_CLOSE;

        soc->result = true;
        
        if(g_net_q != NULL)
        OSQPost(g_net_q,(void *)q);
    }
    
    return 0;
}


s32 qi_urc_dns_prase_cb(u8 *urc_str, u16 len)
{
     s32 ipcahe = 0;

    s32 error,ip[4];

    u8 *temp;

    net_task_q_struct *q = NULL;

    
    temp = (urc_str + strlen(s_urc[2].cmd));
    
    if(temp)
    {
         if(sscanf((const char*)temp,"%d,%d",&error,&ipcahe) == 2)
         {
             LOG(INFO,"clock(%d) error %d ipcahe %d ,str %s",clock(),error,ipcahe,temp);

             if(error == 0)
             {
                 temp = strstr((const char *)(urc_str + strlen(s_urc[2].cmd)),"+QIURC");

                 temp = (temp + strlen(s_urc[2].cmd) + 1);

                 cl_log(INFO,"clock(%d) get ip string %s",clock(),temp);

                 if(sscanf((const char*)(temp),"%d.%d.%d.%d",ip,ip+1,ip+2,ip+3) == 4)
                 {
                     q = malloc(sizeof(net_task_q_struct));

                     if(q)
                     {
                         q->msg_id = NTQ_DNS_PRASE;

                         q->msg[0] = (u8)ip[0];

                         q->msg[1] = (u8)ip[1];

                         q->msg[2] = (u8)ip[2];

                         q->msg[3] = (u8)ip[3];

                         q->msg_len = 4;

                         OSQPost(g_net_q,(void *)q);
                     }
                 }
             }
         }
    }
    
    
    return 0;
}


CmdExeResultEnum get_at_exe_result(const char *result)
{
    u8 i = 0;

    for(i = 0 ; i < AT_RES_UNDEF ; i++)
    {
        if(strstr(result,at_exe_result_string[i]) != NULL)
        {
            break;
        }
    }

    return (CmdExeResultEnum)i;
}

u8 *find_first_0d0a(u8 *data)
{
    u8 i = 0;
    while(data[i] != '\0')
    {
        if((data[i] == 0x0D)&&(data[i + 1] == 0x0A))
        {
            return (data+i);
        }

        i++;
    }

    return NULL;
}

s32 at_socket_data_recv(s32 socket_id , u8 *recv , u16 read_len)
{
    INT8U err;
	
	s32 res;

    u16 jval = 0;

    u8 *temp = NULL;

    int rec_len = 0;
    
    OSSemPend(g_atsend_sem, 0, &err);

    memset(s_at.cmd_buf,0,sizeof(s_at.cmd_buf));

    jval = sprintf(s_at.cmd_buf,"AT+QIRD=%d,%d\r\n",socket_id,read_len);

    s_at.cmdId = AT_READ_SOCKET;

    //cl_log(AT_INFO,"%s",s_at.cmd_buf);
    
    if(mod_com_write((u8 *)s_at.cmd_buf, jval) != jval)
    {
        OSSemPost(g_atsend_sem);

        s_at.cmdId = AT_INVALID;
        
        return -3;
    }

    //OSSemPend(g_atresp_sem, 600, &err);
    err = wait_at_resp(600);

    if(err == OS_ERR_TIMEOUT)
    {
        OSSemPost(g_atsend_sem);

        s_at.cmdId = AT_INVALID;

        return -3;
    }

    temp = (u8 *)strstr((const char*)s_at.resp,(const char*)SOCKET_READ_MARK);

    if(temp)
    {
        rec_len = atoi((const char *)(temp + strlen(SOCKET_READ_MARK)));

        if(rec_len > 0)
        {
            temp = find_first_0d0a(temp);

            memcpy(recv,temp+2,rec_len);

            res = rec_len;
        }
        else
        {
            res = -2;
        }
    }
    else
    {
        res = -3;
    }

    OSSemPost(g_atsend_sem);

    s_at.cmdId = AT_INVALID;

    return res;
}
s32 at_transver_command(const char * format, ... )
{
    u16 jval = 0;
    
    va_list args;  
    
    va_start (args, format);

    s_at.cmdId = AT_QFILE;

    memset(s_at.cmd_buf,0,sizeof(s_at.cmd_buf));

    jval = vsprintf(s_at.cmd_buf,format, args);

    va_end (args);

    //cl_log(AT_INFO,"%s",s_at.cmd_buf);

    s_at.resp[0] = 0;

    if(mod_com_write((u8 *)s_at.cmd_buf, jval) != jval)
    {
        return ERROR;
    }

    return SUCCESS;
}
#if 1
s32 at_socket_data_send(s32 socket_id , u8 *data , u16 len)
{
    s32 res = AT_RES_ERR;
    
    INT8U err;
    
    lock_at_channel();
    
    if(at_transver_command("AT+QISEND=%d,%d\r\n",socket_id,len) == SUCCESS)
    {
        err = wait_at_resp(1500);

        if(err == OS_ERR_NONE)
        {
           #if 0
           if(strstr((const char *)get_at_resp_addr(),">") == NULL)
           {
              LOG(INFO,"SEND ERROR");
           }
           else
           {
               if(mod_com_write(data, len) == len)
               {
                   err = wait_at_resp(600);

                   if(err == OS_ERR_NONE)
                   {
                        if(strstr((const char *)get_at_resp_addr(),"OK") != NULL)
                        {
                            cl_log_hex(data,len,"[DATA]clock(%d) socket(%d)->",clock(),socket_id);

                            res = AT_RES_OK;
                        }
                   }
               }
           }
           #else
           if(strstr((const char *)get_at_resp_addr(),">") == NULL)
           {
              LOG(INFO,"SEND ERROR");
              OSTimeDly(1);
           }

           if(mod_com_write(data, len) == len)
           {
               err = wait_at_resp(600);

               if(err == OS_ERR_NONE)
               {
                    if(strstr((const char *)get_at_resp_addr(),"OK") != NULL)
                    {
                        cl_log_hex(data,len,"[DATA]clock(%d) socket(%d)->",clock(),socket_id);

                        res = AT_RES_OK;
                    }
               }
           }
           #endif
        }
        else
        {
            LOG(INFO,"SEND ERROR");
        }
    }
    
    release_at_channel();


    return res;
}
#else
s32 at_socket_data_send(s32 socket_id , u8 *data , u16 len)
{
    s32 res = AT_RES_ERR;
    
    INT8U err;
    
    lock_at_channel();
    
    if(at_transver_command("AT+QISEND=%d,%d\r\n",socket_id,len) == SUCCESS)
    {
        OSTimeDly(20);
        
        //err = wait_at_resp(600);

        if(mod_com_write(data, len) == len)
        {
            err = wait_at_resp(600);
 
            if(err == OS_ERR_NONE)
            {
                 if(strstr((const char *)get_at_resp_addr(),"OK") != NULL)
                 {
                     cl_log_hex(data,len,"[DATA]clock(%d) socket(%d)->",clock(),socket_id);
 
                     res = AT_RES_OK;
                 }
            }
        }

    }
    
    release_at_channel();


    return res;
}

#endif

ErrorStatus ril_exe_command(u32 timeout,u8 *need_resp,u8 *para,u16 max_len,const char * format, ... )
{
    INT8U err;

    ErrorStatus res = SUCCESS;
    
    u16 jval = 0;

    u32 over_time = timeout/5;
    
    va_list args;  
    
    va_start (args, format);

    OSSemPend(g_atsend_sem, 0, &err);

    s_at.cmdId = AT_QFILE;

    memset(s_at.cmd_buf,0,sizeof(s_at.cmd_buf));

    jval = vsprintf(s_at.cmd_buf,format, args);

    va_end (args);

    //cl_log(DEBUG,"clock(%d)%s",clock(),s_at.cmd_buf);

    if(mod_com_write((u8 *)s_at.cmd_buf, jval) != jval)
    {
        s_at.cmdId = AT_INVALID;
    
        OSSemPost(g_atsend_sem);
        
        return ERROR;
    }

    memset(s_at.resp,0,sizeof(s_at.resp));

    res = ERROR;

    while(over_time > 0)
    {
        OSTimeDly(1);

        if(need_resp)
        {
            if(strlen(s_at.resp) > 0)
            {
                if(strstr((const char*)s_at.resp,(const char*)need_resp) == NULL)
                {  

                    if(strstr(s_at.resp,"ERROR") != NULL)
                    {
                        res = ERROR;

                        cl_log(ERR,"clcok(%d) Error command %s",clock(),s_at.cmd_buf);

                        cl_log(ERR,"clock(%d) At command recv Error resp:%s",clock(),s_at.resp);
                        break;
                    }
                    
                }
                else
                {
                    //cl_log(DEBUG,"clock(%d) At command recv correct resp:%s",clock(),s_at.resp);
                    
                    if(para!= NULL && max_len > 0)
                    {
                        jval = strlen((const char *)s_at.resp);
                        
                        jval = max_len<jval?max_len:jval;

                        memcpy(para,s_at.resp,jval);
                    }

                    res = SUCCESS;
                    break;
                }
            }
            
        }

        over_time--;
    }

    if(over_time == 0)
    {
        cl_log(ERR,"clcok(%d) cannot get rep command %s",clock(),s_at.cmd_buf);
        
        cl_log(ERR,"clock(%d)Cannnot get At command resp!",clock());
    }

    s_at.cmdId = AT_INVALID;
    
    memset(s_at.resp,0,sizeof(s_at.resp));
    
    OSSemPost(g_atsend_sem);

    return res;
}

void lock_at_channel(void)
{
    INT8U err;
    
    OSSemPend(g_atsend_sem, 0, &err);
}


void release_at_channel(void)
{
    s_at.cmdId = AT_INVALID;
    
    OSSemPost(g_atsend_sem);
}

INT8U wait_at_resp(u32 timeout)
{
    INT8U err;

    OSSemSet(g_atresp_sem,0,&err);
    
    OSSemPend(g_atresp_sem,timeout, &err);

    return err;
}

u8 *get_at_resp_addr(void)
{
    return (u8*)s_at.resp;
}


u8 urc_prase_process(const char *line)
{
    char *ptr = NULL;

    u8 i = 0;

    for(i = 0; s_urc[i].cmd != NULL ; i++)
    {
        ptr = strstr(line,s_urc[i].cmd);

        if(ptr != NULL)
        {
            //cl_log(INFO,"clock(%d) URC %s",clock(),line);

            if(s_urc[i].urc_cb != NULL)
            {
                s_urc[i].urc_cb((u8 *)ptr,strlen(line));

                return 1;
            }

            //break;
        }
    }

    return 0;
}

u8 get_resp_line_from_fifo(u8 *line)
{
    //u8 byte = 0 ,len = 0 ,preb = 0;

    u16 len = 0;

    if(fifo_peek_until(mod_channel_fifo_addr(),line,&len, 0x0A))
    {
        return len;
    }
    /*
    while(fifo_retrieve(mod_channel_fifo_addr(),&byte, 1) > 0)
    {
        //fifo_pop_len(mod_channel_fifo_addr(), 1);
        
        line[len++] = byte;

        if(preb == 0x0D && byte == 0x0A && len > 2)
        {
            return len;
        }

        preb = byte;
    }
    */

    return 0;

}


CmdExeResultEnum at_command_resp_prase(u8 *line)
{
    u16 resp_len = 0,len = 0;

    CmdExeResultEnum res = AT_RES_UNDEF;
    
    resp_len = 0;

    do{

        len = get_resp_line_from_fifo(&line[resp_len]);

        resp_len += len;

        line[resp_len] = 0;

        res = get_at_exe_result((const char *)line);

        if(res != AT_RES_UNDEF)
        {
            //OSSemPost(g_atresp_sem);

            //return res;
            break;
        }
    
    }while(len > 0);

    return res;
}

bool at_praser_proc(void)
{
    u16 j,recv_len = 0;

    u8 sentence[128] = {0};

    fifo_TypeDef *fifo = mod_channel_fifo_addr();

    j = 0;

    while(fifo_peek_one_sentence(fifo,sentence,&recv_len,sizeof(sentence)) == SUCCESS)
    {
        fifo_pop_len(fifo,recv_len);

        sentence[recv_len] = 0;
        
        if(recv_len != 2)
        {
            if(!urc_prase_process((const char*)sentence))
            {
                j += sprintf((char*)&s_at.resp[j],"%s",sentence);
            }
        }
        else
        {
            fifo_peek(fifo,&sentence[recv_len],1);

            if(sentence[recv_len] == '>')
            {
                fifo_pop_len(fifo,1);
                
                sentence[++recv_len] = 0;

                j += sprintf((char*)&s_at.resp[j],"%s",sentence);
            }
        }
    }

    if(recv_len > 0 && j > 0)
    {
        recv_len  = fifo_GetLen(fifo);

        fifo_peek(fifo,&s_at.resp[j],recv_len);

        fifo_pop_len(fifo,recv_len);

        j += recv_len;

    }

    s_at.resp[j] = 0;

    return j > 0?true:false;

}


void at_task(void *pdata)
{   
    u8 at_result = 0;

    u16 resp_len  =0;

    //u8 *p = NULL;
    
    memset(&s_model,0,sizeof(model_control_struct));

    g_atsend_sem  = OSSemCreate(1);

    g_atresp_sem  = OSSemCreate(0);

    g_http_get_sem= OSSemCreate(0);
    
    while(1)
    {
        if(is_mod_channel_read_complete())
        {
            resp_len = mod_channel_rec_data_len();

            if(resp_len > 0)
            {
                #if 0

                if(at_praser_proc())
                {
                    OSSemPost(g_atresp_sem);
                }
                
                #else
                if(s_at.cmdId != AT_INVALID)
                {
                    
                    if((s_at.cmdId == AT_SOCKET_SEND)||(s_at.cmdId == AT_READ_SOCKET)||(s_at.cmdId == AT_QFILE))
                    {
                        fifo_peek(mod_channel_fifo_addr(),(u8 *)s_at.resp,resp_len);

                        s_at.resp[resp_len] = 0;

                        fifo_pop_len(mod_channel_fifo_addr(),resp_len);
                        #if 0
                        p = (u8 *)strstr((const char*)s_at.resp,"+QIURC: \"recv\",");

                        if(p != NULL)
                        {
                            qi_urc_socket_recv_cb(p,resp_len);
                        }
                        #else
                        urc_prase_process((const char*)s_at.resp);

                        OSSemPost(g_atresp_sem);
                        #endif

                    }
                    else
                    {
                       
                        resp_len = 0;

                        at_result = at_command_resp_prase((u8 *)&s_at.resp[resp_len]);

                        //cl_log(INFO,"%s",(u8 *)s_at.resp);

                        OSSemPost(g_atresp_sem);
                        #if 0
                        if(at_result != AT_RES_UNDEF)
                        {
                            
                            s_at.cur_at_result = s_at_cmd[s_at.cmdId].callback(at_result,(const char*)s_at.resp,resp_len,s_at.arg);

                            OSSemPost(g_atresp_sem);
                        }
                        #endif
                    }
                    
                }
                else /* urc*/
                {
                    /**/
                        resp_len = fifo_GetLen(mod_channel_fifo_addr());//get_resp_line_from_fifo((u8 *)s_at.resp);

                        fifo_peek(mod_channel_fifo_addr(),s_at.resp,resp_len);

                        fifo_pop_len(mod_channel_fifo_addr(),resp_len);

                        s_at.resp[resp_len] = 0;

                        if(resp_len > 2)
                        {
                            cl_log(INFO,"[URC]%s",s_at.resp);
                        }
                        
                        urc_prase_process((const char*)s_at.resp);

                        
                }
                #endif
            }
        }
        OSTimeDly(1);
        iwdg_feed();
    }
}

