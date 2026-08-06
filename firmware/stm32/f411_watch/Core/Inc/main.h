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
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

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
#define POWER_EN_Pin GPIO_PIN_13
#define POWER_EN_GPIO_Port GPIOC
#define WDI_Pin GPIO_PIN_0
#define WDI_GPIO_Port GPIOA
#define WDOG_EN_Pin GPIO_PIN_1
#define WDOG_EN_GPIO_Port GPIOA
#define TP_RST_Pin GPIO_PIN_2
#define TP_RST_GPIO_Port GPIOA
#define TP_SDA_Pin GPIO_PIN_3
#define TP_SDA_GPIO_Port GPIOA
#define TP_SCL_Pin GPIO_PIN_4
#define TP_SCL_GPIO_Port GPIOA
#define LCD_SCK_Pin GPIO_PIN_5
#define LCD_SCK_GPIO_Port GPIOA
#define LCD_RST_Pin GPIO_PIN_6
#define LCD_RST_GPIO_Port GPIOA
#define LCD_MOSI_Pin GPIO_PIN_7
#define LCD_MOSI_GPIO_Port GPIOA
#define LCD_CS_Pin GPIO_PIN_0
#define LCD_CS_GPIO_Port GPIOB
#define LCD_DC_Pin GPIO_PIN_1
#define LCD_DC_GPIO_Port GPIOB
#define ENCODER_KEY_Pin GPIO_PIN_2
#define ENCODER_KEY_GPIO_Port GPIOB
#define ENCODER_KEY_EXTI_IRQn EXTI2_IRQn
#define KEY_BACK_Pin GPIO_PIN_10
#define KEY_BACK_GPIO_Port GPIOB
#define KEY_BACK_EXTI_IRQn EXTI15_10_IRQn
#define KEY_WAKE_Pin GPIO_PIN_12
#define KEY_WAKE_GPIO_Port GPIOB
#define KEY_WAKE_EXTI_IRQn EXTI15_10_IRQn
#define LCD_BLK_Pin GPIO_PIN_13
#define LCD_BLK_GPIO_Port GPIOB
#define BLE_EN_Pin GPIO_PIN_14
#define BLE_EN_GPIO_Port GPIOB
#define LSM06_INT2_Pin GPIO_PIN_15
#define LSM06_INT2_GPIO_Port GPIOB
#define LSM06_INT2_EXTI_IRQn EXTI15_10_IRQn
#define LSM06_INT1_Pin GPIO_PIN_8
#define LSM06_INT1_GPIO_Port GPIOA
#define LSM06_INT1_EXTI_IRQn EXTI9_5_IRQn
#define BLE_TX_Pin GPIO_PIN_9
#define BLE_TX_GPIO_Port GPIOA
#define BLE_RX_Pin GPIO_PIN_10
#define BLE_RX_GPIO_Port GPIOA
#define W25Q128_CS_Pin GPIO_PIN_15
#define W25Q128_CS_GPIO_Port GPIOA
#define W25Q128_SCK_Pin GPIO_PIN_3
#define W25Q128_SCK_GPIO_Port GPIOB
#define W25Q128_MISO_Pin GPIO_PIN_4
#define W25Q128_MISO_GPIO_Port GPIOB
#define W25Q128_MOSI_Pin GPIO_PIN_5
#define W25Q128_MOSI_GPIO_Port GPIOB
#define ENCODER_B_Pin GPIO_PIN_6
#define ENCODER_B_GPIO_Port GPIOB
#define ENCODER_A_Pin GPIO_PIN_7
#define ENCODER_A_GPIO_Port GPIOB
#define SENSOR_SCL_Pin GPIO_PIN_8
#define SENSOR_SCL_GPIO_Port GPIOB
#define SENSOR_SDA_Pin GPIO_PIN_9
#define SENSOR_SDA_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
