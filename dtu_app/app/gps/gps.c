/**
 gps.c
 */

#include <stdio.h>
#include <math.h>
#include "gps.h"
#include "applied_math.h"
#include "nmea_protocol.h"
#include "config_service.h"

#include "circular_queue.h"
#include "gps_service.h"
#include "agps_service.h"
#include "system_state.h"

#include "utility.h"
#include "usart.h"
#include "at_command.h"
#include "led_service.h"
#include "command.h"
#include "log_service.h"
#include "net_task.h"
#define GPS_BUFF_LEN 10


typedef enum 
{	//未初始化
	GM_GPS_STATE_NONE = 0,
		
	//初始化OK
	GM_GPS_STATE_INITED,

	//发送了版本号查询,等待版本号响应
	GM_GPS_STATE_WAIT_VERSION,

	//发送了AGPS时间请求,等待AGPS时间响应
	GM_GPS_STATE_WAIT_APGS_TIME,
	
	//发送了AGPS数据请求,等待AGPS数据响应
	GM_GPS_STATE_WAIT_APGS_DATA,

	//工作中
	GM_GPS_STATE_WORKING,
	
}GPSInternalState;

typedef struct
{
	//使用的波特率
	U32 baud_rate;

	//GPS芯片型号
	GPSChipType gpsdev_type;

	//内部状态
	GPSInternalState internal_state;

	//整体状态
	GPSState state;

	//定位状态时间记录
    StateRecord state_record;

	//存放定位数据的环形队列
	CircularQueue gps_time_queue;
	CircularQueue gps_lat_queue;
	CircularQueue gps_lng_queue;
	CircularQueue gps_speed_queue;
	CircularQueue gps_course_queue;

	//启动时间
	time_t power_on_time;

	time_t mtk_start_time;

	time_t last_rcv_time;

	bool push_data_enbale;

	//AGPS完成注入时间
	u16 agps_time;

	//定位时间
	time_t fix_time;

	//内存中保存时间（每秒一条）
	time_t save_time;

	//上报平台时间
	time_t report_time;

	time_t sleep_time;

	//从上次上报点到当前点的距离（曲线距离）
	float distance_since_last_report;

	//是否已打开VTG语句（泰斗芯片需要打开VTG语句）
	bool has_opened_td_vtg;
	
	NMEASentenceRMC frame_rmc;
	NMEASentenceGSV frame_gsv;


	//辅助定位经纬度
	float ref_lng;
	float ref_lat;

	//定位状态
	//GPSState的第0位表示是否定位定位,0——未定位；1——已定位
	bool is_fix;
	
	//GPSState的第1位表示2D/3D定位,0——2D；1——3D
	bool is_3d;

	//GPSState的第2位表示是否差分定位定位,0——非差分定位；1——差分定位
	bool is_diff;

	//信号强度等级（0-100,100最高）
	u8 signal_intensity_grade;

	//水平方向定位精度[0.5,99.9]
	float hdop;

    //可见卫星数
	u8 satellites_tracked;

	//天线高度（相对于平均海平面,单位米）
    float altitude; 

	//最大信噪比(1分钟更新一次)
	u8 max_snr;

	//信号最好的5颗星
	SNRInfo snr_array[SNR_INFO_NUM];

	//可见卫星数(1秒更新一次)
	u8 satellites_inview;

	//snr大于35的卫星数,即我们认为可用的卫星数(1秒更新一次)
	u8 satellites_good;

	float aclr_avg_calculate_by_speed;
	u32 constant_speed_time;
	u32 static_speed_time;
	StateRecord over_speed_alarm_state;
}Gps,*PGps;

static Gps s_gps;

static void clear_data(void);
static void on_received_rmc(const NMEASentenceRMC rmc);
static void check_fix_state_change(void);
static void calc_alcr_by_speed(GPSData gps_info);
static void check_over_speed_alarm(float speed);
static void upload_gps_data(const GPSData current_gps_data);
static void check_over_speed_alarm(float speed);
static bool is_turn_point(const GPSData current_gps_data);
static void on_received_gsv(const NMEASentenceGSV gsv);


GM_ERRCODE gps_create(void)
{
    if(!gps_is_fixed())
    {
        clear_data();
    	s_gps.sleep_time = 0;
    	circular_queue_create(&s_gps.gps_time_queue, GPS_BUFF_LEN, GM_QUEUE_TYPE_INT);
    	circular_queue_create(&s_gps.gps_lat_queue, GPS_BUFF_LEN, GM_QUEUE_TYPE_FLOAT);
    	circular_queue_create(&s_gps.gps_lng_queue, GPS_BUFF_LEN, GM_QUEUE_TYPE_FLOAT);
    	circular_queue_create(&s_gps.gps_speed_queue, GPS_BUFF_LEN, GM_QUEUE_TYPE_FLOAT);
    	circular_queue_create(&s_gps.gps_course_queue, GPS_BUFF_LEN, GM_QUEUE_TYPE_FLOAT);

        gps_power_on();
    }
	

	return GM_SUCCESS;
}

