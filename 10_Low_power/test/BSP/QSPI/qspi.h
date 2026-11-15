/**
 ****************************************************************************************************
 * @file        qspi.h
 * @brief       QSPI 外设底层驱动（命令发送 / 数据收发）
 * @note        硬件初始化由 CubeMX 生成的 quadspi.c 完成，本模块仅封装 HAL 命令与传输接口。
 ****************************************************************************************************
 */

#ifndef __BSP_QSPI_H
#define __BSP_QSPI_H

#include "main.h"
#include "quadspi.h"

/**
 * @brief  线模式编码（与参考实验 norflash 驱动一致）
 *         mode[1:0] : 指令线数  00无 01单 10双 11四
 *         mode[3:2] : 地址线数
 *         mode[5:4] : 地址位宽  00=8 01=16 10=24 11=32
 *         mode[7:6] : 数据线数
 */
#define QSPI_MODE_INSTR_SHIFT   0U
#define QSPI_MODE_ADDR_SHIFT    2U
#define QSPI_MODE_ADDRSIZE_SHIFT 4U
#define QSPI_MODE_DATA_SHIFT    6U

#define QSPI_MAKE_MODE(instr, addr, addrsize, data) \
    (uint8_t)(((data) << QSPI_MODE_DATA_SHIFT) | ((addrsize) << QSPI_MODE_ADDRSIZE_SHIFT) | \
              ((addr) << QSPI_MODE_ADDR_SHIFT) | ((instr) << QSPI_MODE_INSTR_SHIFT))

uint8_t qspi_wait_flag(uint32_t flag, uint8_t sta, uint32_t wtime);
void    qspi_send_cmd(uint8_t cmd, uint32_t addr, uint8_t mode, uint8_t dmcycle);
uint8_t qspi_receive(uint8_t *buf, uint32_t datalen);
uint8_t qspi_transmit(uint8_t *buf, uint32_t datalen);

#endif /* __BSP_QSPI_H */
