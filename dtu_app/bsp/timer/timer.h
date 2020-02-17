#ifndef __TIMER_H
#define __TIMER_H
#include "sys.h"

extern void TIM3_Int_Init(void);

extern u32 clock(void);

extern u32 util_clock(void);

extern u32 get_local_utc_sec(void); 

extern void set_local_time(u32 ticks);
#endif
