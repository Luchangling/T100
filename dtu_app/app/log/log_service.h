/**
 * File name:        log_service.h
 * Author:                  
 * Version:          1.0
 * Date:             2019-03-01
 * Description:      
 * Others:      
 * Function List:    
    1. 创建log_service模块
    2. 销毁log_service模块
    3. log_service模块定时处理入口
    4. 打印日志到串口
    5. 上传日志到日志后台
 * History: 
    1. Date:         2019-03-01
       Author:       
       Modification: 创建初始版本
    2. Date: 		 
	   Author:		 
	   Modification: 

 */

#ifndef __LOG_SERVICE_H__
#define __LOG_SERVICE_H__

#include "sys.h"
#include "errorcode.h"
#include "json.h"
#include "socket.h"
#include <stdio.h>

#define LOG_BUFFER_NUM 5


/**
 * Function:   创建log_service模块
 * Description:创建log_service模块
 * Input:	   无
 * Output:	   无
 * Return:	   GM_SUCCESS——成功；其它错误码——失败
 * Others:	   使用前必须调用，否则调用其它接口返回失败错误码
 */
GM_ERRCODE log_service_create(void);

/**
 * Function:   销毁log_service模块
 * Description:销毁log_service模块
 * Input:	   无
 * Output:	   无
 * Return:	   GM_SUCCESS——成功；其它错误码——失败
 * Others:	   
 */
GM_ERRCODE log_service_destroy(void);

/**
 * Function:   log_service模块定时处理入口
 * Description:log_service模块定时处理入口
 * Input:	   无
 * Output:	   无
 * Return:	   GM_SUCCESS——成功；其它错误码——失败
 * Others:	   
 */
GM_ERRCODE log_service_timer_proc(void);


/*由socket层回调*/
void log_service_connection_failed(void);
void log_service_connection_ok(void);
void log_service_close_for_reconnect(void);



void log_service_upload(LogLevelEnum level,JsonObject* p_root);

#endif

