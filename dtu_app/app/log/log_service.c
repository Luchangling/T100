/**
 * Copyright @ Goome Technologies Co., Ltd. 2009-2019. All rights reserved.
 * File name:        log_service.h
 * Author:               
 * Version:          1.0
 * Date:             2019-12-04
 * Description:      
 * Others:      
 * Function List:    
    1. 创建log_service模块
    2. 销毁log_service模块
    3. log_service模块定时处理入口
    4. 打印日志到串口
    5. 上传日志到日志后台
 * History: 
    1. Date:         2019-12-04
       Author:      
       Modification: 创建初始版本
    2. Date: 		 
	   Author:		 
	   Modification: 

 */

#include <stdarg.h>
#include <stdio.h> 
#include "sys.h"
#include "log_service.h"
#include "config_service.h"
#include "utility.h"
#include "usart.h"
#include "gprs.h"
#include "system_state.h"
#include "socket.h"
#include "net_task.h"
#define LOG_PING_TIME 60

typedef struct
{
    U8* buf;
    u16 len;
}LogSaveData;


/*它是个fifo*/
typedef struct
{
    LogSaveData *his;
    u16 size;  /*总空间*/
    u16 read_idx;  /*当前读到哪一条, read==write 表示队列空*/
    u16 write_idx;  /*当前写到哪一条, 新消息往write处写*/
}LogCurData;

typedef struct
{
    u32 data_finish_time;
	U32 log_seq;
}LogServiceExtend;


static fifo_TypeDef s_log_data_fifo = {0,0,0,0,0};

static LogServiceExtend s_log_socket_extend = {0,0};

#define GM_LOG_MAX_LEN 1536


static SocketType s_log_socket = {-1,"",SOCKET_STATUS_ERROR,};



static GM_ERRCODE log_service_transfer_status(u8 new_status);
static void log_service_init_proc(void);
static void log_service_connecting_proc(void);
static void log_service_work_proc(void);
static void log_msg_receive(void);
static void log_service_close(void);
static void log_service_data_finish_proc(void);
static GM_ERRCODE log_data_insert_one(U8* data, u16 len);
static void log_data_release(LogSaveData *one);

static GM_ERRCODE log_data_insert_one(U8* data, u16 len)
{
    GM_ERRCODE ret = GM_SUCCESS;
    LogSaveData log_data = {0};
	//多申请一个字节，放字符串结束符
    log_data.buf = (U8* )malloc(len + 1);
    if(NULL == log_data.buf)
    {
        //这里不适合再调用 log_service_print，出错会导致递归, 打印只求速死
        LOG(WARN,"clock(%d) log_data_insert_one assert(GM_MemoryAlloc(%d) failed", util_clock(), len);
        return GM_SYSTEM_ERROR;
    }
    log_data.len = len;
    memcpy(log_data.buf, data, len);
	log_data.buf[len] = '\0';


    if(!fifo_insert(&s_log_data_fifo, (u8*)&log_data, sizeof(LogSaveData)))
    {
        ret = GM_MEM_NOT_ENOUGH;
        //这里不适合再调用 log_service_print，出错会导致递归
        free(log_data.buf);
    }
    return ret;
}

static void log_data_release(LogSaveData *one)
{
    if(one && one->buf)
    {
        free(one->buf);
        one->buf = NULL;
        one->len = 0;
    }
}

