/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef _FIFO_H_
#define _FIFO_H_
/* Includes ------------------------------------------------------------------*/
#include "sys.h"

typedef struct
{
    u32 head;  /* output conut */
    u32 tail;     /*  input count   */
    u32 size;   /*  total size  */
    u32 len;    /*  data length */
    u8 * baseAddr;
}fifo_TypeDef;


ErrorStatus fifo_Init(fifo_TypeDef * _fifo, u32 size);

ErrorStatus fifo_DeInit(fifo_TypeDef * _fifo);
ErrorStatus fifo_Reset(fifo_TypeDef * _fifo);
GM_ERRCODE fifo_delete(fifo_TypeDef * fifo);


u32 fifo_insert(fifo_TypeDef * _fifo, u8 *data, u32 len);
u32 fifo_retrieve(fifo_TypeDef * _fifo, u8 *data, u32 len);

u32 fifo_GetLen(fifo_TypeDef * _fifo);
FlagStatus fifo_NotFull(fifo_TypeDef * _fifo);
u32 fifo_GetFreeSpace(fifo_TypeDef * _fifo);

ErrorStatus fifo_peek(fifo_TypeDef * fifo, u8 *data, u32 len);


ErrorStatus fifo_peek_and_get_len(fifo_TypeDef * fifo, u8 *data, u32 *len_p);

ErrorStatus fifo_pop_len(fifo_TypeDef * fifo, u32 len);

ErrorStatus fifo_peek_until(fifo_TypeDef* fifo, u8* data, u16* len_p, const u8 until_char);

#endif

