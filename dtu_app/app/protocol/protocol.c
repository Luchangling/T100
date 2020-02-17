#include <stdio.h>
#include "sys.h"
#include "utility.h"
#include "protocol.h"
#include "protocol_jt808.h"
#include "config_service.h"
#include "gps_save.h"
#include "system_state.h"
#include "gprs.h"
#include "bms_service.h"
#include "fifo.h"
static bool protocol_msg_skiped_wrong(SocketType *socket, u8 *check, u16 check_len, u8 *head, u16 head_len)
{
    u16 start_idx = 0;
    u16 idx = 0;

    LOG(INFO,"clcok(%d) skiped fifo len %d",util_clock(),fifo_GetLen(&socket->fifo));
    
    while(start_idx + check_len <= head_len)
    {
        bool match = true;
        for(idx = 0; idx < check_len; ++idx )
        {
            if(head[start_idx + idx] != check[idx])
            {
                match = false;
                break;
            }
        }

        if(match)
        {
            if(start_idx == 0)
            {
                //继续后面的执行
                
                return true;  
            }
            else
            {
                 //跳掉错误的消息， 等下次循环读取消息头
                LOG(WARN,"clock(%d) protocol_msg_skiped_wrong skiped(%d).", util_clock(), start_idx);
                cl_log_hex((u8 *)head, head_len,"[WARN]Skip head:");
                fifo_pop_len(&socket->fifo, start_idx);
                return false;
            }
        }
        else
        {
            start_idx++;
        }
    }

    //跳掉错误的消息， 等下次循环读取消息头
    LOG(WARN,"clock(%d) protocol_msg_skiped_wrong skiped(%d).", util_clock(), start_idx);
    cl_log_hex((u8 *)head, head_len,"[WARN]Skiped head:");
    fifo_pop_len(&socket->fifo, start_idx);
    return false;  
}

static bool alloc_memory_for_socket_msg(u8 **ppdata, u16 msg_len, SocketType *socket)
{
    if(*ppdata)
    {
        return true;
    }
    
    *ppdata = malloc(msg_len);
    if(*ppdata == NULL)
    {
        LOG(INFO,"clock(%d) alloc_memory_for_socket_msg alloc buf failed. len:%d", util_clock(), msg_len);

        //clear fifo and restart socket.
        fifo_Reset(&socket->fifo);
        system_state_set_gpss_reboot_reason("malloc error.");
        gm_socket_close_for_reconnect(socket);
        return false;
    }
    return true;
}



static bool get_jt808_message(u8 *pdata, u16 *len_p)
{
    u8 *pend;
    if(*pdata != PROTOCOL_HEADER_JT808)
    {
        LOG(WARN,"clock(%d) get_jt808_message assert first byte failed.", util_clock());
        return false;
    }
    
    pend = pdata + 1;
    while((pend - pdata + 1) <= (*len_p))
    {
        if(*pend == PROTOCOL_HEADER_JT808)
        {
            *len_p = (pend - pdata + 1);
            return true;
        }
        pend ++;
    }
    return false;
}

