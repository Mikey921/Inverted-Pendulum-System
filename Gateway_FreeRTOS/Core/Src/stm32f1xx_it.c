/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f1xx_it.c
  * @brief   Interrupt Service Routines.
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
#include "stm32f1xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "modbus.h"
#include "ds18b20.h"
#include "FreeRTOS.h"
#include "semphr.h"
#include "queue.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/
extern TIM_HandleTypeDef htim2;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef htim4;

/* USER CODE BEGIN EV */
extern QueueHandle_t g_xQueue;
/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M3 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */
  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Prefetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/******************************************************************************/
/* STM32F1xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f1xx.s).                    */
/******************************************************************************/

/**
  * @brief This function handles TIM2 global interrupt.
  */
void TIM2_IRQHandler(void)
{
  /* USER CODE BEGIN TIM2_IRQn 0 */
  if (__HAL_TIM_GET_FLAG(&htim2, TIM_FLAG_UPDATE) != RESET) {
      if (__HAL_TIM_GET_IT_SOURCE(&htim2, TIM_IT_UPDATE) != RESET) {
          __HAL_TIM_CLEAR_IT(&htim2, TIM_IT_UPDATE);
          
          // 停掉定时器，省电，且防止重复触发
          htim2.Instance->CR1 &= ~TIM_CR1_CEN;

          BaseType_t xHigherPriorityTaskWoken = pdFALSE;
          // 触发信号量
          xSemaphoreGiveFromISR(g_Modbus_Rx_Sem, &xHigherPriorityTaskWoken);
          // 触发任务调度
          portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

          return;
      }
  }
  /* USER CODE END TIM2_IRQn 0 */
  HAL_TIM_IRQHandler(&htim2);
  /* USER CODE BEGIN TIM2_IRQn 1 */

  /* USER CODE END TIM2_IRQn 1 */
}

/**
  * @brief This function handles TIM4 global interrupt.
  */
void TIM4_IRQHandler(void)
{
  /* USER CODE BEGIN TIM4_IRQn 0 */

  /* USER CODE END TIM4_IRQn 0 */
  HAL_TIM_IRQHandler(&htim4);
  /* USER CODE BEGIN TIM4_IRQn 1 */

  /* USER CODE END TIM4_IRQn 1 */
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
  /* USER CODE BEGIN USART1_IRQn 0 */
  if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_ORE) != RESET) {
      __HAL_UART_CLEAR_OREFLAG(&huart1); // 清除错误标志
      huart1.Instance->DR;               // 读一次 DR 寄存器，彻底恢复硬件状态
  }
  
  // 检查是否是“RXNE”中断
  if(__HAL_UART_GET_FLAG(&huart1, UART_FLAG_RXNE) != RESET) {
    // 读取数据，同时自动清除 RXNE 标志
      uint8_t data = (uint8_t)(huart1.Instance->DR & 0xFF);

      // 只有在 IDLE 状态下，才允许把数据存入缓冲区
      if (mb_state == MB_STATE_IDLE) {
          if (rx_len < MODBUS_BUF_SIZE) {
              rx_buffer[rx_len++] = data;
          }
          // 【喂狗】收到数据了，把定时器清零，重新开始数数
          // 只要数据一直来，定时器就永远数不到 5ms，就不会触发超时
          htim2.Instance->CNT = 0;
          htim2.Instance->CR1 |= TIM_CR1_CEN;  // 确保定时器开着
      }

      return;
  }
  /* USER CODE END USART1_IRQn 0 */
  HAL_UART_IRQHandler(&huart1);
  /* USER CODE BEGIN USART1_IRQn 1 */

  /* USER CODE END USART1_IRQn 1 */
}

/**
  * @brief This function handles USART2 global interrupt.
  */
void USART2_IRQHandler(void)
{
  /* USER CODE BEGIN USART2_IRQn 0 */
  if(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_ORE) != RESET) {
      __HAL_UART_CLEAR_OREFLAG(&huart2); // 清除错误标志
      huart2.Instance->DR;               // 读一次 DR 寄存器，彻底恢复硬件状态
  }
  
  // 检查是否是“RXNE”中断
  if(__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE) != RESET) {
      uint8_t rx_byte = (uint8_t)(huart2.Instance->DR & 0xFF);

      static uint8_t state = 0;
      static uint8_t check_num = 0;
      static uint8_t rx_buf[20];
      static uint8_t buf_cnt = 0;
      static uint8_t rx_len = 0;
      static uint8_t rx_cmd = 0;

      switch (state)
      {
        case 0:
          if (rx_byte == 0xAA){ state = 1; check_num = rx_byte; }
          break;
        case 1:
          if (rx_byte == 0x55){ state = 2; check_num += rx_byte; }
          else  state = 0;
          break;
        case 2:
          rx_cmd = rx_byte;
          check_num += rx_byte;
          state = 3;
          break;
        case 3:
          rx_len = rx_byte;
          if (rx_len > sizeof(rx_buf))
          {
            state = 0;
            check_num = 0;
            break;
          }
          check_num += rx_byte;
          buf_cnt = 0;
          state = 4;
          break;
        case 4:
          rx_buf[buf_cnt++] = rx_byte;
          check_num += rx_byte;
          if (buf_cnt >= rx_len) state = 5;
          break;
        case 5:
          if (rx_byte == check_num)
          {
            if (rx_cmd == 0x04 && rx_len == 19)
            {
              Data_t rx_data;
              rx_data.angle = (rx_buf[0] << 8) | rx_buf[1];
              rx_data.location = (rx_buf[2] << 8) | rx_buf[3];
              rx_data.target_location = (rx_buf[4] << 8) | rx_buf[5];
              rx_data.angle_kp = (rx_buf[6] << 8) | rx_buf[7];
              rx_data.angle_ki = (rx_buf[8] << 8) | rx_buf[9];
              rx_data.angle_kd = (rx_buf[10] << 8) | rx_buf[11];
              rx_data.location_kp = (rx_buf[12] << 8) | rx_buf[13];
              rx_data.location_ki = (rx_buf[14] << 8) | rx_buf[15];
              rx_data.location_kd = (rx_buf[16] << 8) | rx_buf[17];
              rx_data.run_mode = rx_buf[18];

              if (g_xQueue != NULL)
              {
                BaseType_t xHigherPriorityTaskWoken = pdFALSE;
                xQueueSendFromISR(g_xQueue, &rx_data, &xHigherPriorityTaskWoken);
                portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
              }
            }
          }
          state = 0;
          break;
        default:
          state = 0;
          break;
      }
  }
  /* USER CODE END USART2_IRQn 0 */
  HAL_UART_IRQHandler(&huart2);
  /* USER CODE BEGIN USART2_IRQn 1 */

  /* USER CODE END USART2_IRQn 1 */
}

/* USER CODE BEGIN 1 */

/* USER CODE END 1 */
