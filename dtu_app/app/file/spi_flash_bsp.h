
#ifndef __SPI_FLASH_BSP_H__
#define __SPI_FLASH_BSP_H__

#include "sys.h"
#include "stm32f10x_spi.h"
#include "w25x_spi_flash_reg.h"
#if USE_INTERNAL_FLASH == 0
void spi_flash_init(void);


void SPI_Flash_Read(u8* pBuffer,u32 ReadAddr,u16 NumByteToRead);


void SPI_Flash_Write(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite);

void SPI_Flash_Write_NoCheck(u8* pBuffer,u32 WriteAddr,u16 NumByteToWrite);

#endif
#endif