GM_ERRCODE gps_destroy(void)
{
	circular_queue_destroy(&s_gps.gps_time_queue, GM_QUEUE_TYPE_INT);
	circular_queue_destroy(&s_gps.gps_lat_queue, GM_QUEUE_TYPE_FLOAT);
	circular_queue_destroy(&s_gps.gps_lng_queue, GM_QUEUE_TYPE_FLOAT);
	circular_queue_destroy(&s_gps.gps_speed_queue, GM_QUEUE_TYPE_FLOAT);
	circular_queue_destroy(&s_gps.gps_course_queue, GM_QUEUE_TYPE_FLOAT);
	return GM_SUCCESS;
}

//const char *gps_test = "$GPGSV,4,1,15,01,11,180,21,04,47,226,25,07,18,320,37,08,79,286,23*75\r\n$GPGSV,4,2,15,09,40,274,33,11,47,177,22,23,45,236,26,27,54,028,23*79\r\n$GPGSV,4,3,15,31,07,126,20,03,,,,05,,,,10,,,*4F\r\n$GPGSV,4,4,15,39,,,36,40,,,35,41,,,35*73\r\n$GPRMC,082732.00,A,2235.446376,N,11351.035678,E,0.0,246.4,160220,2.3,W,A*23\r\n";

GM_ERRCODE gps_timer_proc(void)
{
    static u32 pre_time = 0;

    u8 *pdata = NULL;
    u8 *p  = NULL;
    u8 temp[1500] = {0};
    //u8 *temp = NULL;
    u8 sentence[200] = {0};

    if(clock() - pre_time >= 1)
    {
        pre_time = clock();
        
        if(s_gps.state != GM_GPS_OFF)
        {
            //temp = malloc(2048);

            if(temp != NULL)
            {
                memset(temp,0,1500);

                //sprintf((char *)temp,"%s",gps_test);

                ril_exe_command(1000,"OK",temp,1500,"AT+QGPSGNMEA=\"GSV\"\r\n");

                ril_exe_command(1000,"OK",&temp[strlen((const char *)temp)],1500-strlen((const char*)temp),"AT+QGPSGNMEA=\"RMC\"\r\n");

                pdata = (u8 *)strstr((const char*)temp,"$");

                while(pdata != NULL)
                {   
                     memset(sentence,0,200);
                    
                     if(util_memcpy_until_speical(pdata,sentence,0x0D,200) > 0)
                     {
                         //LOG(DEBUG,"%s",sentence);
                         if(is_nmea_log_enable())
                         {
                            LOG(ATSTR,"%s\r\n",sentence);
                         }
                         
                         gps_on_rcv_uart_data((char *)sentence,strlen((const char*)sentence));
                     }
 
                     p = pdata;
 
 
                     pdata = (u8 *)strstr((const char*)p + 1,"$"); 
                }

            }
            else
            {
                LOG(ERR,"GPS Recive BUFF malloc error!");
            }

            //free(temp);
        }
    }
    
	return GM_SUCCESS;
}

static void clear_data(void)
{
	s_gps.gpsdev_type = GM_GPS_TYPE_UNKNOWN;
	s_gps.baud_rate = 115200;
	s_gps.internal_state = GM_GPS_STATE_INITED;
	s_gps.agps_time = 0;
	s_gps.state = GM_GPS_OFF;
	s_gps.state_record.state = false;
	s_gps.state_record.true_state_hold_seconds = 0;
	s_gps.state_record.false_state_hold_seconds = 0;
	s_gps.power_on_time = 0;
	s_gps.mtk_start_time = 0;
    s_gps.last_rcv_time = 0;
	s_gps.push_data_enbale = true;
	s_gps.fix_time = 0;
	s_gps.save_time = 0;
	s_gps.report_time = 0;
	s_gps.distance_since_last_report = 0;
	s_gps.has_opened_td_vtg = false;
	s_gps.ref_lng = 0;
	s_gps.ref_lat = 0;
	s_gps.is_fix = false;
	s_gps.is_3d = false;
	s_gps.is_diff = false;
	s_gps.signal_intensity_grade = 0;
	s_gps.hdop = 99.9;
	s_gps.satellites_tracked = 0;
	s_gps.max_snr = 0;
	memset(s_gps.snr_array, 0, sizeof(s_gps.snr_array));
	s_gps.satellites_inview = 0;
	s_gps.satellites_good = 0;
	s_gps.altitude = 0;
	s_gps.aclr_avg_calculate_by_speed = 0;
	s_gps.constant_speed_time = 0;
	s_gps.static_speed_time = 0;
	s_gps.static_speed_time = 0;
	s_gps.over_speed_alarm_state.state = system_state_get_overspeed_alarm();
	s_gps.over_speed_alarm_state.true_state_hold_seconds = 0;
	s_gps.over_speed_alarm_state.false_state_hold_seconds = 0;
	
	memset(&s_gps.frame_rmc, 0, sizeof(NMEASentenceRMC));
	memset(&s_gps.frame_gsv, 0, sizeof(NMEASentenceGSV));
	
}

