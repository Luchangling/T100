
#ifndef __BMS_H__
#define __BMS_H__

#include "sys.h"
#include "usart.h"
#include "timer.h"

#define MAX_BMS_QUE_COUNTS 10

#define PER_BMS_PACKET_LEN 20


#define BMS_PROTOCOL_VER 1

#if BMS_PROTOCOL_VER == 1
#define MAX_BMS_SINGLE_BAT_COUNTS 8
typedef enum
{
    BMS_PF_CODE_INFO,
    BMS_PF_DEV_INFO,
    BMS_PF_SINGLE_VOL_2,
    BMS_PF_SINGLE_VOL_3,
    BMS_PF_SINGLE_VOL_4,
    BMS_PF_SINGLE_VOL_5,
    BMS_PF_SINGLE_VOL_6,
    BMS_PF_SINGLE_VOL_7,
    BMS_PF_SINGLE_VOL_8,
    BMS_PF_SINGLE_VOL_9,
    BMS_PF_VOL_STATIS_INFO,
    BMS_PF_TEMPERATURE_INFO,
    BMS_PF_MAX_CURRENT_TEMP_INFO,
    BMS_PF_BLANCE_HW_CURRENT_INFO,
    BMS_PF_FAULT_CODE_1,
    BMS_PF_FAULT_CODE_2,
    BMS_PF_ENERGY_INFO,
    BMS_PF_CAP_INFO,
    BMS_PF_USE_INFO,
    BMS_PF_READ_MAX, //19
}BMS_Read_PF_Enum;
#else
#define MAX_BMS_SINGLE_BAT_COUNTS 5
typedef enum
{
    BMS_PF_CODE_INFO,
    BMS_PF_DEV_INFO,
    BMS_PF_SINGLE_VOL_2,
    BMS_PF_SINGLE_VOL_3,
    BMS_PF_SINGLE_VOL_4,
    BMS_PF_SINGLE_VOL_5,
    BMS_PF_SINGLE_VOL_6,
    BMS_PF_VOL_STATIS_INFO,
    BMS_PF_TEMPERATURE_INFO,
    BMS_PF_MAX_CURRENT_TEMP_INFO,
    BMS_PF_BLANCE_HW_CURRENT_INFO,
    BMS_PF_FAULT_CODE_1,
    BMS_PF_FAULT_CODE_2,
    BMS_PF_ENERGY_INFO,
    BMS_PF_CAP_INFO,
    BMS_PF_USE_INFO,
    BMS_PF_READ_MAX, //16
}BMS_Read_PF_Enum;
#endif


typedef struct
{
   u16 bat1_vol;
   u16 bat2_vol;
   u16 bat3_vol;
   u16 bat4_vol;
}SingleBatVoltageInfo;

