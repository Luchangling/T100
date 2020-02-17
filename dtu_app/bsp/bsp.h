
#ifndef __BSP_H__
#define __BSP_H__

extern void bsp_init(void);

#define MODEL_POWER_ON()    GPIO_SetBits(GPIOA,GPIO_Pin_0)

#define MODEL_POWER_KEY_H() GPIO_SetBits(GPIOB, GPIO_Pin_8)
#define MODEL_POWER_KEY_L() GPIO_ResetBits(GPIOB, GPIO_Pin_8) 

#define MODEL_RESET_H() GPIO_SetBits(GPIOB, GPIO_Pin_9)
#define MODEL_RESET_L() GPIO_ResetBits(GPIOB, GPIO_Pin_9) 


#define TRANS_TTL2MODEL_OE1(STA) STA>0?GPIO_SetBits(GPIOC, GPIO_Pin_5):GPIO_ResetBits(GPIOC, GPIO_Pin_5)
#define TRANS_MODEL2TTL_OE2(STA) STA>0?GPIO_SetBits(GPIOC, GPIO_Pin_4):GPIO_ResetBits(GPIOC, GPIO_Pin_4)
void ResetSystem(u8 *string);
void iwdg_feed(void);



#endif

