/**
 ******************************************************************************
 * @file    bl24c16f.c
 * @brief   BL24C16F I2C EEPROM 驱动实现
 *
 * 寻址说明（与手册 Figure 5 对应）：
 *   8 位设备地址：| 1 | 0 | 1 | 0 | A10 | A9 | A8 | R/W |
 *   - 高 4 位固定为 1010
 *   - A10/A9/A8 为存储地址的高 3 位，决定使用哪个 I2C 设备地址
 *   - R/W：0=写，1=读
 *   - 字地址字节 = 存储地址低 8 位（A7~A0）
 *
 * 地址计算示例：
 *   存储地址 0x0156 → 设备写地址 0xA0，字地址 0x56
 *   存储地址 0x0234 → 设备写地址 0xA4，字地址 0x34
 ******************************************************************************
 */

#include "bl24c16f.h"

/* -------------------------------------------------------------------------- */
/*                              内部辅助函数                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief  根据存储地址计算 8 位 I2C 设备地址（含 R/W=0 的写方向）
 * @param  addr  存储器地址（0x0000 ~ 0x07FF）
 * @return 8 位设备地址，范围 0xA0 ~ 0xAE
 *
 * @note   公式：0xA0 | ((addr >> 8) & 0x07) << 1
 *         addr >> 8 取出 A10/A9/A8，左移 1 位填入设备地址的 B3~B1 位
 */
static uint16_t BL24C16F_GetDevAddr(uint16_t addr)
{
  return (uinit16_t)(BL24C16F_DEV_ADDR_BASE | (((addr >> 8) & 0x07U) << 1U));
}

/**
 * @brief  等待 EEPROM 内部写周期完成
 * @param  hdev  驱动句柄指针
 * @param  addr  当前写入的存储地址（用于确定轮询哪个设备地址块）
 * @retval HAL_OK      写周期完成，设备已就绪
 * @retval HAL_TIMEOUT 等待超时
 *
 * @note   页写或字节写结束后，芯片需约 5ms 内部写入时间，
 *         期间不响应 I2C，通过 IsDeviceReady 轮询检测
 */
static HAL_StatusTypeDef BL24C16F_WaitWriteCycle(BL24C16F_HandleTypeDef *hdev, uint16_t addr)
{
  uint32_t tickstart = HAL_GetTick();
  uint16_t dev_addr = BL24C16F_GetDevAddr(addr);

  while (HAL_I2C_IsDeviceReady(hdev->hi2c, dev_addr, 1U, 1U) != HAL_OK)
  {
    if ((HAL_GetTick() - tickstart) > hdev->timeout)
    {
      HAL_TIMEOUT;
    }
  }

  return HAL_OK;
}

/**
 * @brief  校验地址和长度参数是否合法
 * @param  addr  起始存储地址
 * @param  len   读写字节数
 * @retval HAL_OK    参数合法
 * @retval HAL_ERROR len 为 0 或 addr+len 超出芯片容量
 */
static HAL_StatusTypeDef BL24C16F_CheckParams(uint16_t addr, uint16_t len)
{
  if ((len == 0U) || ((uint32_t)addr + len > BL24C16F_TOTAL_SIZE))
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

/* -------------------------------------------------------------------------- */
/*                              对外接口函数                                   */
/* -------------------------------------------------------------------------- */

/**
 * @brief  初始化 EEPROM 驱动句柄，绑定 I2C 外设并设置默认超时
 */
void BL24C16F_Init(BL24C16F_HandleTypeDef *hdev, I2C_HandleTypeDef *hi2c)
{
  hdev->hi2c = hi2c;
  hdev->timeout = BL24C16F_DEFAULT_TIMEOUT;
}

/**
 * @brief  检测设备是否在线（使用第一地址块 0xA0 进行探测）
 */
HAL_StatusTypeDef BL24C16F_IsReady(BL24C16F_HandleTypeDef *hdev)
{
  if ((hdev == NULL) || (hdev->hi2c == NULL))
  {
    return HAL_ERROR;
  }

  return HAL_I2C_IsDeviceReady(hdev->hi2c, BL24C16F_DEV_ADDR_BASE, 3U, hdev->timeout);
}

/**
 * @brief  读取单字节（封装 BL24C16F_Read）
 */
HAL_StatusTypeDef BL24C16F_ReadByte(BL24C16F_HandleTypeDef *hdev, uint16_t addr, uint8_t *data)
{
  return BL24C16F_Read(hdev, addr, data, 1U);
}

/**
 * @brief  写入单字节（封装 BL24C16F_Write）
 */
HAL_StatusTypeDef BL24C16F_WriteByte(BL24C16F_HandleTypeDef *hdev, uint16_t addr, uint8_t data)
{
  return BL24C16F_Write(hdev, addr, &data, 1U);
}