void protocol_msg_receive(SocketType *socket)
{
    u8 head[7];
    u32 len = sizeof(head);
    u16 msg_len = 0;
    u8 *pdata = NULL;
    static u32 packet_error_start = 0;

    if(socket->status != SOCKET_STATUS_LOGIN && socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_msg_receive socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return;
    }
    
    /*
    goome协议   最短7
        信息头(0x67 0x67)    2 协议号1    包长度2(下一字节至包尾)       信息序列号2 信息内容 N
                 
    concox协议 最短10
        起始位 2 包长度(下一字节至错误校验) 1 协议号 1 信息内容 N 信息序列号 2 错误校验 2 停止位 2
 
    jt808协议 最短15
        标志位1(0x7e) 消息头12/14(ID 2 属性2(10位长度) 终端号6 流水2 封装0/4(属性 位13决定)) 消息体N 校验1 标志位1
        注意0x7e转码 0x7e->0x7d+02 0x7d->0x7d+01
    */

    if(fifo_peek(&socket->fifo, head, len) == 0)
    {
        // no msg
        return;
    }

    cl_log_hex(head,sizeof(head),"[DATA]recv head->");

    //cl_log_hex(head,len,"POP head:");

    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
    {
        u8 check[1] = {0x7e};
        u32 msglen;
        if(!protocol_msg_skiped_wrong(socket,check,sizeof(check),head,sizeof(head)))
        {
            return;
        }

        /*
            jt808 要以0x7e至0x7e 作为消息.
            直接取消息中的长度,会导致消息在有转义时取不全
        */
        msglen = MAX_JT808_MSG_LEN;
        if(!alloc_memory_for_socket_msg(&pdata, msglen, socket))
        {
            return;
        }
        if(SUCCESS != fifo_peek_and_get_len(&socket->fifo, pdata, &msglen))
        {
            LOG(INFO,"clock(%d) head %d tail %d size %d",util_clock(),socket->fifo.head,socket->fifo.tail,socket->fifo.size);
            free(pdata);
            return;
        }
        msg_len = msglen;
        if(!get_jt808_message(pdata, &msg_len))
        {
            cl_log_hex(pdata,msglen,"[DATA]error len farame->");
            LOG(WARN,"clock(%d) get_jt808_message error len",clock());
            free(pdata);
            return;
        }
        break;
    }
    default:
        LOG(WARN,"clock(%d) protocol_msg_receive assert(protocol(%d) head(%02x)) failed.", 
            util_clock(), config_service_get_app_protocol(), head[0]);
        return;
    }

    if(msg_len > MAX_GPRS_MESSAGE_LEN)
    {
        LOG(WARN,"clock(%d) protocol_msg_receive assert(msg_len(%d)) failed.",util_clock(), msg_len);
        //clear fifo and restart socket.
        fifo_Reset(&socket->fifo);
        system_state_set_gpss_reboot_reason("msg_len error.");
        gm_socket_close_for_reconnect(socket);

        //actually，for jt808 it is less than MAX_JT808_MSG_LEN，so, will not in this branch
        if(pdata) free(pdata);  
        return;
    }

    if(!alloc_memory_for_socket_msg(&pdata, msg_len, socket))
    {
        return;
    }

    if(SUCCESS != fifo_peek(&socket->fifo, pdata, msg_len))
    {
        // GM_EMPTY_BUF
        free(pdata);
        
        if(packet_error_start == 0)
        {
            LOG(DEBUG,"clock(%d) protocol_msg_receive get msg failed. len:%d", util_clock(), msg_len);
            //log_service_print_hex((const char*)head, sizeof(head));
            packet_error_start = util_clock();
        }
        else
        {
            if((util_clock() - packet_error_start) > MAX_GPRS_PART_MESSAGE_TIMEOUT)
            {
                LOG(WARN,"clock(%d) protocol_msg_receive MAX_GPRS_PART_MESSAGE_TIMEOUT.",util_clock());
                //clear fifo and restart socket.
                fifo_Reset(&socket->fifo);
                system_state_set_gpss_reboot_reason("msg only part.");
                gm_socket_close_for_reconnect(socket);
                packet_error_start = 0;
            }
        }
        return;
    }
    fifo_pop_len(&socket->fifo, msg_len);
	LOG(DEBUG,"clock(%d) protocol_msg_receive msg len(%d) head(%02x)", util_clock(), msg_len, head[0]);

    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
        protocol_jt_parse_msg(pdata,  msg_len);
        break;
    default:
        LOG(WARN,"clock(%d) protocol_msg_receive assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
        break;
    }

    free(pdata);
}






GM_ERRCODE protocol_send_login_msg(SocketType *socket)
{
    u8 buff[200];
    u16 len = sizeof(buff);
    u16 idx = 0;
    u8 value_u8 = 0;
    
    memset(buff, 0x00, sizeof(buff));

    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
        config_service_get(CFG_JT_ISREGISTERED, TYPE_BYTE, &value_u8, sizeof(value_u8));
        if(value_u8)
        {
            protocol_jt_pack_auth_msg(buff, &idx, len); // max 115 bytes
            LOG(DEBUG,"clock(%d) protocol_send_login_msg PROTOCOL_JT808 auth msg len(%d)).", util_clock(), idx);
        }
        else
        {
            protocol_jt_pack_regist_msg(buff, &idx, len);   // max 76 byes
            LOG(DEBUG,"clock(%d) protocol_send_login_msg PROTOCOL_JT808 regist msg len(%d)).", util_clock(), idx);
        }
        break;
    default:
        LOG(WARN,"clock(%d) protocol_send_login_msg assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
        return GM_PARAM_ERROR;
    }
    
    len=idx;  // idx is msg len

    socket->send_time = util_clock();
    LOG(DEBUG,"clock(%d) protocol_send_login_msg len(%d) protocol(%d).", util_clock(), len, config_service_get_app_protocol());
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send login");
        gps_service_destroy_gprs();
        return GM_NET_ERROR;
    }
    
    return GM_SUCCESS;
}

