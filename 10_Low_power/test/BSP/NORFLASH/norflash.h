/**
 ****************************************************************************************************
 * @file        norflash.h
 * @brief       W25Q 系列 SPI NOR Flash 驱动（QSPI 四线模式）
 * @note        默认适配板载 W25Q256（32MB），兼容 W25Q128 等容量。
 ****************************************************************************************************
 */

#ifndef __BSP_NORFLASH_H
#define __BSP_NORFLASH_H

#include "main.h"

/* Flash 芯片 ID */
#define W25Q80      0xEF13U
#define W25Q16      0xEF14U
#define W25Q32      0xEF15U
#define W25Q64      0xEF16U
#define W25Q128     0xEF17U
#define W25Q256     0xEF18U
#define BY25Q64     0x6816U
#define BY25Q128    0x6817U
#define BY25Q256    0x6818U
#define NM25Q64     0x5216U
#define NM25Q128    0x5217U
#define GD25Q64     0xC816U
#define GD25Q128    0xC817U
#define GD25Q256    0xC818U   /* 板载实际芯片：GD25Q256，JEDEC = C8 40 19 */

/* 容量与扇区大小（字节） */
#define NORFLASH_SECTOR_SIZE      4096U
#define NORFLASH_PAGE_SIZE        256U
#define NORFLASH_SIZE_W25Q256     (32U * 1024U * 1024U)

extern uint16_t g_norflash_type;

/* 指令集（Winbond W25Q 兼容） */
#define FLASH_WriteEnable           0x06U
#define FLASH_WriteDisable          0x04U
#define FLASH_ReadStatusReg1        0x05U
#define FLASH_ReadStatusReg2        0x35U
#define FLASH_ReadStatusReg3        0x15U
#define FLASH_WriteStatusReg1       0x01U
#define FLASH_WriteStatusReg2       0x31U
#define FLASH_WriteStatusReg3       0x11U
#define FLASH_FastReadQuad          0xEBU
#define FLASH_PageProgramQuad       0x32U
#define FLASH_SectorErase           0x20U
#define FLASH_ChipErase             0xC7U
#define FLASH_ManufactDeviceID      0x90U
#define FLASH_JedecDeviceID         0x9FU
#define FLASH_Enable4ByteAddr       0xB7U

uint8_t  norflash_init(void);
uint32_t norflash_read_jedec_id(void);
uint16_t norflash_read_id(void);
void     norflash_qe_enable(void);
void     norflash_qspi_disable(void);
void     norflash_write_enable(void);
void     norflash_write_disable(void);
uint8_t  norflash_read_sr(uint8_t regno);
void     norflash_write_sr(uint8_t regno, uint8_t sr);
void     norflash_erase_chip(void);
void     norflash_erase_sector(uint32_t saddr);
void     norflash_read(uint8_t *pbuf, uint32_t addr, uint32_t datalen);
void     norflash_write_nocheck(uint8_t *pbuf, uint32_t addr, uint32_t datalen);
void     norflash_write(uint8_t *pbuf, uint32_t addr, uint32_t datalen);

#endif /* __BSP_NORFLASH_H */