GM_ERRCODE gps_power_on(void)
{
    if(s_gps.state == GM_GPS_OFF)
    {
        if(ril_exe_command(600,"OK",NULL,0,"AT+QGPS=1\r\n") == SUCCESS)
        {
            led_service_gps_set(LED_SLOW_FLASH);
            s_gps.state = GM_GPS_FIX_NONE;
        }
    }
    
    
    return GM_SUCCESS;
}
#if 0
static void check_fix_state(void)
{
	//JsonObject* p_log_root = NULL;
	//char snr_str[128] = {0};
	if (s_gps.state < GM_GPS_FIX_3D)
	{
		if (false == system_state_has_reported_lbs_since_boot() && false == system_state_has_reported_gps_since_boot())
		{
			gps_service_push_lbs();
			system_state_set_has_reported_lbs_since_boot(true);
		}
        #if 0
		p_log_root = json_create();
		json_add_string(p_log_root, "event", "unfixed");
		json_add_int(p_log_root, "AGPS time", s_gps.agps_time);
		json_add_int(p_log_root, "satellites_in_view", s_gps.satellites_inview);
		json_add_int(p_log_root, "satellites_good", s_gps.satellites_good);
		json_add_int(p_log_root, "satellites_tracked", s_gps.satellites_tracked);

		GM_snprintf(snr_str,128,"%03d:%02d,%03d:%02d,%03d:%02d,%03d:%02d,%03d:%02d,",
			s_gps.snr_array[0].prn,s_gps.snr_array[0].snr,
			s_gps.snr_array[1].prn,s_gps.snr_array[1].snr,
			s_gps.snr_array[2].prn,s_gps.snr_array[2].snr, 
			s_gps.snr_array[3].prn,s_gps.snr_array[3].snr,
			s_gps.snr_array[4].prn,s_gps.snr_array[4].snr);

		json_add_string(p_log_root, "snr", snr_str);
		json_add_int(p_log_root, "csq", gsm_get_csq());
        
		//泰斗可能会出现AGPS数据无效的情况，导致发送几个数据包以后收不到芯片确认，需要重新下载AGPS数据
		if (GM_GPS_TYPE_TD_AGPS == s_gps.gpsdev_type && 0 == s_gps.agps_time && GM_GPS_STATE_WAIT_APGS_DATA == s_gps.internal_state)
		{
			//s_gps.internal_state = GM_GPS_STATE_WAIT_APGS_DATA;
			//agps_service_require_to_gps(AGPS_TO_GPS_DATA,true);
			json_add_string(p_log_root, "AGPS data", "invalid");
		}
		log_service_upload(INFO,p_log_root);
        #endif
	}
	
}

#endif

GM_ERRCODE gps_power_off(void)
{
    if(s_gps.state != GM_GPS_OFF)
    {
        if(ril_exe_command(1000,"OK",NULL,0,"AT+QGPSEND\r\n") == SUCCESS)
        {
            s_gps.state = GM_GPS_OFF;
            led_service_gps_set(LED_SLOW_FLASH);
        }
    }

    return GM_SUCCESS;
}

GPSState gps_get_state(void)
{
	return s_gps.state;
}

bool gps_is_fixed(void)
{
	if (GM_GPS_FIX_3D > gps_get_state() || GM_GPS_OFF == gps_get_state())
	{
		return false;
	}
	else
	{
		return true;
	}
}

u16 gps_get_fix_time(void)
{
	return s_gps.fix_time;
}

u8 gps_get_max_snr(void)
{
	return s_gps.max_snr;
}

u8 gps_get_satellites_tracked(void)
{
	return s_gps.satellites_tracked;
}

u8 gps_get_satellites_inview(void)
{
	return s_gps.satellites_inview;
}

u8 gps_get_satellites_good(void)
{
	return s_gps.satellites_good;
}
const SNRInfo* gps_get_snr_array(void)
{
	return s_gps.snr_array;
}

bool gps_get_last_data(GPSData* p_data)
{
	return gps_get_last_n_senconds_data(0,p_data);
}

/**
 * Function:   获取最近n秒的GPS数据
 * Description:
 * Input:	   seconds:第几秒,从0开始
 * Output:	   p_data:指向定位数据的指针
 * Return:	   GM_SUCCESS——成功；其它错误码——失败
 * Others:	   
 */