#pragma anon_unions  
typedef struct
{
    //编码信息
    u8 code_info[4];
    u8 factory_date[3];
    u8 reserv0;
    //设备信息
    u16 bat_pack_count;
    u8  reserv1;
    u8  bat_factory;
    u8  sw_ver;
    u8  hw_ver;
    u8  protocol_ver;
    u8  reserv2;
    //单体电压信息
    SingleBatVoltageInfo bat[MAX_BMS_SINGLE_BAT_COUNTS];
    //电压统计信息
    u16 max_vol;
    u16 min_vol;
    u16 averge_vol;
    u16 total_vol;
    //温度统计信息
    u8 single_tmp1;
    u8 single_tmp2;
    u8 single_tmp3;
    u8 single_tmp4;
    u8 max_tmp;
    u8 min_tmp;
    u8 averge_tmp;
    u8 div_tmp;
    //电流及极值位置信息
    u16 current;
    u16 reserv3;
    u8  max_tmp_pos;
    u8  min_tmp_pos;
    u8  max_vol_pos;
    u8  min_vol_pos;
    //均衡,硬件及电流状态
    u8  balence_op_1;
    u8  balence_op_2;
    u8  balence_op_3;
    union
    {
        u8  input_state;
        struct
        {
            u8 in1 : 1;
            u8 in2 : 1;
            u8 in3 : 1;
            u8 in4 : 1;
            u8 reserv4 : 4;
        };
    };
    union
    {
        u8  mos_state;
        struct
        {
            u8 in_mos : 1;
            u8 out_mos: 1;
            u8 reserv5: 6;
        };
    };
    u8  current_sta;
    union
    {
        u8  switch_sta;
        struct
        {
            u8 switch1 : 1;
            u8 switch2 : 1;
            u8 switch3 : 1;
            u8 switch4 : 1;
            u8 reserv6 : 4; 
        };
    };  
    u8  balence_life;
    //故障信息1
    u8  vol_check_map[3];
    u8  tmp_check_map;
    union
    {
        u8  conn_err;
        struct
        {
            u8 afe_h : 1;
            u8 afe_l : 1;
            u8 reserv7:5;
            u8 conn  : 1;
        };
    };
    u8 reserv10[2];
    u8 fault1_life;
    //故障信息2
    union
    {
        u8 warn_protect_sta1;
        struct
        {
            u8 t_vol_hig : 1;
            u8 t_vol_low : 1;
            u8 s_vol_hig : 1;
            u8 s_vol_low : 1;
            u8 o_tmp_hig : 1;
            u8 c_tmp_hig : 1;
            u8 o_tmp_low : 1;
            u8 c_tmp_low : 1;
        };
    };
    union
    {
       u8 warn_protect_sta2;
       struct
       {
            u8 c_short_pt : 1;
            u8 o_short_pt : 1;
            u8 c_current_hig : 1;
            u8 o_current_hig : 1;
            u8 div_tmp_hig   : 1;
            u8 single_vol_diff:1;
            u8 SOC_sta : 1;
            u8 SOH_sta : 1;
       };
    };
    u8 reserv8[2];
    u8 reserv9[2];
    u8  reserv12;
    u8  fault2_life;
    //容量信息
    u16 reming_energy;
    u16 total_energy;
    u8  reserv11[2];
    u8  SOC;
    u8  SOH;
    //累计电容信息
    u32 total_o_energy;
    u32 total_c_energy;
    //累计充放电次数
    u32 total_o_counts;
    u32 total_c_counts;
    //BMS电压阈值
    u16 total_ov_v; //ov 过压
    u16 total_lm_v; //lm 欠压
    u16 single_ov_v;
    u16 single_lm_v;
    //BMS 电压恢复数据
    u16 total_ov_rcy_v; //rcy recovery总电压恢复
    u16 total_lm_rcy_v;
    u16 single_ov_rcy_v;
    u16 single_lm_rcy_v;
    //BMS温度阈值与恢复数据
    u8 charge_temp_high_threshold;
    u8 charge_temp_low_threshold;
    u8 discharge_temp_high_threshold;
    u8 discharge_temp_low_threshold;
    u8 charge_temp_high_rcy_threshold;
    u8 charge_temp_low_rcy_threshold;
    u8 discharge_temp_high_rcy_threshold;
    u8 discharge_temp_low_rcy_threshold;
    //BMS电流过流阈值和恢复数据
    u16 charge_over_cur_threshold;
    u16 discharge_over_cur_threshold;
    u16 charge_over_cur_rcy_threshold;
    u16 discharge_over_cur_rcy_threshold;
    //BMS电压均衡开启电压、开启压差和关闭压差数据
    u16 balance_open_vol;
    u16 balance_open_vol_diff;
    u16 balance_close_vol_diff;
    u16 reserv13;
    //BMS保护延时值数据
    u8 overvoltage_protect_delay;
    u8 undervoltage_protect_delay;
    u8 overcurrent_protect_delay;
    u8 single_high_temp_protect_delay;
    u8 single_low_temp_protect_delay;
    u8 MOS_high_temp_protect_delay;
    u8 MOS_high_temp_threshold;
    u8 MOS_high_temp_rcy_threshold;
    //BMS休眠状态上传
    u8 sleep_enble;
    u8 sleep_reming_time;
    u8 data5[6];
    
}BmsBatteryInfoStruct;

