
#ifndef __AGPS_SERVICE_H__
#define __AGPS_SERVICE_H__

GM_ERRCODE agps_service_create(void);

GM_ERRCODE agps_service_timer_proc(void);

s32 qi_http_get_urc_recv_cb(u8 *urc_str, u16 len);

s32 qi_http_read_urc_recv_cb(u8 *urc_str, u16 len);

void agps_service_rewite_data(void);

#endif