bool gps_get_last_n_senconds_data(u8 seconds,GPSData* p_data)
{
	if (NULL == p_data)
	{
		return false;
	}
	else
	{
		if(!circular_queue_get_by_index_i(&s_gps.gps_time_queue, seconds, (S32 *)&p_data->gps_time))
		{
				return false;
		}
		if(!circular_queue_get_by_index_f(&s_gps.gps_lat_queue, seconds, &p_data->lat))
		{
				return false;
		}
		if(!circular_queue_get_by_index_f(&s_gps.gps_lng_queue, seconds, &p_data->lng))
		{
				return false;
		}
		if(!circular_queue_get_by_index_f(&s_gps.gps_speed_queue, seconds, &p_data->speed))
		{
				return false;
		}
		if(!circular_queue_get_by_index_f(&s_gps.gps_course_queue, seconds, &p_data->course))
		{
				return false;
		}
        p_data->satellites = s_gps.satellites_tracked;
		p_data->precision = s_gps.hdop;
		p_data->signal_intensity_grade = s_gps.signal_intensity_grade;
		return true;
	}
}

#if 0
static void check_has_received_data(void)
{
}
#endif

void gps_on_rcv_uart_data(char* p_data, const U16 len)
{
	//bool has_agps_data_left = false;
	s_gps.last_rcv_time = util_clock();
	
	switch (nmea_sentence_id(p_data,len, false)) 
	{			
        case NMEA_SENTENCE_RMC: 
		{
            if (nmea_parse_rmc(&s_gps.frame_rmc, p_data)) 
			{
                //LOG(DEBUG,"$RMC: raw coordinates,speed and course:vaild %d (%d/%d,%d/%d), %d/%d,%d/%d",
                        //s_gps.frame_rmc.valid,
                        //s_gps.frame_rmc.latitude.value, s_gps.frame_rmc.latitude.scale,
                        //s_gps.frame_rmc.longitude.value, s_gps.frame_rmc.longitude.scale,
                        //s_gps.frame_rmc.speed.value, s_gps.frame_rmc.speed.scale,
                        //s_gps.frame_rmc.course.value, s_gps.frame_rmc.course.scale);
 
				
				on_received_rmc(s_gps.frame_rmc);
            }
            else
			{
                LOG(WARN,"$RMC sentence is not parsed");
            }
        } 
		break;


        
		case NMEA_SENTENCE_GSV: 
		{	
            if (nmea_parse_gsv(&s_gps.frame_gsv, p_data))
			{ 
                on_received_gsv(s_gps.frame_gsv);
            }
            else 
			{
                LOG(WARN,"$XXGSV sentence is not parsed");
            }
        } 
        break;

        default:
		{
			//LOG(DEBUG,"Unknown data:%s",p_data);
            //LOG_HEX(p_data,len);
        } 
			break;
    }
}

static void on_received_fixed_state(const NMEAGSAFixType fix_type)
{
	if (NMEA_GPGSA_FIX_NONE == fix_type)
	{
		s_gps.is_fix = false;
		s_gps.is_3d = false;
		s_gps.is_diff = false;
	}
	else if (NMEA_GPGSA_FIX_2D == fix_type)
	{
		s_gps.is_fix = true;
		s_gps.is_3d = false;
	}
	else if (NMEA_GPGSA_FIX_3D == fix_type)
	{
		s_gps.is_fix = true;
		s_gps.is_3d = true;
	}
	else 
	{
		LOG(WARN, "Unknown fix_type:%d", fix_type);
	}
	check_fix_state_change();
}

static void on_received_gsv(const NMEASentenceGSV gsv)
{
	u8 index_sate = 0;
	u8 index_snr = 0;
	static u8 satellites_good = 0;
	static u8 max_snr = 0;
	//信号最好的5颗星
	static SNRInfo snr_array[5] = {{0,0},{0,0},{0,0},{0,0},{0,0}};
	s_gps.satellites_inview = gsv.total_satellites;


    for (index_sate = 0; index_sate < 4; index_sate++)
    {
    	//找出最差的那颗星的位置
		u8 min_snr_index = 0;
		u8 min_snr = 0xFF;
		
		for(index_snr = 0;index_snr < SNR_INFO_NUM;index_snr++)
		{
			if (snr_array[index_snr].snr < min_snr)
			{
				min_snr = snr_array[index_snr].snr;
				min_snr_index = index_snr;
			}
		}

		//LOG(DEBUG,"min_snr:%d,min_snr_index:%d",min_snr,min_snr_index);
	
		//如果当前这颗星比之前保存的5颗中最差的一个要好，那么替换掉最差的那个
		if (gsv.satellites[index_sate].snr > min_snr)
		{
			snr_array[min_snr_index].prn = gsv.satellites[index_sate].nr;
			snr_array[min_snr_index].snr = gsv.satellites[index_sate].snr;
		}

		if (gsv.satellites[index_sate].snr > 35)
		{
			satellites_good++;
		}
		
		if (gsv.satellites[index_sate].snr > max_snr)
		{
			max_snr = gsv.satellites[index_sate].snr;
		}
    }

	//LOG(DEBUG,"msg_number:%d,total_msgs:%d",gsv.msg_number, gsv.total_msgs);
	//最后一条消息，更新可用卫星数和最大信噪比等成员变量,中间变量清零
	if (gsv.msg_number == gsv.total_msgs)
	{
		s_gps.satellites_good = satellites_good;
		satellites_good = 0;
		
		s_gps.max_snr = max_snr;
		max_snr = 0;
		//LOG(DEBUG,"$GSV: sattelites in view: %d,satellites_good: %d,max_snr %d", gsv.total_satellites,s_gps.satellites_good,s_gps.max_snr);
			
		//上个版本用的水平精度因子（和卫星分布有关）
		s_gps.signal_intensity_grade = s_gps.max_snr/10;
		if (s_gps.signal_intensity_grade > 4)
		{
			s_gps.signal_intensity_grade = 4;
		}
		memcpy(s_gps.snr_array, snr_array, sizeof(snr_array));
		memset(snr_array, 0, sizeof(snr_array));
	}
}

