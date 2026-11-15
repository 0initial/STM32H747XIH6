/**
 ****************************************************************************************************
 * @file        sdram.c
 * @brief       SDRAM driver (CubeMX FMC + post-init sequence)
 ****************************************************************************************************
 */

#include "sdram.h"
#include <stdio.h>

/**
 * @brief  SDRAM mode register init and refresh rate (call after MX_FMC_Init)
 */
void sdram_init(void)
{
    // SDRAM初始化命令序列：CLK使能 -> 全部预充(PALL) -> 自刷新(AUTOREFRESH) -> 加载模式寄存器(LOAD_MODE)
    sdram_initialization_sequence();
    // 设置刷新计数：让SDRAM控制器周期性发出REFRESH命令，确保内部电荷不丢失
    HAL_SDRAM_ProgramRefreshRate(&hsdram2, SDRAM_REFRESH_COUNT);
}

/**
 * @brief  SDRAM controller initialization command sequence
 */
void sdram_initialization_sequence(void)
{
    uint32_t temp = 0;

    // 1) 使能SDRAM时钟（确保SDRAM内部时序基准存在）
    sdram_send_cmd(1, FMC_SDRAM_CMD_CLK_ENABLE, 1, 0);
    HAL_Delay(1);

    // 2) 全部预充电（PALL：关闭当前激活行，并把bank置于可激活状态）
    sdram_send_cmd(1, FMC_SDRAM_CMD_PALL, 1, 0);
    // 3) 执行自刷新（AUTOREFRESH_MODE：通过多次自刷新把阵列状态“稳定”到可正常访问）
    sdram_send_cmd(1, FMC_SDRAM_CMD_AUTOREFRESH_MODE, 8, 0);

    // 4) 组合SDRAM模式寄存器（Mode Register）
    //    该寄存器决定：突发长度/类型、CAS延迟、工作模式、写突发方式等。
    //    temp由下面的宏按位或组合得到，然后通过LOAD_MODE命令写入SDRAM。
    temp = (uint32_t)SDRAM_MODEREG_BURST_LENGTH_1 |
           SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL |
           SDRAM_MODEREG_CAS_LATENCY_2 |
           SDRAM_MODEREG_OPERATING_MODE_STANDARD |
           SDRAM_MODEREG_WRITEBURST_MODE_SINGLE;

    // 5) 加载模式寄存器（LOAD_MODE：把temp写入SDRAM模式寄存器）
    sdram_send_cmd(1, FMC_SDRAM_CMD_LOAD_MODE, 1, (uint16_t)temp);
}

/**
 * @brief  Send SDRAM command
 * @param  bankx   0: Bank1, 1: Bank2
 */
uint8_t sdram_send_cmd(uint8_t bankx, uint8_t cmd, uint8_t refresh, uint16_t regval)
{
    uint32_t target_bank;
    FMC_SDRAM_CommandTypeDef command;

    // bankx：0 表示 Bank1，1 表示 Bank2。
    // HAL_SDRAM_SendCommand内部需要的是“命令目标bank枚举”，因此这里做一次映射。
    if (bankx == 0)
    {
        target_bank = FMC_SDRAM_CMD_TARGET_BANK1;
    }
    else
    {
        target_bank = FMC_SDRAM_CMD_TARGET_BANK2;
    }

    // 组装要下发给FMC的SDRAM命令结构
    command.CommandMode = cmd;
    command.CommandTarget = target_bank;
    // AUTOREFRESH_MODE时refresh字段表示需要执行多少次自刷新；
    // 其他命令此字段通常不参与或由HAL忽略，但这里统一赋值即可。
    command.AutoRefreshNumber = refresh;
    // LOAD_MODE时regval字段表示要写入的模式寄存器(MR)内容；
    // 其他命令下该字段通常无效/被忽略。
    command.ModeRegisterDefinition = regval;

    if (HAL_SDRAM_SendCommand(&hsdram2, &command, 0x1000U) == HAL_OK)
    {
        // 返回0表示发送成功（工程原写法如此，保留逻辑不改）
        return 0;
    }

    // 返回1表示发送失败
    return 1;
}

/**
 * @brief  Probe SDRAM capacity by writing incremental patterns every 16KB
 */
void sdram_test(void)
{
    uint32_t i;
    uint32_t temp = 0;
    uint32_t sval = 0;

    // 以 16KB 为步长写入递增pattern：
    // 写入时使用 temp(从0开始递增)，每写一个“16KB块”就把temp++。
    // 若超过实际可用范围或访问开始不可靠，回读的pattern将不再保持递增。
    for (i = 0; i < 32U * 1024U * 1024U; i += 16U * 1024U)
    {
        *(volatile uint32_t *)(BANK6_SDRAM_ADDR + i) = temp;
        temp++;
    }

    // 再按相同步长读回，查找第一个“读回值不再满足递增”的点
    for (i = 0; i < 32U * 1024U * 1024U; i += 16U * 1024U)
    {
        temp = *(volatile uint32_t *)(BANK6_SDRAM_ADDR + i);

        if (i == 0U)
        {
            // i==0处记录起始值（用于后续估算“已经读回多少块”）
            sval = temp;
        }
        else if (temp <= sval)
        {
            // 递增pattern应始终大于起始值；
            // 当读回值出现不再递增/回环/错误时会触发这里并跳出循环
            break;
        }

        // 每成功读回一个16KB块，容量估算增加16KB。
        printf("SDRAM Capacity: %lu KB\r\n", (unsigned long)((temp - sval + 1U) * 16U));
    }
}
