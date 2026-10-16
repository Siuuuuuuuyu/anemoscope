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
#include "stm32l4xx_hal.h"

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
#define WIND_R_Pin GPIO_PIN_10
#define WIND_R_GPIO_Port GPIOA
#define WIND_T_Pin GPIO_PIN_9
#define WIND_T_GPIO_Port GPIOA
#define MUX_B_Pin GPIO_PIN_8
#define MUX_B_GPIO_Port GPIOA
#define CHA_A_Pin GPIO_PIN_0
#define CHA_A_GPIO_Port GPIOC
#define CHD_A_Pin GPIO_PIN_8
#define CHD_A_GPIO_Port GPIOC
#define CHB_A_Pin GPIO_PIN_2
#define CHB_A_GPIO_Port GPIOC
#define CHC_A_Pin GPIO_PIN_6
#define CHC_A_GPIO_Port GPIOC
#define MUX_A_Pin GPIO_PIN_7
#define MUX_A_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