static void check_fix_state_change(void)
{
	GM_CHANGE_ENUM change = util_check_state_change(s_gps.is_3d,&s_gps.state_record,3,10);
	
	if (GM_CHANGE_TRUE == change)
	{
		JsonObject* p_log_root = json_create();
		char snr_str[128] = {0};
		
		s_gps.internal_state = GM_GPS_STATE_WORKING;

		if(0 == s_gps.fix_time)
		{
			s_gps.fix_time = util_clock() - s_gps.power_on_time;
            #if 1
			json_add_string(p_log_root, "event", "fixed");
		    //json_add_int(p_log_root, "AGPS time", s_gps.agps_time);
			//距离上次休眠到现在超过6小时或者没有休眠过是冷启动，否则是热启动
			if((util_clock() - s_gps.sleep_time) > 6*SECONDS_PER_HOUR || 0 == s_gps.sleep_time)
			{
				json_add_int(p_log_root, "cold fix_time", s_gps.fix_time);
				LOG(INFO,"cold fix_time:%d",s_gps.fix_time);
			}
			else
			{
				json_add_int(p_log_root, "hot fix_time", s_gps.fix_time);
				LOG(INFO,"hot fix_time:%d",s_gps.fix_time);
			}
            #endif
		}
        #if 1
		else
		{
			json_add_string(p_log_root, "event", "become to fixed");
		}
				
		json_add_int(p_log_root, "satellites_in_view", s_gps.satellites_inview);
		json_add_int(p_log_root, "satellites_good", s_gps.satellites_good);
		json_add_int(p_log_root, "satellites_tracked", s_gps.satellites_tracked);
		
		    snprintf(snr_str,128,"%03d:%02d,%03d:%02d,%03d:%02d,%03d:%02d,%03d:%02d,",
			s_gps.snr_array[0].prn,s_gps.snr_array[0].snr,
			s_gps.snr_array[1].prn,s_gps.snr_array[1].snr,
			s_gps.snr_array[2].prn,s_gps.snr_array[2].snr, 
			s_gps.snr_array[3].prn,s_gps.snr_array[3].snr,
			s_gps.snr_array[4].prn,s_gps.snr_array[4].snr);

		json_add_string(p_log_root, "snr", snr_str);
		json_add_int(p_log_root, "csq", net_get_csq());
		log_service_upload(INFO, p_log_root);

		
		
		//led_set_gps_state(GM_LED_ON);
        #endif
        led_service_gps_set(LED_ON);
		s_gps.state = (GPSState)(s_gps.is_fix << 2 | (s_gps.is_3d << 1) | (s_gps.is_diff));
	}
	else if(GM_CHANGE_FALSE == change)
	{
        #if 1
		JsonObject* p_log_root = json_create();
		char snr_str[128] = {0};
		json_add_string(p_log_root, "event", "become to unfixed");
		json_add_int(p_log_root, "satellites_in_view", s_gps.satellites_inview);
		json_add_int(p_log_root, "satellites_good", s_gps.satellites_good);
		json_add_int(p_log_root, "satellites_tracked", s_gps.satellites_tracked);
		
	    snprintf(snr_str,128,"%03d:%02d,%03d:%02d,%03d:%02d,%03d:%02d,%03d:%02d,",
			s_gps.snr_array[0].prn,s_gps.snr_array[0].snr,
			s_gps.snr_array[1].prn,s_gps.snr_array[1].snr,
			s_gps.snr_array[2].prn,s_gps.snr_array[2].snr, 
			s_gps.snr_array[3].prn,s_gps.snr_array[3].snr,
			s_gps.snr_array[4].prn,s_gps.snr_array[4].snr);

		json_add_string(p_log_root, "snr", snr_str);
		json_add_int(p_log_root, "csq", net_get_csq());
		log_service_upload(INFO, p_log_root);
		
        #endif
        led_service_gps_set(LED_SLOW_FLASH);
        agps_service_rewite_data();
		s_gps.state = (GPSState)(s_gps.is_fix << 2 | (s_gps.is_3d << 1) | (s_gps.is_diff));
	}
	else
	{
		
	}
	
}
static void on_received_rmc(const NMEASentenceRMC rmc)
{
	GPSData gps_data = {0};
	u8 index = 0;
	float lat_avg = 0;
	float lng_avg = 0;
	float speed_avg = 0;
	time_t seconds_from_reboot = util_clock();


	on_received_fixed_state(rmc.valid?NMEA_GPGSA_FIX_3D:NMEA_GPGSA_FIX_NONE);


	if (s_gps.state != GM_GPS_FIX_3D)
	{
		return;
	}

	if (seconds_from_reboot - s_gps.save_time >= 1)
	{
		u8 filter_mode = 0;
		gps_data.gps_time = nmea_get_utc_time(&rmc.date,&rmc.time);
		gps_data.lat = nmea_tocoord(&rmc.latitude);
		gps_data.lng = nmea_tocoord(&rmc.longitude);
		gps_data.speed = util_mile_to_km(nmea_tofloat(&rmc.speed));
		//判断角度是否有效,如果无效取上一个点的角度
		if (0 == rmc.course.value && 0 == rmc.course.scale)
		{	
			GPSData last_gps_data = {0};
			if(gps_get_last_data(&last_gps_data))
			{
				gps_data.course = last_gps_data.course;
			}
			else
			{
				gps_data.course = 0;
			}
		}
		else
		{
			gps_data.course = nmea_tofloat(&rmc.course);
		}
		gps_data.satellites = s_gps.satellites_tracked;
		gps_data.precision = s_gps.hdop;
		gps_data.signal_intensity_grade = s_gps.signal_intensity_grade;

		//检查异常数据
		if (0 == gps_data.gps_time || false == rmc.valid)
		{
            LOG(INFO,"get invalid gps time");
			return;
		}
		
		if (fabs(gps_data.lat) < 0.1 &&  fabs(gps_data.lng)  < 0.1)
		{

            LOG(INFO,"get invalid lat %f lng %f",fabs(gps_data.lat),fabs(gps_data.lng));
			return;
		}
		
		calc_alcr_by_speed(gps_data);
		check_over_speed_alarm(gps_data.speed);

		
		s_gps.save_time = seconds_from_reboot;
		
		circular_queue_en_queue_i(&s_gps.gps_time_queue,gps_data.gps_time);
		circular_queue_en_queue_f(&s_gps.gps_lat_queue,gps_data.lat);
		circular_queue_en_queue_f(&s_gps.gps_lng_queue, gps_data.lng);
		circular_queue_en_queue_f(&s_gps.gps_speed_queue,gps_data.speed);
		circular_queue_en_queue_f(&s_gps.gps_course_queue,gps_data.course);
		
		//config_service_get(CFG_SMOOTH_TRACK, TYPE_BYTE, &filter_mode, sizeof(filter_mode));
		filter_mode = 1;
		if(filter_mode == 1)
		{
			//LOG(DEBUG,"Avage filter.");
			//取前10秒（包括当前时间点）平均值
			for(index = 0;index < 10;index++)
			{
				time_t last_n_time = 0;
				float last_n_lat = 0;
				float last_n_lng = 0;
				float last_n_speed = 0;
			
				if(false == circular_queue_get_by_index_i(&s_gps.gps_time_queue,index,(S32*)&last_n_time))
				{
					break;
				}
				if (gps_data.gps_time - last_n_time >= 10)
				{
					break;
				}
				circular_queue_get_by_index_f(&s_gps.gps_lat_queue,index,&last_n_lat);
				circular_queue_get_by_index_f(&s_gps.gps_lng_queue, index,&last_n_lng);
				circular_queue_get_by_index_f(&s_gps.gps_speed_queue,index,&last_n_speed);
			
				lat_avg = (lat_avg*index + last_n_lat)/(index + 1);
				lng_avg = (lng_avg*index + last_n_lng)/(index + 1);
				speed_avg = (speed_avg*index + last_n_speed)/(index + 1);
			}
			gps_data.lat = lat_avg;
			gps_data.lng = lng_avg;
			gps_data.speed = speed_avg;
		}
		else if(filter_mode == 2)
		{
            #if 0
			double lat_filted = 0;
			double lng_filted = 0;
			if(gps_kalman_filter_update(gps_data.lat, gps_data.lng,1.0))
			{
				gps_kalman_filter_read(&lat_filted,&lng_filted);
				LOG(DEBUG,"Kalman filter:lat=%f,lng=%f,lat_filted=%f,lng_filted=%f",gps_data.lat,gps_data.lng,lat_filted,lng_filted);
				gps_data.lat = lat_filted;
				gps_data.lng = lng_filted;
			}
            #endif
		}
		else
		{
			LOG(DEBUG,"filter_mode:%d",filter_mode);
		}
		upload_gps_data(gps_data);
	}
}

