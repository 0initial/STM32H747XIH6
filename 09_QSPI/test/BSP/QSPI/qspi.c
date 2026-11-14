/**
 ****************************************************************************************************
 * @file        qspi.c
 * @brief       QSPI 外设底层驱动
 * @note        使用 CubeMX 生成的 hqspi 句柄，不在此重复调用 HAL_QSPI_MspInit。
 ****************************************************************************************************
 */

#include "qspi.h"

/* 指向 CubeMX 初始化的 QSPI 句柄 */
#define QSPI_HANDLE  (&hqspi)

/**
 * @brief  中止上一次传输并清除 QSPI 状态标志
 */
static void qspi_abort(void)
{
    (void)HAL_QSPI_Abort(QSPI_HANDLE);
}

/**
 * @brief  轮询等待 QSPI 状态寄存器标志
 * @param  flag  : QUADSPI->SR 中要检查的位（如 QSPI_FLAG_BUSY）
 * @param  sta   : 期望的标志状态（0 或 1）
 * @param  wtime : 最大轮询次数
 * @retval 0 成功，1 超时
 */
uint8_t qspi_wait_flag(uint32_t flag, uint8_t sta, uint32_t wtime)
{
    while (wtime--)
    {
        uint8_t flagsta = (QUADSPI->SR & flag) ? 1U : 0U;

        if (flagsta == sta)
        {
            return 0;
        }
    }

    return 1;
}

/**
 * @brief  发送 QSPI 命令（无数据阶段或后续由 Receive/Transmit 完成）
 * @param  cmd     : Flash 指令码
 * @param  addr    : 地址字段（按 mode 决定位宽）
 * @param  mode    : 线模式，见 QSPI_MAKE_MODE / 参考实验编码
 * @param  dmcycle : Dummy 空周期数
 */
void qspi_send_cmd(uint8_t cmd, uint32_t addr, uint8_t mode, uint8_t dmcycle)
{
    QSPI_CommandTypeDef cmdcfg = {0};

    qspi_abort();

    cmdcfg.Instruction        = cmd;
    cmdcfg.Address            = addr;
    cmdcfg.DummyCycles        = dmcycle;
    cmdcfg.SIOOMode           = QSPI_SIOO_INST_EVERY_CMD;
    cmdcfg.AlternateByteMode  = QSPI_ALTERNATE_BYTES_NONE;
    cmdcfg.DdrMode            = QSPI_DDR_MODE_DISABLE;
    cmdcfg.DdrHoldHalfCycle   = QSPI_DDR_HHC_ANALOG_DELAY;

    switch ((mode >> QSPI_MODE_INSTR_SHIFT) & 0x03U)
    {
        case 0: cmdcfg.InstructionMode = QSPI_INSTRUCTION_NONE; break;
        case 1: cmdcfg.InstructionMode = QSPI_INSTRUCTION_1_LINE; break;
        case 2: cmdcfg.InstructionMode = QSPI_INSTRUCTION_2_LINES; break;
        default: cmdcfg.InstructionMode = QSPI_INSTRUCTION_4_LINES; break;
    }

    switch ((mode >> QSPI_MODE_ADDR_SHIFT) & 0x03U)
    {
        case 0: cmdcfg.AddressMode = QSPI_ADDRESS_NONE; break;
        case 1: cmdcfg.AddressMode = QSPI_ADDRESS_1_LINE; break;
        case 2: cmdcfg.AddressMode = QSPI_ADDRESS_2_LINES; break;
        default: cmdcfg.AddressMode = QSPI_ADDRESS_4_LINES; break;
    }

    switch ((mode >> QSPI_MODE_ADDRSIZE_SHIFT) & 0x03U)
    {
        case 0: cmdcfg.AddressSize = QSPI_ADDRESS_8_BITS; break;
        case 1: cmdcfg.AddressSize = QSPI_ADDRESS_16_BITS; break;
        case 2: cmdcfg.AddressSize = QSPI_ADDRESS_24_BITS; break;
        default: cmdcfg.AddressSize = QSPI_ADDRESS_32_BITS; break;
    }

    switch ((mode >> QSPI_MODE_DATA_SHIFT) & 0x03U)
    {
        case 0: cmdcfg.DataMode = QSPI_DATA_NONE; break;
        case 1: cmdcfg.DataMode = QSPI_DATA_1_LINE; break;
        case 2: cmdcfg.DataMode = QSPI_DATA_2_LINES; break;
        default: cmdcfg.DataMode = QSPI_DATA_4_LINES; break;
    }

    (void)HAL_QSPI_Command(QSPI_HANDLE, &cmdcfg, 5000);
}

/**
 * @brief  接收指定长度数据（需先 qspi_send_cmd 配置好读命令）
 * @param  buf     : 接收缓冲区
 * @param  datalen : 字节数
 * @retval 0 成功，1 失败
 */
uint8_t qspi_receive(uint8_t *buf, uint32_t datalen)
{
    QSPI_HANDLE->Instance->DLR = datalen - 1U;

    if (HAL_QSPI_Receive(QSPI_HANDLE, buf, 5000) == HAL_OK)
    {
        return 0;
    }

    return 1;
}

/**
 * @brief  发送指定长度数据（需先 qspi_send_cmd 配置好写命令）
 * @param  buf     : 发送缓冲区
 * @param  datalen : 字节数
 * @retval 0 成功，1 失败
 */
uint8_t qspi_transmit(uint8_t *buf, uint32_t datalen)
{
    QSPI_HANDLE->Instance->DLR = datalen - 1U;

    if (HAL_QSPI_Transmit(QSPI_HANDLE, buf, 5000) == HAL_OK)
    {
        return 0;
    }

    return 1;
}
