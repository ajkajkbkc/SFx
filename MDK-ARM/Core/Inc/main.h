/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f1xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "iap.h"
/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SPI1_NSS_Pin GPIO_PIN_4
#define SPI1_NSS_GPIO_Port GPIOA
#define SPI2_NSS_Pin GPIO_PIN_12
#define SPI2_NSS_GPIO_Port GPIOB
#define LED_G_Pin GPIO_PIN_8
#define LED_G_GPIO_Port GPIOB
#define LED_R_Pin GPIO_PIN_9
#define LED_R_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */
/* IAP opts -------------------------------------------------------------------------------- */
/* If use this feature, In option [target] set IROM1 Start Address 0x8008000, Size 0x19000 */
#ifndef APP_IAP
#define APP_IAP                 1
#endif

/* Print log opts -------------------------------------------------------------------------- */
#ifndef PRINT_LOG_OPEN
#define PRINT_LOG_OPEN          1 // 1：打开print log功能（只有此为1，才能打印log）
#endif

#ifndef UART1_AS_LOG
#define UART1_AS_LOG            1 // 1: 通过UART1打印log, PA9,PA10
#endif

#ifndef UART2_AS_LOG
#define UART2_AS_LOG            0 // 1: 通过UART2打印log, PA2,PA3
#endif

#ifndef UART3_AS_LOG
#define UART3_AS_LOG            0 // 1: 通过UART3打印log, PC10,PC11
#endif
/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