//每1秒调用一次
static void calc_alcr_by_speed(GPSData gps_info)
{
    float last_second_speed = 0;
    float aclr_calculate_by_speed = 0;

    if (gps_info.speed >= 20)
    {
        s_gps.constant_speed_time++;
        s_gps.static_speed_time = 0;
        if (circular_queue_get_tail_f(&(s_gps.gps_speed_queue), &last_second_speed))
        {
            aclr_calculate_by_speed = (gps_info.speed - last_second_speed) * 1000.0 / SECONDS_PER_HOUR;
            s_gps.aclr_avg_calculate_by_speed = (s_gps.aclr_avg_calculate_by_speed * (s_gps.constant_speed_time - 1) + aclr_calculate_by_speed) / (s_gps.constant_speed_time);
        }
    }
    else
    {
        s_gps.constant_speed_time = 0;
        s_gps.static_speed_time++;
        s_gps.aclr_avg_calculate_by_speed = 0;
    }

}

static void check_over_speed_alarm(float speed)
{
	bool speed_alarm_enable = false;
	u8 speed_threshold = 0;
	u8 speed_check_time = 0;
	GM_CHANGE_ENUM state_change = GM_NO_CHANGE;
	AlarmInfo alarm_info;
	alarm_info.type = ALARM_SPEED;
	alarm_info.info = speed;
	
	config_service_get(CFG_SPEED_ALARM_ENABLE, TYPE_BYTE, &speed_alarm_enable, sizeof(speed_alarm_enable));
	config_service_get(CFG_SPEEDTHR, TYPE_BYTE, &speed_threshold, sizeof(speed_threshold));
	config_service_get(CFG_SPEED_CHECK_TIME, TYPE_BYTE, &speed_check_time, sizeof(speed_check_time));
	
	if(!speed_alarm_enable)
	{
		return;
	}
	
	state_change = util_check_state_change((bool)(speed > speed_threshold), &s_gps.over_speed_alarm_state, speed_check_time, speed_check_time);
	if(GM_CHANGE_TRUE == state_change)
	{
		system_state_set_overspeed_alarm(true);
		gps_service_push_alarm(&alarm_info);
	}
	else if(GM_CHANGE_FALSE == state_change)
	{
		system_state_set_overspeed_alarm(false);
	}
	else
	{
		
	}
}