static GM_ERRCODE log_service_transfer_status(u8 new_status)
{
    u8 old_status = (u8)s_log_socket.status;
    GM_ERRCODE ret = GM_PARAM_ERROR;
    switch(s_log_socket.status)
    {
        case SOCKET_STATUS_INIT:
            switch(new_status)
            {
                case SOCKET_STATUS_INIT:
                    break;
                case SOCKET_STATUS_GET_HOST:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_CONNECTING:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_WORK:
                    break;
                case SOCKET_STATUS_DATA_FINISH:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_ERROR:
                    ret = GM_SUCCESS;
                    break;
                default:
                    break;
            }
            break;
        case SOCKET_STATUS_GET_HOST:
            switch(new_status)
            {
                case SOCKET_STATUS_INIT:
                    break;
                case SOCKET_STATUS_GET_HOST:
                    break;
                case SOCKET_STATUS_CONNECTING:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_WORK:
                    break;
                case SOCKET_STATUS_DATA_FINISH:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_ERROR:
                    ret = GM_SUCCESS;
                    break;
                default:
                    break;
            }
            break;
        case SOCKET_STATUS_CONNECTING:
            switch(new_status)
            {
                case SOCKET_STATUS_INIT:
                    break;
                case SOCKET_STATUS_GET_HOST:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_CONNECTING:
                    break;
                case SOCKET_STATUS_WORK:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_DATA_FINISH:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_ERROR:
                    ret = GM_SUCCESS;
                    break;
                default:
                    break;
            }
            break;
        case SOCKET_STATUS_WORK:
            switch(new_status)
            {
                case SOCKET_STATUS_INIT:
                    break;
                case SOCKET_STATUS_GET_HOST:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_CONNECTING:
                    break;
                case SOCKET_STATUS_WORK:
                    break;
                case SOCKET_STATUS_DATA_FINISH:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_ERROR:
                    ret = GM_SUCCESS;
                    break;
                default:
                    break;
            }
            break;
        case SOCKET_STATUS_DATA_FINISH:
            switch(new_status)
            {
                case SOCKET_STATUS_INIT:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_GET_HOST:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_CONNECTING:
                    break;
                case SOCKET_STATUS_WORK:
                    break;
                case SOCKET_STATUS_DATA_FINISH:
                    break;
                case SOCKET_STATUS_ERROR:
                    ret = GM_SUCCESS;
                    break;
                default:
                    break;
            }
            break;
        case SOCKET_STATUS_ERROR:
            switch(new_status)
            {
                case SOCKET_STATUS_INIT:
                    ret = GM_SUCCESS;
                    break;
                case SOCKET_STATUS_GET_HOST:
                    break;
                case SOCKET_STATUS_CONNECTING:
                    break;
                case SOCKET_STATUS_WORK:
                    break;
                case SOCKET_STATUS_DATA_FINISH:
                    break;
                case SOCKET_STATUS_ERROR:
                    ret = GM_SUCCESS;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }


    if(GM_SUCCESS == ret)
    {
        s_log_socket.status = new_status;
        s_log_socket.status_fail_count = 0;
        LOG(INFO,"clock(%d) log_service_transfer_status from %s to %s success", util_clock(), 
            gm_socket_status_string((SocketStatus)old_status), gm_socket_status_string((SocketStatus)new_status));
    }
    else
    {
        LOG(INFO,"clock(%d) log_service_transfer_status from %s to %s failed", util_clock(), 
            gm_socket_status_string((SocketStatus)old_status), gm_socket_status_string((SocketStatus)new_status));
    }

    return ret;

}


GM_ERRCODE log_service_create(void)
{
    u8 addr[2*GOOME_DNS_MAX_LENTH+1];
    u8 IP[4] = {0};
    u32 port = 0;
    u8 idx = 0;

    if(!s_log_socket.fifo.baseAddr)
    gm_socket_init(&s_log_socket, SOCKET_INDEX_LOG);
    s_log_socket_extend.data_finish_time = 0;

    memset(addr, 0x00, sizeof(addr));
    idx = sscanf((const char *)config_service_get_pointer(CFG_LOGSERVERADDR), "%[^:]:%d", addr, &port);
    if (idx != 2)
    {
        LOG(WARN,"clock(%d) log_service_create assert(idx ==2) failed.", util_clock());
        return GM_PARAM_ERROR;
    }
    
    if(GM_SUCCESS != util_convertipaddr(addr, IP))
    {
        if(util_is_valid_dns(addr, strlen((const char *)addr)))
        {
            gm_socket_set_addr(&s_log_socket, addr, strlen((const char *)addr), port, STREAM_TYPE_DGRAM);
        }
        else
        {
            LOG(WARN,"clock(%d) log_service_create assert(dns(%s)) failed.", util_clock(), addr);
            return GM_PARAM_ERROR;
        }
    }
    else
    {
        gm_socket_set_ip_port(&s_log_socket, IP, port, STREAM_TYPE_DGRAM);
        system_state_set_ip_cache(SOCKET_INDEX_LOG, IP);
    }

	fifo_Init(&s_log_data_fifo, LOG_BUFFER_NUM * sizeof(LogSaveData));
    LOG(WARN,"clock(%d) log_service_create access_id(%d) fifo(%p).", util_clock(), s_log_socket.access_id, &s_log_socket.fifo);
	return GM_SUCCESS;
}

GM_ERRCODE log_service_destroy(void)
{
    if(SOCKET_STATUS_ERROR == s_log_socket.status)
    {
        return GM_SUCCESS;
    }
    
    log_service_close();

    //log_service是在gprs_create中创建, 后面 重建连接, 所以保留fifo
    log_service_transfer_status(SOCKET_STATUS_ERROR);
	return GM_SUCCESS;
}


static void log_service_close(void)
{
    if(s_log_socket.id >=0)
    {
        SocketClose(s_log_socket.id);
        s_log_socket.id=-1;
    }
}


static void log_service_data_finish_proc(void)
{
    u32 current_time = 0;
    
    if(!gprs_is_ok())
    {
        return;
    }
    
    current_time = util_clock();
    if((current_time - s_log_socket_extend.data_finish_time) > LOG_PING_TIME)
    {
        LOG(DEBUG,"clock(%d) log_service_data_finish_proc cur(%d) - fin(%d) > LOG_PING_TIME(%d).",
            util_clock(), current_time, s_log_socket_extend.data_finish_time,LOG_PING_TIME);
        // 可以重建连接
        log_service_transfer_status(SOCKET_STATUS_INIT);
    }
}


GM_ERRCODE log_service_timer_proc(void)
{
    if(!s_log_socket.fifo.baseAddr)
    {
        return GM_SUCCESS;
    }

    switch(s_log_socket.status)
    {
    case SOCKET_STATUS_INIT:
        log_service_init_proc();
        break;
    case SOCKET_STATUS_CONNECTING:
        log_service_connecting_proc();
        break;
    case SOCKET_STATUS_WORK:
        log_service_work_proc();
        break;
    case SOCKET_STATUS_DATA_FINISH:  
        log_service_data_finish_proc();
        break;
    case SOCKET_STATUS_ERROR:
        log_service_data_finish_proc();
        break;
    default:
        LOG(WARN,"clock(%d) log_service_timer_proc assert(s_log_socket.status(%d)) unknown.", util_clock(),s_log_socket.status);
        return GM_ERROR_STATUS;
    }

    return GM_SUCCESS;
}


static void log_service_init_proc(void)
{
    u8 IP[4];
    gm_socket_get_host_by_name_trigger(&s_log_socket);
    system_state_get_ip_cache(SOCKET_INDEX_LOG, IP);
    if(GM_SUCCESS == gm_is_valid_ip(IP))
    {
        memcpy( s_log_socket.ip , IP, sizeof(IP));
        log_service_transfer_status(SOCKET_STATUS_CONNECTING);
        if(GM_SUCCESS == gm_socket_connect(&s_log_socket))
        {
        }
        // else do nothing .   connecting_proc will deal.
    }
    else if((!s_log_socket.excuted_get_host) && (GM_SUCCESS == gm_is_valid_ip(s_log_socket.ip)))
    {
        log_service_transfer_status(SOCKET_STATUS_CONNECTING);
        if(GM_SUCCESS == gm_socket_connect(&s_log_socket))
        {
        }
        // else do nothing .   connecting_proc will deal.
    }
}

void log_service_connection_failed(void)
{
    log_service_close();
    
    if(s_log_socket.status_fail_count >= MAX_CONNECT_REPEAT)
    {
        // if excuted get_host transfer to error statu, else get_host.
        if(s_log_socket.excuted_get_host || (s_log_socket.addr[0] == 0))
        {
            s_log_socket_extend.data_finish_time = util_clock();
            log_service_transfer_status(SOCKET_STATUS_DATA_FINISH);
        }
        else
        {
            log_service_transfer_status(SOCKET_STATUS_INIT);
        }
    }
    // else do nothing . wait connecting proc to deal.
}


void log_service_connection_ok(void)
{
    log_service_transfer_status(SOCKET_STATUS_WORK);
}

void log_service_close_for_reconnect(void)
{
    log_service_close();
    s_log_socket_extend.data_finish_time = util_clock();
    log_service_transfer_status(SOCKET_STATUS_DATA_FINISH);
}


static void log_service_connecting_proc(void)
{
    u32 current_time = util_clock();

    if((current_time - s_log_socket.send_time) > CONNECT_TIME_OUT)
    {
        s_log_socket.status_fail_count ++;
        log_service_connection_failed();

        if(s_log_socket.status == SOCKET_STATUS_CONNECTING && 
            s_log_socket.status_fail_count < MAX_CONNECT_REPEAT)
        {
            if(GM_SUCCESS == gm_socket_connect(&s_log_socket))
            {
                //do nothing. wait callback
            }
        }
        
    }
}


#define LOG_PKT_HEAD "<RZ"
#define LOG_PKT_SPLIT '*'
#define LOG_PKT_TAIL '>'

void log_service_send_msg(SocketType *socket)
{
    LogSaveData log_data;
	  u8 i = 0;
	U8 imei[17+ 1] = {0};

    u8 *ptr = NULL;
	ptr = config_service_get_pointer(CFG_JT_CEL_NUM);

    if(!ptr)
    {
        LOG(INFO,"clock(%d) log_service_send_msg can not get imei, ret:%d.", util_clock());
    }
    
    if (0 == strlen((const char *)ptr))
    {
        memset(ptr, 0, sizeof(ptr));
    }
    #if 1
    memset(imei,0x30,CONFIG_MAX_CELL_NUM_LEN);

    if(strlen((const char*)ptr) >= 15)
    {
        i = 0;
    }
    else
    {
        i = 15 - strlen((const char*)ptr);
    }

    memcpy(imei + i,ptr , 15);

    imei[15] = 0;
    #endif

    if(SUCCESS == fifo_peek(&s_log_data_fifo, (U8*)&log_data, sizeof(LogSaveData)))
    {
    	char* p_send_buf = NULL;
		U16 send_buf_len = 0;

		if (log_data.len > GM_LOG_MAX_LEN)
		{
			fifo_pop_len(&s_log_data_fifo, sizeof(LogSaveData));
			LOG(ERR,"clock(%d) log_service_send_msg data is too long(%d)", util_clock(), log_data.len);
			log_data_release(&log_data);
			return;
		}
		
		//一条日志<GM*862964022280089*xxxxxx>,6个字节：LOG_PKT_HEAD——3个字节，2个分隔符，1和结束符
		//日志回复<GM*ACK>
		send_buf_len = 6 + strlen((const char*)imei) + 10 + log_data.len;
		p_send_buf = malloc(send_buf_len + 1);
		memset(p_send_buf, 0, send_buf_len + 1);
		#if 1
		//最多从源字符串format中拷贝size字节的内容(含字符串结尾标志'\0')到目标字符串
		snprintf(p_send_buf, send_buf_len + 1, "%s%c%s%c%d%c%s%c", 
													LOG_PKT_HEAD,
													LOG_PKT_SPLIT,
													(const char*)imei,
													LOG_PKT_SPLIT,
													s_log_socket_extend.log_seq
													,LOG_PKT_SPLIT,
													(const char*)log_data.buf,
	        										LOG_PKT_TAIL);
        #else
        //最多从源字符串format中拷贝size字节的内容(含字符串结尾标志'\0')到目标字符串
		snprintf(p_send_buf, send_buf_len + 1, "%s%c%s%s%c", 
													LOG_PKT_HEAD,
													LOG_PKT_SPLIT,
													(const char*)imei,
													(const char*)log_data.buf,
	        										LOG_PKT_TAIL);
        #endif
        s_log_socket_extend.log_seq++;
		send_buf_len = strlen(p_send_buf);
        if(GM_SUCCESS == gm_socket_send(socket, (U8*)p_send_buf,send_buf_len))
        {
            fifo_pop_len(&s_log_data_fifo, sizeof(LogSaveData));
			LOG(DEBUG,"clock(%d) log_service_send_msg msglen(%d):%s", util_clock(), send_buf_len,p_send_buf);
			log_data_release(&log_data);
        }
        else
        {
        	LOG(ERR,"Failed to send log data!");
            log_service_close();
            s_log_socket_extend.data_finish_time = util_clock();
            log_service_transfer_status(SOCKET_STATUS_DATA_FINISH);
        }
		free(p_send_buf);
    }
    
    return;
}

static void log_service_work_proc(void)
{
    u32 current_time = util_clock();

	//每秒钟最多一条日志，防止发送过快
    if((current_time - s_log_socket.send_time) >= 1)
    {
        log_service_send_msg(&s_log_socket);
		s_log_socket.send_time = current_time;
    }
    log_msg_receive();
}



/*当前服务器是只收不发响应的. 下面这个函数正常来説不会工作 */
static void log_msg_receive(void)
{
    // parse buf msg
    // if OK, after creating other socket, transfer to finish
    // not ok, ignore msg.
    u8 msg[100] = {0};

	u16 len = sizeof(msg);

    if(SOCKET_STATUS_WORK != s_log_socket.status)
    {
        return;
    }
    
    //get head then delete
    if(!fifo_peek_until(&s_log_socket.fifo, msg, &len,'>'))
    {
        // no msg
        return;
    }

    fifo_pop_len(&s_log_socket.fifo, len);

	LOG(DEBUG,"clock(%d) log_msg_receive msg:%s, len:%d", util_clock(), msg,len);

    //do nothing. just read and clear msgs

}

char s_log_str[GM_LOG_MAX_LEN] = {0};

void log_service_upload(LogLevelEnum level,JsonObject* p_root)
{
	//s8 zone = 0;
  //  char date[50] = {0};
	
    memset(s_log_str,0,GM_LOG_MAX_LEN);

    json_add_int(p_root,"clock", util_clock());

	if(json_print_to_buffer(p_root, s_log_str, GM_LOG_MAX_LEN - 1))
	{
		log_data_insert_one((U8*)s_log_str, strlen(s_log_str));
	}

    
    //debug级肯定不再回调log_service_upload
    LOG(DEBUG, s_log_str);
	json_destroy(p_root);
	p_root = NULL;
}


