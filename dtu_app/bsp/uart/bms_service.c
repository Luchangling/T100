
/*BMS 相关*/
#include "sys.h"
#include "bms_service.h"
#include "bms_service_bsp.h"
#include "fifo.h"
#include "utility.h"
#include "json.h"
#include "log_service.h"
#include "config_service.h"

#define BMS_CONNEECT_CMD "CONNECTED\r\n"

u8 bms_head[3] = {0xFF, 0xAA, 0xBB};
u8 bms_tail[3] = {0x55, 0x99, 0x00};
void bms_service_data_que_delete_one(BMSPacketStruct *bms);
void bms_service_control_process(void);
//BMSPacketStruct *bms_service_get_one_data(void);
void bms_service_data_que_insert_one(BMSPacketStruct *bms);

BmsDataQue s_bms_que;

BMS_Serv_Control_Struct s_bms_ctl;

BmsBatteryInfoStruct g_bms;


void trans_bms_service_status(BmsServFlowState state)
{
    s_bms_ctl.state = state;

    s_bms_ctl.start_time = clock();

    cl_log(INFO,"BMS SERV TRANS STATE %d",state);
    
}

bool is_bms_connected(void)
{
    fifo_TypeDef *fifo = bms_channel_fifo_addr();

    u8 data[3] = {0};

    if(fifo_GetLen(fifo) >= sizeof(bms_head))
    {
        fifo_peek(fifo,data,sizeof(bms_head));

        if(!util_arry_compare(bms_head,data,sizeof(bms_head)))
        {
            //cl_log_hex(data,3,"BMS FIFO :");
            fifo_pop_len(fifo, 1);
        }
        else
        {
            return true;
        }
    }

    return false;
}

void bms_service_create(void)
{
    memset(&g_bms,0,sizeof(g_bms));
    cl_log(INFO,"bms struct size %d",sizeof(g_bms));
    memset(&s_bms_que,0,sizeof(BmsDataQue));
    memset(&s_bms_ctl,0,sizeof(s_bms_ctl));
    s_bms_ctl.start_time = clock();
    //bms_uart_init(115200);
}

void bms_serivce_power_control(void)
{
    u8 state = 0;

    config_service_get(CFG_RELAY_STATE,TYPE_BYTE,&state,1);

    if(state == BMS_RELAY_ON)
    {
        bms_service_restart_power();
    }
    else
    {
        bms_service_cut_power();
    }
}

void bms_service_init(void)
{
    if(s_bms_ctl.fail_count < 5 && clock() - s_bms_ctl.start_time > 1)
    {
        s_bms_ctl.fail_count++;
        cl_log(INFO , "BMS SERVICE  CONNECT .................");
        bms_com_write((const u8 *)BMS_CONNEECT_CMD, strlen(BMS_CONNEECT_CMD));
        s_bms_ctl.start_time = clock();
    }
    else if(s_bms_ctl.fail_count >= 5)
    {
        
        //trans_bms_service_status(BMS_SERV_CONECT_OK);
        JsonObject* p_log_root = json_create();
		
		json_add_string(p_log_root, "event", "BMS connect fail");
		
		log_service_upload(INFO,p_log_root);

        trans_bms_service_status(BMS_SERV_DESTORY);
        
        return;
    }

    if(is_bms_connected())
    {
        JsonObject* p_log_root = json_create();
		
		json_add_string(p_log_root, "event", "BMS connect OK");
		
		log_service_upload(INFO,p_log_root);

        bms_service_data_que_init();
        
        trans_bms_service_status(BMS_SERV_CONECT_OK);

        bms_serivce_power_control();
    }
}
#if 0
u16 GetCRC16(u8 *ptr,  u16 len)
{
    u32 index;
    u8 crch = 0xFF;  //高CRC字节
    u8 crcl = 0xFF;  //低CRC字节
    u8 TabH[] = {  //CRC高位字节值表
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,  
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,  
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,  
        0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,  
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,  
        0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,  
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,  
        0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,  
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,  
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,  
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,  
        0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,  
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,  
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,  
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,  
        0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,  
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,  
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,  
        0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,  
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,  
        0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,  
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,  
        0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,  
        0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,  
        0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,  
        0x80, 0x41, 0x00, 0xC1, 0x81, 0x40  
    } ;  
    u8 TabL[] = {  //CRC低位字节值表
        0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,  
        0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,  
        0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,  
        0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,  
        0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,  
        0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,  
        0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,  
        0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,  
        0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,  
        0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,  
        0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,  
        0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,  
        0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,  
        0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,  
        0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,  
        0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,  
        0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,  
        0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,  
        0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,  
        0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,  
        0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,  
        0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,  
        0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,  
        0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,  
        0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,  
        0x43, 0x83, 0x41, 0x81, 0x80, 0x40  
    } ;
    while (len--)  //计算指定长度的CRC
    {
        index = crch ^ *ptr++;
        crch = crcl ^ TabH[ index];
        crcl = TabL[ index];
    }
   
    return ((crch<<8) | crcl);  
}  
#else
u16 ModBusCRC16(u8 *data, int len)
{
    u16 i, j, tmp, CRC16;

    CRC16 = 0xFFFF;             //CRC寄存器初始值
    for (i = 0; i < len; i++)
    {
        CRC16 ^= data[i];
        for (j = 0; j < 8; j++)
        {
            tmp = (u16)(CRC16 & 0x0001);
            
            CRC16 >>= 1;
            if (tmp == 1)
            {
                CRC16 ^= 0xA001;    //异或多项式
            }
        }
    }

    return CRC16;

}

