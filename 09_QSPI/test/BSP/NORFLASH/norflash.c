/**
 ****************************************************************************************************
 * @file        norflash.c
 * @brief       W25Q 系列 SPI NOR Flash 驱动（基于 QSPI 四线读写）
 * @note        使用前须先调用 MX_QUADSPI_Init()，再调用 norflash_init()。
 ****************************************************************************************************
 */

#include "norflash.h"
#include "qspi.h"
#include <stdio.h>

uint16_t g_norflash_type = W25Q256;

/* 地址模式：2=24bit，3=32bit（W25Q256 需 32bit） */
static volatile uint8_t s_norflash_addrw = 2U;

/* 扇区擦写缓存 */
static uint8_t s_norflash_sector_buf[NORFLASH_SECTOR_SIZE];

static void norflash_wait_busy(void);
static void norflash_write_page(uint8_t *pbuf, uint32_t addr, uint16_t datalen);

/**
 * @brief  根据 JEDEC ID 推断 16 位芯片型号（与 norflash.h 中宏一致）
 */
static uint16_t norflash_type_from_jedec(uint32_t jedec)
{
    uint8_t mf  = (uint8_t)((jedec >> 16) & 0xFFU);
    uint8_t cap = (uint8_t)(jedec & 0xFFU);

    /* 兆易创新 GD25Q 系列（板载实际为 GD25Q256，JEDEC = C8 40 19） */
    if (mf == 0xC8U)
    {
        switch (cap)
        {
            case 0x19U: return GD25Q256;
            case 0x18U: return GD25Q128;
            case 0x17U: return GD25Q64;
            default:    return 0;
        }
    }

    /* 华邦 W25Q 系列 */
    if (mf != 0xEFU)
    {
        return 0;
    }

    switch (cap)
    {
        case 0x19U: return W25Q256;
        case 0x18U: return W25Q256;
        case 0x17U: return W25Q128;
        case 0x16U: return W25Q64;
        default:    return 0;
    }
}

/**
 * @brief  综合 JEDEC 与 0x90 指令 ID 判定芯片型号
 */
static uint16_t norflash_resolve_chip_id(uint32_t jedec, uint16_t id90)
{
    if (id90 == W25Q128 || id90 == W25Q256 || id90 == BY25Q128 ||
        id90 == BY25Q256 || id90 == GD25Q128 || id90 == GD25Q256)
    {
        return id90;
    }

    return norflash_type_from_jedec(jedec);
}

/**
 * @brief  初始化外部 NOR Flash（流程与实验23一致）
 * @note   JEDEC 读取 -> QE 使能 -> 0x90 读 ID -> 256Mbit 芯片进入 4 字节地址
 * @retval 0 成功，1 未识别到支持的 Flash
 */
uint8_t norflash_init(void)
{
    uint8_t temp;
    uint32_t jedec;
    uint16_t id90;

    /* 先读 JEDEC（单线 SPI，无需 QE） */
    jedec = norflash_read_jedec_id();

    /* 与实验23相同：QE 使能后再读 0x90 ID */
    norflash_qe_enable();
    id90 = norflash_read_id();

    g_norflash_type = norflash_resolve_chip_id(jedec, id90);

    if (g_norflash_type == W25Q256 || g_norflash_type == BY25Q256 ||
        g_norflash_type == GD25Q256)
    {
        s_norflash_addrw = 3U;

        /* SR3 的 4 字节地址位是华邦特有的，GD25Q 用 0xB7 指令即可，别乱写 SR3 */
        if (g_norflash_type != GD25Q256)
        {
            temp = norflash_read_sr(3);

            if ((temp & 0x01U) == 0U)
            {
                norflash_write_enable();
                temp |= (uint8_t)(1U << 1);
                norflash_write_sr(3, temp);
                HAL_Delay(20);
            }
        }

        norflash_write_enable();
        qspi_send_cmd(FLASH_Enable4ByteAddr, 0,
                      QSPI_MAKE_MODE(1, 0, 0, 0), 0);
    }
    else if (g_norflash_type != 0U)
    {
        s_norflash_addrw = 2U;
    }

    printf("NOR Flash JEDEC: 0x%06lX, ID(0x90): 0x%04X, type: 0x%04X, addr: %u-bit\r\n",
           (unsigned long)jedec, id90, g_norflash_type,
           (unsigned int)(s_norflash_addrw * 8U + 8U));

    return (g_norflash_type != 0U) ? 0U : 1U;
}

