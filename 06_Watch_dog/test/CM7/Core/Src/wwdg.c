/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    wwdg.c
  * @brief   This file provides code for the configuration
  *          of the WWDG instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "wwdg.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

WWDG_HandleTypeDef hwwdg1;

/* WWDG1 init function */
void MX_WWDG1_Init(void)
{

  /* USER CODE BEGIN WWDG1_Init 0 */

  /* USER CODE END WWDG1_Init 0 */

  /* USER CODE BEGIN WWDG1_Init 1 */

  /* USER CODE END WWDG1_Init 1 */
  hwwdg1.Instance = WWDG1;
  /* PCLK3 = 120 MHz, prescaler 128 -> 1 tick = 4096*128/120MHz = 4.369 ms.
     Counter MUST be 0x41..0x7F: 0x40 underflows to 0x3F in ONE tick and resets at once.
     0x7F -> 64 ticks -> max 279.6 ms; window 0x68 -> refresh allowed after 23 ticks = 100.5 ms. */
  hwwdg1.Init.Prescaler = WWDG_PRESCALER_128;
  hwwdg1.Init.Window = 104;  /* 0x68: too early a refresh also resets */
  hwwdg1.Init.Counter = 127; /* 0x7F */
  hwwdg1.Init.EWIMode = WWDG_EWI_DISABLE;
  if (HAL_WWDG_Init(&hwwdg1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN WWDG1_Init 2 */
  __HAL_DBGMCU_FREEZE_WWDG1(); /* freeze WWDG when the core is halted by the debugger */

  /* USER CODE END WWDG1_Init 2 */

}

void HAL_WWDG_MspInit(WWDG_HandleTypeDef* wwdgHandle)
{

  if(wwdgHandle->Instance==WWDG1)
  {
  /* USER CODE BEGIN WWDG1_MspInit 0 */

  /* USER CODE END WWDG1_MspInit 0 */
    /* WWDG1 clock enable */
    HAL_RCCEx_WWDGxSysResetConfig(RCC_WWDG1);
    __HAL_RCC_WWDG1_CLK_ENABLE();
  /* USER CODE BEGIN WWDG1_MspInit 1 */

  /* USER CODE END WWDG1_MspInit 1 */
  }
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */

