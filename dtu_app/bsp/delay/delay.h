#ifndef __DELAY_H
#define __DELAY_H 			   
#include "sys.h"
	 
void delay_init(void);
void delay_ms(u32 nms);
void delay_us(u32 nus);

typedef void (*fun_timeout_cb) (void); 

#endif





