/**
 * @brief  等待 Flash 内部操作完成（BUSY 清零）
 */
static void norflash_wait_busy(void)
{
    while ((norflash_read_sr(1) & 0x01U) != 0U)
    {
    }
}

/**
 * @brief  使能 QE 位，开启 IO2/IO3 四线模式
 */
void norflash_qe_enable(void)
{
    uint8_t sr2 = norflash_read_sr(2);

    if ((sr2 & 0x02U) == 0U)
    {
        norflash_write_enable();
        sr2 |= (uint8_t)(1U << 1);
        norflash_write_sr(2, sr2);
    }
}

/**
 * @brief  关闭 QE 位，退出 QSPI 四线模式
 */
void norflash_qspi_disable(void)
{
    uint8_t sr2 = norflash_read_sr(2);

    if ((sr2 & 0x02U) != 0U)
    {
        norflash_write_enable();
        sr2 &= (uint8_t)~(1U << 1);
        norflash_write_sr(2, sr2);
    }
}

void norflash_write_enable(void)
{
    qspi_send_cmd(FLASH_WriteEnable, 0, QSPI_MAKE_MODE(1, 0, 0, 0), 0);
}

void norflash_write_disable(void)
{
    qspi_send_cmd(FLASH_WriteDisable, 0, QSPI_MAKE_MODE(1, 0, 0, 0), 0);
}

/**
 * @brief  读状态寄存器
 * @param  regno : 1~3
 */
uint8_t norflash_read_sr(uint8_t regno)
{
    uint8_t cmd;
    uint8_t byte = 0;

    switch (regno)
    {
        case 2:  cmd = FLASH_ReadStatusReg2; break;
        case 3:  cmd = FLASH_ReadStatusReg3; break;
        default: cmd = FLASH_ReadStatusReg1; break;
    }

    qspi_send_cmd(cmd, 0, QSPI_MAKE_MODE(1, 0, 0, 1), 0);
    qspi_receive(&byte, 1);
    return byte;
}

/**
 * @brief  写状态寄存器
 * @param  regno : 1~3
 * @param  sr    : 写入值
 */
void norflash_write_sr(uint8_t regno, uint8_t sr)
{
    uint8_t cmd;

    switch (regno)
    {
        case 2:  cmd = FLASH_WriteStatusReg2; break;
        case 3:  cmd = FLASH_WriteStatusReg3; break;
        default: cmd = FLASH_WriteStatusReg1; break;
    }

    qspi_send_cmd(cmd, 0, QSPI_MAKE_MODE(1, 0, 0, 1), 0);
    qspi_transmit(&sr, 1);
}

/**
 * @brief  读取 JEDEC ID（0x9F，3 字节：厂商/类型/容量）
 */
uint32_t norflash_read_jedec_id(void)
{
    uint8_t buf[3] = {0};

    /* 单线指令 + 单线读 3 字节，无地址（与实验23 指令编码一致） */
    qspi_send_cmd(FLASH_JedecDeviceID, 0, QSPI_MAKE_MODE(1, 0, 0, 1), 0);
    (void)qspi_receive(buf, 3);

    return ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];
}

/**
 * @brief  读取制造商/设备 ID（0x90 指令，实验23 同款时序）
 */
uint16_t norflash_read_id(void)
{
    uint8_t temp[2] = {0};
    uint16_t deviceid;

    /* 单线数据 + 24 位地址 + 单线地址 + 单线指令 */
    qspi_send_cmd(FLASH_ManufactDeviceID, 0,
                  (uint8_t)((1U << 6) | (2U << 4) | (1U << 2) | (1U << 0)), 0);
    (void)qspi_receive(temp, 2);
    deviceid = (uint16_t)(((uint16_t)temp[0] << 8) | temp[1]);

    if (deviceid == W25Q256 || deviceid == BY25Q256 || deviceid == GD25Q256)
    {
        s_norflash_addrw = 3U;
    }

    return deviceid;
}