GM_ERRCODE protocol_send_transiver_data_msg(SocketType *socket)
{
    u8 buff[1500];
    u8 *temp = NULL;
    u16 len = sizeof(buff);
    u16 rlen,idx = 0;

    if(socket->status != SOCKET_STATUS_LOGIN && socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_device_msg socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return GM_PARAM_ERROR;
    }

    //temp = bms_service_get_one_data(&rlen);

    if(temp == NULL)
    {
        return GM_SUCCESS;
    }
    
    switch(config_service_get_app_protocol())
    {
        case PROTOCOL_JT808:
            protocol_jt_pack_up_trans_msg(buff,&idx,len,temp,rlen);

            //bms_service_data_que_delete_one();
            
            break;
        default:
            LOG(WARN,"clock(%d) protocol_send_device_msg assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
            return GM_PARAM_ERROR;
    }

    len=idx;  // idx is msg len

    socket->send_time = util_clock();
    LOG(DEBUG,"clock(%d) protocol_send_device_msg len(%d) protocol(%d).", util_clock(), len, config_service_get_app_protocol());
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send devmsg");
        gps_service_destroy_gprs();
        return GM_NET_ERROR;
    }
    return GM_SUCCESS;
}

GM_ERRCODE protocol_send_bms_data_msg(SocketType *socket)
{
    u8 buff[1500];
    //u8 *temp = NULL;
    u16 len = sizeof(buff);
    u16 idx = 0;
    BmsBatteryInfoStruct *bms = NULL;

    BmsBatteryInfoStruct *bms1 = NULL;

    if(socket->status != SOCKET_STATUS_LOGIN && socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_bms_data_msg socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return GM_PARAM_ERROR;
    }

    //temp = bms_service_get_one_data(&rlen);
    bms = (BmsBatteryInfoStruct *)malloc(sizeof(BmsBatteryInfoStruct));

    if(bms == NULL)
    {
        LOG(ERR,"clock(%d) protocol_send_bms_data_msg malloc error",clock());
        return GM_SUCCESS;
    }

    bms1 = bms_service_get_entry_addr();

    *bms = *bms1;
    
    switch(config_service_get_app_protocol())
    {
        case PROTOCOL_JT808:
            //protocol_jt_pack_up_trans_msg(buff,&idx,len,temp,rlen);
            protocol_jt_pack_bms_msg(buff,&idx,len,bms);
            free(bms);
            break;
        default:
            free(bms);
            LOG(WARN,"clock(%d) protocol_send_bms_data_msg assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
            return GM_PARAM_ERROR;
    }

    len=idx;  // idx is msg len

    socket->send_time = util_clock();
    LOG(DEBUG,"clock(%d) protocol_send_bms_data_msg len(%d) protocol(%d).", util_clock(), len, config_service_get_app_protocol());
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send bms");
        gps_service_destroy_gprs();
        return GM_NET_ERROR;
    }
    return GM_SUCCESS;
}


GM_ERRCODE protocol_send_device_msg(SocketType *socket)
{
    u8 buff[50];
    u16 len = sizeof(buff);
    u16 idx = 0;

    if(socket->status != SOCKET_STATUS_LOGIN && socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_device_msg socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return GM_PARAM_ERROR;
    }
    
    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
        protocol_jt_pack_iccid_msg(buff, &idx, len);  // max 39 bytes
        break;
    default:
        LOG(WARN,"clock(%d) protocol_send_device_msg assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
        return GM_PARAM_ERROR;
    }

    len=idx;  // idx is msg len

    socket->send_time = util_clock();
    LOG(DEBUG,"clock(%d) protocol_send_device_msg len(%d) protocol(%d).", util_clock(), len, config_service_get_app_protocol());
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send devmsg");
        gps_service_destroy_gprs();
        return GM_NET_ERROR;
    }
    
    return GM_SUCCESS;
}


GM_ERRCODE protocol_send_heartbeat_msg(SocketType *socket)
{
    u8 buff[20];
    u16 len = sizeof(buff);
    u16 idx = 0;

    if(socket->status != SOCKET_STATUS_LOGIN && socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_heartbeat_msg socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return GM_PARAM_ERROR;
    }
    
    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
        protocol_jt_pack_heartbeat_msg(buff, &idx, len);  // max 19 bytes
        break;
    default:
        LOG(WARN,"clock(%d) protocol_send_heartbeat_msg assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
        return GM_PARAM_ERROR;
    }

    len=idx;  // idx is msg len

    LOG(DEBUG,"clock(%d) protocol_send_heartbeat_msg len(%d) protocol(%d).", util_clock(), len, config_service_get_app_protocol());
    socket->send_time = util_clock();
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send heart");
        gps_service_destroy_gprs();
        return GM_NET_ERROR;
    }
    return GM_SUCCESS;
}



