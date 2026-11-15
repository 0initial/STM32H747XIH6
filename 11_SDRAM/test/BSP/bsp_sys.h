/**
 ****************************************************************************************************
 * @file        bsp_sys.h
 * @brief       BSP system helpers
 ****************************************************************************************************
 */

#ifndef BSP_SYS_H
#define BSP_SYS_H

#include "main.h"

void bsp_cache_enable(void);
// bsp_cache_enable：开启ICache/DCache，并设置SCB->CACR相关位（用于改善总线访问性能）

#endif /* BSP_SYS_H */