typedef struct
{
    //控制BMS继电器
    u8 relay1;
    u8 relay2;
    u8 relay3;
    u8 relay4;
    u8 charge_MOS;
    u8 discharge_MOS;
    u8 reserv1;
    u8 life;

    //控制电压阈值写入
    u16 total_ov_v; //ov 过压
    u16 total_lm_v; //lm 欠压
    u16 single_ov_v;
    u16 single_lm_v;

    //控制BMS电压恢复值写入
    u16 total_ov_rcy_v; //rcy recovery总电压恢复
    u16 total_lm_rcy_v;
    u16 single_ov_rcy_v;
    u16 single_lm_rcy_v;

    //控制BMS温度阈值与恢复值写入
    u8 charge_temp_high_threshold;
    u8 charge_temp_low_threshold;
    u8 discharge_temp_high_threshold;
    u8 discharge_temp_low_threshold;
    u8 charge_temp_high_rcy_threshold;
    u8 charge_temp_low_rcy_threshold;
    u8 discharge_temp_high_rcy_threshold;
    u8 discharge_temp_low_rcy_threshold;

     //控制BMS电流过流阈值和恢复值写入
    u16 charge_over_cur_threshold;
    u16 discharge_over_cur_threshold;
    u16 charge_over_cur_rcy_threshold;
    u16 discharge_over_cur_rcy_threshold;
    
    //控制BMS电压均衡开启电压、开启压差和关闭压差
    u16 balance_open_vol;
    u16 balance_open_vol_diff;
    u16 balance_close_vol_diff;
    u16 reserv13;
    
    //控制BMS保护延时值写入
    u8 overvoltage_protect_delay;
    u8 undervoltage_protect_delay;
    u8 overcurrent_protect_delay;
    u8 single_high_temp_protect_delay;
    u8 single_low_temp_protect_delay;
    u8 MOS_high_temp_protect_delay;
    u8 MOS_high_temp_threshold;
    u8 MOS_high_temp_rcy_threshold;
    
}BmsBatteryInfoSetStruct;

#pragma anon_unions  
typedef struct
{
    u8 head[3];
    
    union
    {
        u8 frame[12];
        struct
        {
            u8 type;
            u8 pf;
            u8 da;
            u8 sa;
            u8 data[8];
        };
    };
    
    u8 crc[2];
    u8 tail[3];
}BMSPacketStruct;

typedef struct
{
    u8 *buff;
    u16 len;
}BmsDataFrameStruct;

typedef struct
{
    u8 in;
    u8 out;
    BMSPacketStruct *bms[MAX_BMS_QUE_COUNTS];
}BmsDataQue;



typedef enum
{
    BMS_PF_SWITCH_CONTROL,
    BMS_PF_VOL_THRESHOLD_CONTROL,
    BMS_PF_VOL_RECOVERY_CONTROL,
    BMS_PF_TEMP_RECOVERY_CONTROL,
    BMS_PF_CURRENT_RECOVERY_CONTROL,
    BMS_PF_DELAY_CONTROL,
}BMS_Control_PF_Enum;


typedef enum
{
    BMS_SERV_INIT,
    //BMS_SERV_CONNECT,
    BMS_SERV_CONECT_OK,
    BMS_SERV_CLOSE,
    BMS_SERV_DESTORY
}BmsServFlowState;

typedef enum
{
    FRAME_CONTROL = 0x0C,

    FRAME_DATA    = 0x18,
    
}BMSFrameTypeEnum;
 
typedef struct
{
    u8 state;
    u8 fail_count;
    u32 start_time;
    
}BMS_Serv_Control_Struct;


typedef enum
{
    BMS_RELAY_ON,

    BMS_POWER_OFF,
    
}BmsRelayStateEnum;


extern BmsBatteryInfoStruct g_bms;


void bms_service_data_que_init(void);

void bms_service_data_que_delete_one(BMSPacketStruct *bms);

void bms_service_data_que_insert_one(BMSPacketStruct *bms);

BMSPacketStruct *bms_service_get_one_data(void);

void bms_service_timer_proc(void);

void bms_service_create(void);

extern void USART2_IRQHandler(void);

bool bms_service_state(void);

BmsBatteryInfoStruct *bms_service_get_entry_addr(void);

void bms_service_packet_control_info(BmsBatteryInfoSetStruct *ctrl,u16 index ,u16 value);

void bms_service_cut_power(void);

void bms_service_restart_power(void);

void bms_service_packet_control_info_insert(u8 index,BmsBatteryInfoSetStruct *set);


#endif