u32 protocol_send_gps_msg(SocketType *socket)
{

    LocationSaveData one;
	u32 send_counts = 0;

    if(socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_gps_msg socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return 0;
    }

    
    //先发送最新的数据
    while(GM_SUCCESS == gps_service_peek_one(&one, false))
    {
        LOG(DEBUG,"clock(%d) protocol_send_gps_msg len(%d).", util_clock(), one.len);
        if(one.len == 0)
        {
            gps_service_commit_peek(false);
            continue;
        }

        if(GM_SUCCESS == gps_service_cache_send(one.buf, one.len))
        {
            gps_service_commit_peek(false);
			send_counts++;
            if(send_counts >= MAX_GPS_MSGS_PER_SEND)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    
    return send_counts;
}


GM_ERRCODE protocol_send_logout_msg(SocketType *socket)
{
    u8 buff[20];
    u16 len = sizeof(buff);
    u16 idx = 0;

    #if 0
    if(socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_logout_msg socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return GM_PARAM_ERROR;
    }
    #endif
    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
        protocol_jt_pack_logout_msg(buff, &idx, len);  // 15 bytes
        break;
    default:
        LOG(WARN,"clock(%d) protocol_send_logout_msg assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
        return GM_PARAM_ERROR;
    }

    len=idx;  // idx is msg len
    
    LOG(DEBUG,"clock(%d) protocol_send_logout_msg len(%d) protocol(%d).", util_clock(), len, config_service_get_app_protocol());

    socket->send_time = util_clock();
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send logout");
        gps_service_destroy_gprs();
        return GM_NET_ERROR;
    }
    
    return GM_SUCCESS;
}


GM_ERRCODE protocol_send_remote_ack(SocketType *socket, u8 *pRet, u16 retlen)
{
    u8 *buff;
    u16 len = 30 + retlen;  //jt808 need 22 bytes
    u16 idx = 0;

    if(socket->status != SOCKET_STATUS_LOGIN && socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_remote_ack socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return GM_PARAM_ERROR;
    }

    buff = malloc(len);
    if (NULL == buff)
    {
        LOG(WARN,"clock(%d) protocol_send_remote_ack assert(GM_MemoryAlloc(%d)) failed.", util_clock(), len);
        return GM_MEM_NOT_ENOUGH;
    }
    
    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
        protocol_jt_pack_remote_ack(buff, &idx, len, pRet, retlen);  //22|26 +retlen bytes
        break;
    default:
        LOG(WARN,"clock(%d) protocol_send_remote_ack assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
        free(buff);
        return GM_PARAM_ERROR;
    }

    len=idx;  // idx is msg len

    socket->send_time = util_clock();
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send remote_ack");
        gps_service_destroy_gprs();
        free(buff);
        return GM_NET_ERROR;
    }
    free(buff);
    return GM_SUCCESS;
}



GM_ERRCODE protocol_send_general_ack(SocketType *socket)
{
    u8 buff[50];
    u16 len = sizeof(buff); 
    u16 idx = 0;

    if(socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_remote_ack socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return GM_PARAM_ERROR;
    }
    
    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
        protocol_jt_pack_general_ack(buff, &idx, len);  //max 24 bytes
        break;
    default:
        LOG(WARN,"clock(%d) protocol_send_general_ack assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
        return GM_PARAM_ERROR;
    }

    len=idx;  // idx is msg len
    LOG(DEBUG,"clock(%d) protocol_send_general_ack len(%d) protocol(%d).", util_clock(), len, config_service_get_app_protocol());

    socket->send_time = util_clock();
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send geneal_ack");
        gps_service_destroy_gprs();
        return GM_NET_ERROR;
    }
    return GM_SUCCESS;
}



GM_ERRCODE protocol_send_param_get_ack(SocketType *socket)
{
    u8 *buff;
    u16 len = 1000; 
    u16 idx = 0;

    if(socket->status != SOCKET_STATUS_WORK)
    {
        LOG(WARN,"clock(%d) protocol_send_remote_ack socket->status(%s) error.", util_clock(), gm_socket_status_string((SocketStatus)socket->status));
        return GM_PARAM_ERROR;
    }
    

    buff = malloc(len);
    if (NULL == buff)
    {
        LOG(WARN,"clock(%d) protocol_send_param_get_ack assert(GM_MemoryAlloc(%d)) failed.", util_clock(), len);
        return GM_MEM_NOT_ENOUGH;
    }
    
    switch(config_service_get_app_protocol())
    {
    case PROTOCOL_JT808:
        protocol_jt_pack_param_ack(buff, &idx, len); 
        break;
    default:
        LOG(WARN,"clock(%d) protocol_send_param_get_ack assert(app protocol(%d)) failed.", util_clock(), config_service_get_app_protocol());
        free(buff);
        return GM_PARAM_ERROR;
    }

    len=idx;  // idx is msg len
    LOG(DEBUG,"clock(%d) protocol_send_param_get_ack len(%d) protocol(%d).", util_clock(), len, config_service_get_app_protocol());

    socket->send_time = util_clock();
    if(GM_SUCCESS != gm_socket_send(socket, buff, idx))
    {
        system_state_set_gpss_reboot_reason("gm_socket_send param_ack");
        gps_service_destroy_gprs();
        free(buff);
        return GM_NET_ERROR;
    }
    free(buff);
    return GM_SUCCESS;
}