#endif
void bms_service_connect_ok_proc(void)
{
    fifo_TypeDef *fifo = bms_channel_fifo_addr();

    u16 *crc = NULL;

    u16 calc_crc = 0;

    u8 *temp = NULL;

    BMSPacketStruct s_pa;
    
    if(fifo_GetLen(fifo) >= PER_BMS_PACKET_LEN)
    {
        memset(&s_pa,0,sizeof(BMSPacketStruct));
        #if 1
        fifo_peek(fifo,(u8 *)&s_pa,PER_BMS_PACKET_LEN);

        if(util_arry_compare(s_pa.head,bms_head,sizeof(bms_head)) &&\
           util_arry_compare(s_pa.tail,bms_tail,sizeof(bms_tail)))
        {
            //pack_crc = MKWORD(s_pa.crc[0], );
            crc = (u16 *)s_pa.crc;

            //cl_log_hex(s_pa.frame,sizeof(s_pa.frame),"BMS Frame->");

            calc_crc = ModBusCRC16(s_pa.frame,sizeof(s_pa.frame));

            cl_log(INFO,"BMS service packet calc crc %x , packet crc %x",calc_crc , (*crc));
            
            if(calc_crc == *crc)
            {
                cl_log(INFO,"BMS service Get BMS FRAME type(%x) pf(%x)",s_pa.type,s_pa.pf);
                
                if(s_pa.type == FRAME_DATA)
                {
                    temp = (u8 *)&g_bms;

                    cl_log(INFO,"g_bms addr %x",(temp + s_pa.pf * 8));
                    
                    memcpy((u8 *)(temp + s_pa.pf * 8), (u8 *)s_pa.data , sizeof(s_pa.data));
                }
                else if(s_pa.type == FRAME_CONTROL)
                {
                   cl_log(INFO,"get bms control resp pf(%x)",s_pa.pf);
                   bms_service_data_que_delete_one(&s_pa);                     
                }
                
            }
        }
        #endif
        fifo_pop_len(fifo,PER_BMS_PACKET_LEN);
    }
    else
    {
        fifo_pop_len(fifo,1);
    }
        
}

void bms_service_timer_proc(void)
{
    switch(s_bms_ctl.state)        
    {
        case BMS_SERV_INIT:
            bms_service_init();
            break;
        case BMS_SERV_CONECT_OK:
            bms_service_connect_ok_proc();
            bms_service_control_process();
            break;
        case BMS_SERV_CLOSE:
            break;
        default:
            break;
    }
}

void bms_service_data_que_init(void)
{
    u8 i = 0;
    
    s_bms_que.in = 0;

    s_bms_que.out = 0;

    for(i = 0; i < MAX_BMS_QUE_COUNTS ; i ++)
    {
        if(s_bms_que.bms[i])
        {
            free(s_bms_que.bms[i]);

            s_bms_que.bms[i] = 0;
        }
    }
}