time_t gps_get_constant_speed_time(void)
{
	return s_gps.constant_speed_time;
}

float gps_get_aclr(void)
{
	return s_gps.aclr_avg_calculate_by_speed;
}

static GpsDataModeEnum upload_mode(const GPSData current_gps_data)
{
	GM_ERRCODE err_code = GM_SUCCESS;
	u32 seconds_from_reboot = util_clock();
    u16 upload_time_interval = 0;
	bool static_upload_enable = false;
	
	err_code = config_service_get(CFG_UPLOADTIME, TYPE_SHORT, &upload_time_interval, sizeof(upload_time_interval));
	if (GM_SUCCESS != err_code)
	{
		LOG(ERR, "Failed to config_service_get(CFG_UPLOADTIME),err_code=%d", err_code);
		return GPS_MODE_NONE;
	}

	if(false == system_state_has_reported_gps_since_boot())
	{
		system_state_set_has_reported_gps_since_boot(true);
		s_gps.distance_since_last_report = 0;
		s_gps.report_time = seconds_from_reboot;
		return GPS_MODE_POWER_UP;
	}
	
	if(false == system_state_has_reported_gps_since_modify_ip())
	{
		system_state_set_reported_gps_since_modify_ip(true);
		s_gps.distance_since_last_report = 0;
		s_gps.report_time = seconds_from_reboot;
		return GPS_MODE_POWER_UP;
	}
	
    config_service_get(CFG_IS_STATIC_UPLOAD, TYPE_BYTE, &static_upload_enable, sizeof(static_upload_enable));
	if (VEHICLE_STATE_STATIC == system_state_get_vehicle_state()  && false == static_upload_enable && false == config_service_is_test_mode())
	{
		LOG(DEBUG, "Need not upload data because of static.");
		return GPS_MODE_NONE;
	}
	else 
	{
		if ((seconds_from_reboot - s_gps.report_time) >= upload_time_interval)
		{
			s_gps.distance_since_last_report = 0;
			s_gps.report_time = seconds_from_reboot;
			if(VEHICLE_STATE_STATIC == system_state_get_vehicle_state())
			{
				return GPS_MODE_STATIC_POSITION;
			}
			else
			{
                LOG(INFO,"Upload GPS_MODE_FIX_TIME!");
				return GPS_MODE_FIX_TIME;
			}
			
		}
		else if (is_turn_point(current_gps_data)) 
		{
			s_gps.distance_since_last_report = 0;
			s_gps.report_time = seconds_from_reboot;
			LOG(DEBUG, "GPS_MODE_TURN_POINT");
			return GPS_MODE_TURN_POINT;
		}
		else
		{
			return GPS_MODE_NONE;
		}
	}
}