/**
 * @brief  四线快速读
 * @param  pbuf    : 数据缓冲区
 * @param  addr    : 起始地址
 * @param  datalen : 长度（字节）
 */
void norflash_read(uint8_t *pbuf, uint32_t addr, uint32_t datalen)
{
    qspi_send_cmd(FLASH_FastReadQuad, addr,
                  QSPI_MAKE_MODE(1, 3, s_norflash_addrw, 3), 6);
    qspi_receive(pbuf, datalen);
}

/**
 * @brief  页编程（最多 256 字节，且不跨页）
 */
static void norflash_write_page(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    norflash_write_enable();
    qspi_send_cmd(FLASH_PageProgramQuad, addr,
                  QSPI_MAKE_MODE(1, 1, s_norflash_addrw, 3), 0);
    qspi_transmit(pbuf, datalen);
    norflash_wait_busy();
}

/**
 * @brief  无擦除检查写入（目标区域须已为 0xFF）
 */
void norflash_write_nocheck(uint8_t *pbuf, uint32_t addr, uint32_t datalen)
{
    uint16_t pageremain = (uint16_t)(NORFLASH_PAGE_SIZE - (addr % NORFLASH_PAGE_SIZE));

    if (datalen <= pageremain)
    {
        pageremain = (uint16_t)datalen;
    }

    while (1)
    {
        norflash_write_page(pbuf, addr, pageremain);

        if (datalen == pageremain)
        {
            break;
        }

        pbuf += pageremain;
        addr += pageremain;
        datalen -= pageremain;

        pageremain = (datalen > NORFLASH_PAGE_SIZE) ? NORFLASH_PAGE_SIZE : (uint16_t)datalen;
    }
}

/**
 * @brief  带扇区擦除的写入（自动读-改-写）
 */
void norflash_write(uint8_t *pbuf, uint32_t addr, uint32_t datalen)
{
    uint32_t secpos;
    uint16_t secoff;
    uint16_t secremain;
    uint16_t i;

    secpos = addr / NORFLASH_SECTOR_SIZE;
    secoff = (uint16_t)(addr % NORFLASH_SECTOR_SIZE);
    secremain = (uint16_t)(NORFLASH_SECTOR_SIZE - secoff);

    if (datalen <= secremain)
    {
        secremain = (uint16_t)datalen;
    }

    while (1)
    {
        norflash_read(s_norflash_sector_buf, secpos * NORFLASH_SECTOR_SIZE, NORFLASH_SECTOR_SIZE);

        for (i = 0; i < secremain; i++)
        {
            if (s_norflash_sector_buf[secoff + i] != 0xFFU)
            {
                break;
            }
        }

        if (i < secremain)
        {
            norflash_erase_sector(secpos);

            for (i = 0; i < secremain; i++)
            {
                s_norflash_sector_buf[secoff + i] = pbuf[i];
            }

            norflash_write_nocheck(s_norflash_sector_buf, secpos * NORFLASH_SECTOR_SIZE,
                                   NORFLASH_SECTOR_SIZE);
        }
        else
        {
            norflash_write_nocheck(pbuf, addr, secremain);
        }

        if (datalen == secremain)
        {
            break;
        }

        secpos++;
        secoff = 0;
        pbuf += secremain;
        addr += secremain;
        datalen -= secremain;
        secremain = (datalen > NORFLASH_SECTOR_SIZE) ? NORFLASH_SECTOR_SIZE : (uint16_t)datalen;
    }
}

/**
 * @brief  整片擦除（耗时很长）
 */
void norflash_erase_chip(void)
{
    norflash_write_enable();
    norflash_wait_busy();
    qspi_send_cmd(FLASH_ChipErase, 0, QSPI_MAKE_MODE(1, 0, 0, 0), 0);
    norflash_wait_busy();
}

/**
 * @brief  擦除一个扇区
 * @param  saddr : 扇区号（从 0 开始），非字节地址
 */
void norflash_erase_sector(uint32_t saddr)
{
    uint32_t addr = saddr * NORFLASH_SECTOR_SIZE;

    norflash_write_enable();
    norflash_wait_busy();
    qspi_send_cmd(FLASH_SectorErase, addr,
                  QSPI_MAKE_MODE(1, 1, s_norflash_addrw, 0), 0);
    norflash_wait_busy();
}