void bms_service_control_process(void)
{
    static u32 send_time  = 0;

    BMSPacketStruct *bms = bms_service_get_one_data();

    if(clock() - send_time >= 3)
    {
        if(bms)
        {
            send_time = clock();

            bms_com_write((u8 *)bms , sizeof(BMSPacketStruct));
        }
    }
}

bool bms_service_state(void)
{
    return s_bms_ctl.state == BMS_SERV_CONECT_OK?true:false;
}

BmsBatteryInfoStruct *bms_service_get_entry_addr(void)
{
    return &g_bms;
}


#define BRANCHSET(index,name) case index:\
                                       ctrl->name = value;\
                                       break;
                                      
                            

void bms_service_packet_control_info(BmsBatteryInfoSetStruct *ctrl,u16 index ,u16 value)
{
   switch (index)
   {

        BRANCHSET(0,total_ov_v)
        BRANCHSET(1,total_lm_v)
        BRANCHSET(2,single_ov_v)
        BRANCHSET(3,single_lm_v)
        //控制BMS电压恢复值写入
        BRANCHSET(4, total_ov_rcy_v) //rcy recovery总电压恢复
        BRANCHSET(5, total_lm_rcy_v)
        BRANCHSET(6, single_ov_rcy_v)
        BRANCHSET(7, single_lm_rcy_v)

        //控制BMS温度阈值与恢复值写入
        BRANCHSET(8, charge_temp_high_threshold)
        BRANCHSET(9, charge_temp_low_threshold)
        BRANCHSET(10, discharge_temp_high_threshold)
        BRANCHSET(11, discharge_temp_low_threshold)
        BRANCHSET(12, charge_temp_high_rcy_threshold)
        BRANCHSET(13, charge_temp_low_rcy_threshold)
        BRANCHSET(14, discharge_temp_high_rcy_threshold)
        BRANCHSET(15, discharge_temp_low_rcy_threshold)

         //控制BMS电流过流阈值和恢复值写入
        BRANCHSET(16, charge_over_cur_threshold)
        BRANCHSET(17, discharge_over_cur_threshold)
        BRANCHSET(18, charge_over_cur_rcy_threshold)
        BRANCHSET(19, discharge_over_cur_rcy_threshold)
        
        //控制BMS电压均衡开启电压、开启压差和关闭压差
        BRANCHSET(20, balance_open_vol)
        BRANCHSET(21, balance_open_vol_diff)
        BRANCHSET(22, balance_close_vol_diff)
        //BRANCHSET(24, reserv12)
        
        //控制BMS保护延时值写入
        BRANCHSET(23, overvoltage_protect_delay)
        BRANCHSET(24, undervoltage_protect_delay)
        BRANCHSET(25, overcurrent_protect_delay)
        BRANCHSET(26, single_high_temp_protect_delay)
        BRANCHSET(27, single_low_temp_protect_delay)
        BRANCHSET(28, MOS_high_temp_protect_delay)
        BRANCHSET(29, MOS_high_temp_threshold)
        BRANCHSET(30, MOS_high_temp_rcy_threshold)
        default:
        break;
    
   }
}

void bms_service_packet_control_info_insert(u8 index,BmsBatteryInfoSetStruct *set)
{
    BMSPacketStruct *bms = NULL;

    u16 calc_crc = 0;

    bms = (BMSPacketStruct *)malloc(sizeof(BMSPacketStruct));

    if(bms)
    {
        memcpy(bms->head,bms_head,3);

        bms->type = FRAME_CONTROL;

        bms->da   = 0xFF;

        bms->sa   = 0xFF;

        bms->pf   = index + 1;

        //memset(bms->data,0,8); //所有的控制器关闭
        switch(index)
        {
           case 0:
                memcpy(bms->data,(u8 *)&set->total_ov_v,8);
                break;
           case 1:
                memcpy(bms->data,(u8 *)&set->total_ov_rcy_v,8);
                break;
           case 2:
                memcpy(bms->data,(u8 *)&set->charge_temp_high_threshold,8);
                break;
           case 3:
                memcpy(bms->data,(u8 *)&set->charge_over_cur_threshold,8);
                break;
           case 4:
                memcpy(bms->data,(u8 *)&set->balance_open_vol,8);
                break;
           case 5:
                memcpy(bms->data,(u8 *)&set->overvoltage_protect_delay,8);
                break;
           default:
                free(bms);
                return;
        }

        calc_crc = ModBusCRC16(bms->frame,sizeof(bms->frame));

        bms->crc[0] = (calc_crc     )&0xFF; 

        bms->crc[1] = (calc_crc >> 8)&0xFF; 

        memcpy(bms->tail,bms_tail,3);

        bms_service_data_que_insert_one(bms);
    }
    else
    {
        LOG(FATAL,"BMS control packet malloc error");
    }
    
}