static void upload_gps_data(const GPSData current_gps_data)
{
	GM_ERRCODE ret = GM_SUCCESS;
	GpsDataModeEnum mode = upload_mode(current_gps_data);
	//ST_Time st_time = {0};
	//struct tm tm_time = {0};

	static GPSData last_report_gps = {0};

	if(false == s_gps.push_data_enbale)
	{
		LOG(WARN,"do not push data!");
		return;
	}
	
	if (0 == current_gps_data.gps_time)
	{
		LOG(WARN,"Invalid GPS data!");
		return;
	}

	if (GPS_MODE_NONE == mode)
	{
		return;
	}

	if (GPS_MODE_POWER_UP == mode)
	{	
		//tm_time = util_gmtime(current_gps_data.gps_time);
		//util_tm_to_mtktime(&tm_time,&st_time);
		//GM_SetLocalTime(&st_time);
		//LOG(INFO,"Set local time:(%d-%02d-%02d %02d:%02d:%02d)).",st_time.year,st_time.month,st_time.day,st_time.hour,st_time.minute,st_time.second);
	}

	ret = gps_service_push_gps(mode,&current_gps_data);

	if (last_report_gps.gps_time != 0)
	{
		s_gps.distance_since_last_report = applied_math_get_distance(current_gps_data.lng, current_gps_data.lat, last_report_gps.lng, last_report_gps.lat);
		system_state_set_mileage(system_state_get_mileage() + s_gps.distance_since_last_report);
	}

	last_report_gps = current_gps_data;
	
	
	if (GM_SUCCESS != ret)
	{
		LOG(ERR, "Failed to gps_service_push_gps,ret=%d", ret);
		return;
	}
}

static bool is_turn_point(const GPSData current_gps_data)
{
	u16 course_change_threshhold = 0;
    float course_change = 0;
	float current_course = current_gps_data.course;
	float last_second_course = 0;
	float last_speed = 0;
	GM_ERRCODE ret = GM_SUCCESS;
	u8 index = 0;

	ret = config_service_get(CFG_TURN_ANGLE, TYPE_SHORT, &course_change_threshhold, sizeof(course_change_threshhold));
	if(GM_SUCCESS != ret)
	{
		LOG(ERR, "Failed to config_service_get(CFG_TURN_ANGLE),ret=%d", ret);
		return false;
	}
	
	if(!circular_queue_get_by_index_f(&s_gps.gps_course_queue, 1, &last_second_course))
	{
		return false;
	}

	course_change = applied_math_get_angle_diff(current_course,last_second_course);	
	//LOG(DEBUG,"[%s]:current course:%f,last second course:%f,course change:%f",__FUNCTION__,current_course,last_second_course,course_change);
	
    if (current_gps_data.speed >= 80.0f)
    {
        if (course_change >= 6)
        {   
            return true;
        }
        else if (course_change >= course_change_threshhold)
        {
            return true;
        }
		else
		{
			return false;
		}
    }
    else if (current_gps_data.speed >= 40.0f)
    {
        if (course_change >= 7)
        {
            return true;
        }
        else if (course_change >= course_change_threshhold)
        {
            return true;
        }
		else
		{
			return false;
		}
    }
    else if (current_gps_data.speed >= 25.0f)
    {
        if (course_change >= 8)
        {
            return true;
        }
        else if (course_change >= course_change_threshhold)
        {
            return true;
        }
		else
		{
			return false;
		}
    }
    else if (current_gps_data.speed >= 15.0f)
    {
        if (course_change >= (course_change_threshhold - 6))
        {
            return true;
        }
		else
		{
			return false;
		}
    }
    else if ((current_gps_data.speed >= 5.0f) && course_change >= 16 && s_gps.distance_since_last_report > 20.0f)
    {
        // 连续5秒速度大于5KMH,角度大于20度,里程超过20米,上传拐点
        for (index =0; index < 5; index++)
        {
			if(!circular_queue_get_by_index_f(&s_gps.gps_speed_queue, index, &last_speed))
			{
				return false;
			}
            if (last_speed <= 5.0f)
            {
                return false;
            }
        }
		return true;
    }
	else
	{
		return false;
	}
}

