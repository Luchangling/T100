
#ifndef __ERROR_CODE_H__
#define __ERROR_CODE_H__

typedef enum 
{
	GM_SUCCESS = 0,             //成功
	GM_UNKNOWN = -1,            //未知错误
	GM_NOT_INIT = -2,           //未初始化
	GM_MEM_NOT_ENOUGH = -3,     //内存不足
	GM_PARAM_ERROR = -4,        //参数错误
	GM_EMPTY_BUF = -5,          //没有数据
    GM_ERROR_STATUS = -6,       //状态错误
    GM_HARD_WARE_ERROR = -7,    //硬件错误
    GM_SYSTEM_ERROR = -8,       //系统错误
    GM_WILL_DELAY_EXEC = -9,    //延迟执行
    GM_NET_ERROR = -10,         //网络错误 
}GM_ERRCODE;

#endif



