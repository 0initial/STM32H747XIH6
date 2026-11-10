/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define UART1_RX_BUFFER_SIZE 100U
#define DCACHE_LINE_SIZE 32U

#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
__attribute__((section(".dma_buffer"), aligned(DCACHE_LINE_SIZE))) static uint8_t Rx_data[UART1_RX_BUFFER_SIZE];

// 串口返回固定提示字符串
static const uint8_t uart_led_on_msg[] = "LED ON\r\n";
static const uint8_t uart_led_off_msg[] = "LED OFF\r\n";

/* 对 UART1 接收缓冲区做 D-Cache 失效处理
 * 原因:
 * DMA 接收数据时，数据是直接写到 RAM 中的，
 * 但 CPU 读取时可能还在读旧的 Cache 数据。
 * 所以在读取 Rx_data 之前，需要先把这段缓存失效，
 * 让 CPU 下一次读取时重新从 RAM 取最新数据。
 *
 * 这里之所以要重新计算对齐地址和对齐长度，
 * 是因为 SCB_InvalidateDCache_by_Addr() 通常要求
 * 地址和长度按 Cache Line 对齐。
 */
static void UART1_InvalidateRxBufferCache(void)
{
  // 原始缓冲区起始地址
  uint32_t addr = (uint32_t)Rx_data;

  // 向下对齐到 Cache Line 边界
  uint32_t aligned_addr = addr & ~(DCACHE_LINE_SIZE - 1U);

  // 计算覆盖整个缓冲区所需的对其长度
  // 先算出 [aligned_size, Rx_data结束地址] 的实际跨度
  // 再向上对齐到 Cache Line 的整数倍
  uint32_t aligned_size = (((addr + UART1_RX_BUFFER_SIZE) - aligned_addr) + (DCACHE_LINE_SIZE - 1U)) & ~(DCACHE_LINE_SIZE - 1U);

  // 使这一段 D-cache 失效，确保后续读取的是 DMA 写入 RAM 的最新数据
  SCB_InvalidateDCache_by_Addr((uint32_t *)aligned_addr, (int32_t)aligned_size);
}

/* 启动 UART1 的 ReceiveToIdle DMA 接收
 *
 * ReceiveToIdle 模式含义:
 * DMA 持续接收数据，一旦检测到串口空闲 (Idle) 或者缓冲区满，
 * 就触发回调 HAL_UARTEx_RxEventCallback()。
 */
static void UART1_StartReceiveToIdle(void)
{
  /* 启动接收前先失效接收缓冲区对应的 Cache，
   * 避免 CPU 后面读到旧缓存内容
   */
  UART1_InvalidateRxBufferCache();

  /* 启动 UART DMA 空闲接收 */
  if (HAL_UARTEx_ReceiveToIdle_DMA(&huart1, Rx_data, UART1_RX_BUFFER_SIZE) != HAL_OK)
  {
    Error_Handler();
  }

  /* 关闭 DMA 半传输中断
   * 这里只关心"接收完成/空闲事件"，不需要处理中途一半的数据
   */
  __HAL_DMA_DISABLE_IT(huart1.hdmarx, DMA_IT_HT);
}

/* UART1 的 DMA 空闲接收事件回调
 *
 * 当满足以下条件之一时会进入这里：
 * 1. 检测到串口空闲
 * 2. DMA 缓冲区接收满
 *
 * Size 表示本次实际接收到的字节数
 */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  // 本地命令缓冲区，多留 1 个字节给字符串结束符 '\0'
  char cmd[UART1_RX_BUFFER_SIZE + 1U];
  uint16_t copy_size;

  // 只处理 USART1 的接收事件
  if (huart->Instance == USART1)
  {
    // DMA 已经把数据写入 RAM
    // 这里再次失效 Cache，确保 CPU 读取到的是最新的数据
    UART1_InvalidateRxBufferCache();

    // 做长度保护，避免越界
    copy_size = Size;
    if (copy_size > UART1_RX_BUFFER_SIZE)
    {
      copy_size = UART1_RX_BUFFER_SIZE;
    }

    // 将 DMA 接收到的数据拷贝到本地缓冲区
    memcpy(cmd, Rx_data, copy_size);
    // 手动补字符串结束符，方便后面用 strstr 做字符串匹配
    cmd[copy_size] = '\0';

    // 判断是否收到
    if (strstr(cmd, "LED ON") != NULL)
    {
      // 通过 DMA 异步发送返回消息
      HAL_UART_Transmit_DMA(&huart1, uart_led_on_msg, sizeof(uart_led_on_msg) - 1U);
    }
    else if (strstr(cmd, "LED OFF") != NULL)
    {
      HAL_UART_Transmit_DMA(&huart1, uart_led_off_msg, sizeof(uart_led_off_msg) - 1U);
    }

    // 重新开启下一轮空闲接收
    UART1_StartReceiveToIdle();
  }
}
/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void)
{
  /* USER CODE BEGIN 1 */
  SCB_EnableICache();
  SCB_EnableDCache();

  /* USER CODE END 1 */
  /* USER CODE BEGIN Boot_Mode_Sequence_0 */
  int32_t timeout;
  /* USER CODE END Boot_Mode_Sequence_0 */

  /* USER CODE BEGIN Boot_Mode_Sequence_1 */
  /* Wait until CPU2 boots and enters in stop mode or timeout*/
  timeout = 0xFFFF;
  while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) != RESET) && (timeout-- > 0))
    ;
  if (timeout < 0)
  {
    Error_Handler();
  }
  /* USER CODE END Boot_Mode_Sequence_1 */
  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();
  /* USER CODE BEGIN Boot_Mode_Sequence_2 */
  /* When system initialization is finished, Cortex-M7 will release Cortex-M4 by means of
  HSEM notification */
  /*HW semaphore Clock enable*/
  __HAL_RCC_HSEM_CLK_ENABLE();
  /*Take HSEM */
  HAL_HSEM_FastTake(HSEM_ID_0);
  /*Release HSEM in order to notify the CPU2(CM4)*/
  HAL_HSEM_Release(HSEM_ID_0, 0);
  /* wait until CPU2 wakes up from stop mode */
  timeout = 0xFFFF;
  while ((__HAL_RCC_GET_FLAG(RCC_FLAG_D2CKRDY) == RESET) && (timeout-- > 0))
    ;
  if (timeout < 0)
  {
    Error_Handler();
  }
  /* USER CODE END Boot_Mode_Sequence_2 */

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  UART1_StartReceiveToIdle();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Supply configuration update enable
   */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
   */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
  {
  }

  __HAL_RCC_SYSCFG_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

  while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY))
  {
  }

  /** Initializes the RCC Oscillators according to the specified parameters
   * in the RCC_OscInitTypeDef structure.
   */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_DIV1;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 60;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_3;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
   */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2 | RCC_CLOCKTYPE_D3PCLK1 | RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef USE_FULL_ASSERT
/**
 * @brief  Reports the name of the source file and the source line number
 *         where the assert_param error has occurred.
 * @param  file: pointer to the source file name
 * @param  line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
