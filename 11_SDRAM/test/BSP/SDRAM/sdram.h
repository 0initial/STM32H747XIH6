/**
 ****************************************************************************************************
 * @file        sdram.h
 * @brief       SDRAM driver (STM32H747XIH6 + IS42S32800J, FMC Bank2)
 ****************************************************************************************************
 */

#ifndef BSP_SDRAM_H
#define BSP_SDRAM_H

#include "main.h"
#include "fmc.h"

#define BANK6_SDRAM_ADDR           ((uint32_t)0xD0000000U)

/* BANK6_SDRAM_ADDR：
 * 在 STM32H7 的 FMC 外部存储映射中，SDRAM 的某个FMC bank会对应到 Cortex-M 的地址空间。
 * 本工程测试时直接访问该基址（0xD0000000），用于验证外部SDRAM读写是否稳定。
 * 若你在 CubeMX 中选择的 SDRAM 连接/映射bank发生变化，需要同时检查这里的基址是否仍匹配。 */

/* SDRAM mode register：SDRAM 模式寄存器（Mode Register, MR）位域定义（用于 LOAD_MODE 命令）。 */
/* Burst Length（突发长度）选择：这里选择 Burst Length = 1（即按单次/不使用长突发的方式工作） */
#define SDRAM_MODEREG_BURST_LENGTH_1             ((uint16_t)0x0000U)
#define SDRAM_MODEREG_BURST_LENGTH_2             ((uint16_t)0x0001U)
#define SDRAM_MODEREG_BURST_LENGTH_4             ((uint16_t)0x0002U)
#define SDRAM_MODEREG_BURST_LENGTH_8             ((uint16_t)0x0004U)
/* Burst Type（突发类型）：Sequential（顺序）/Interleaved（交错） */
#define SDRAM_MODEREG_BURST_TYPE_SEQUENTIAL      ((uint16_t)0x0000U)
#define SDRAM_MODEREG_BURST_TYPE_INTERLEAVED     ((uint16_t)0x0008U)
/* CAS Latency（CAS延迟）：这里支持CL=2/3两种定义 */
#define SDRAM_MODEREG_CAS_LATENCY_2              ((uint16_t)0x0020U)
#define SDRAM_MODEREG_CAS_LATENCY_3              ((uint16_t)0x0030U)
/* Operating Mode（工作模式）：Standard（标准工作模式） */
#define SDRAM_MODEREG_OPERATING_MODE_STANDARD    ((uint16_t)0x0000U)
/* Write Burst Mode（写突发模式）：这里选择 Single（写突发采用单次） */
#define SDRAM_MODEREG_WRITEBURST_MODE_PROGRAMMED ((uint16_t)0x0000U)
#define SDRAM_MODEREG_WRITEBURST_MODE_SINGLE     ((uint16_t)0x0200U)

/* FMC ker_ck=240MHz, SDCLK=ker_ck/2=120MHz，刷新周期约 64ms，4096 行（行地址12位）下计算得到的刷新计数值。 */
/* 注意：refresh count 强相关于 SDRAM 时钟（SDCLK）和 CubeMX/HAL下实际的SDRAM刷新周期计算方式；
 * 只要你改了FMC/SDRAM时钟、行地址位宽或相关时序参数，就需要重新确认该值。 */
#define SDRAM_REFRESH_COUNT          (1855U)

void sdram_init(void);
void sdram_initialization_sequence(void);
uint8_t sdram_send_cmd(uint8_t bankx, uint8_t cmd, uint8_t refresh, uint16_t regval);
void sdram_test(void);

#endif /* BSP_SDRAM_H */
