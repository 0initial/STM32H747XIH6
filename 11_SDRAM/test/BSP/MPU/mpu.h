/**
 ****************************************************************************************************
 * @file        mpu.h
 * @brief       MPU memory protection
 ****************************************************************************************************
 */

#ifndef BSP_MPU_H
#define BSP_MPU_H

#include "main.h"

uint8_t mpu_set_protection(uint32_t baseaddr, uint32_t size, uint32_t rnum,
                           uint8_t de, uint8_t ap, uint8_t sen, uint8_t cen, uint8_t ben);
void mpu_memory_protection(void);

#endif /* BSP_MPU_H */