void bms_service_cut_power(void)
{
    BMSPacketStruct *bms = NULL;

    u16 calc_crc = 0;

    bms = (BMSPacketStruct *)malloc(sizeof(BMSPacketStruct));

    if(bms)
    {
        memcpy(bms->head,bms_head,3);

        bms->type = FRAME_CONTROL;

        bms->pf   = 0;

        bms->da   = 0xFF;

        bms->sa   = 0xFF;

        memset(bms->data,0,8); //所有的控制器关闭

        calc_crc = ModBusCRC16(bms->frame,sizeof(bms->frame));

        bms->crc[0] = (calc_crc     )&0xFF; 

        bms->crc[1] = (calc_crc >> 8)&0xFF; 

        memcpy(bms->tail,bms_tail,3);

        bms_service_data_que_insert_one(bms);
    }
    else
    {
        LOG(FATAL,"BMS control packet malloc error");
    }
    
}

void bms_service_restart_power(void)
{
    BMSPacketStruct *bms = NULL;

    u16 calc_crc = 0;

    bms = (BMSPacketStruct *)malloc(sizeof(BMSPacketStruct));

    if(bms)
    {
        memcpy(bms->head,bms_head,3);

        bms->type = FRAME_CONTROL;

        bms->pf   = 0;

        bms->da   = 0xFF;

        bms->sa   = 0xFF;

        memset(bms->data,1,8); //所有的控制器开启

        calc_crc = ModBusCRC16(bms->frame,sizeof(bms->frame));

        bms->crc[0] = (calc_crc     )&0xFF; 

        bms->crc[1] = (calc_crc >> 8)&0xFF; 

        memcpy(bms->tail,bms_tail,3);

        bms_service_data_que_insert_one(bms);
    }
    else
    {
        LOG(FATAL,"BMS control packet malloc error");
    }
    
}


void bms_service_data_que_delete_one(BMSPacketStruct *bms)
{
    u8 n = 0;

    BMSPacketStruct *q = NULL;

    s_bms_que.out = s_bms_que.out%MAX_BMS_QUE_COUNTS;

    n = s_bms_que.out;

    q = s_bms_que.bms[n];

    if(memcmp(bms,q,sizeof(BMSPacketStruct))==0)
    {
        cl_log(INFO,"BMS control que del one");
        
        free(q);

        s_bms_que.bms[n] = 0;
    }

    s_bms_que.out++;
}

void bms_service_data_que_insert_one(BMSPacketStruct *bms)
{
    u8 n = 0;

    //BmsDataFrameStruct *q = NULL;

    s_bms_que.in = s_bms_que.in%MAX_BMS_QUE_COUNTS;

    n = s_bms_que.in;

    //q = s_bms_que.bms[n];

    if(s_bms_que.bms[n] != NULL)
    {
        free(s_bms_que.bms[n]);

        s_bms_que.bms[n] = 0;
    }

    s_bms_que.bms[n] = bms;

    s_bms_que.in++;

    
}

BMSPacketStruct *bms_service_get_one_data(void)
{
    u8 n = 0;

    //BmsDataFrameStruct *q = NULL;
    
    if(s_bms_que.in != s_bms_que.out)
    {
        n = s_bms_que.out;

        if(s_bms_que.bms[n])
        {
            return s_bms_que.bms[n];
        }
    }

    return NULL;
}

