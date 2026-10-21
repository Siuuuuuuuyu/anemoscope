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
#include "adc.h"
#include "dma.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "bsp_dwt.h"
#include "bsp_gpio.h"
#include "bsp_dma.h"
#include "bsp_uart.h"
#include "bsp_adc.h"
#include "bsp_tim.h"
#include "wave_generate.h"
#include "wind_speed_calculate.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

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
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint32_t main_dwt_count = 0;
uint32_t main_count = 0;
float dt_main_s = 0.01f;
float main_task_time_s = 0.0f;
float main_wait_time_s = 0.0f;
uint8_t test_data[10];
uint16_t send_index = 0;
float dt_ac = 0.0f;
float dt_bd = 0.0f;
float v_ac = 0.0f;
float v_bd = 0.0f;
float v = 0.0f;
float lag_ac = 0.0f;
float lag_bd = 0.0f;
float score_ac = 0.0f;
float score_bd = 0.0f;
float lag_ac_filtered = 0.0f;
float lag_bd_filtered = 0.0f;
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  MX_TIM6_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  DWT_Init(80);
  bsp_gpio_init();
  bsp_dma_register_callback(&hdma_tim6_up);
  bsp_uart_init(&huart1, UART1_Rx_Buff, 8, uart1_callback);
  bsp_adc_init();

  test_data[0] = 0xAB;

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    DWT_GetDeltaT(&main_dwt_count);

    wave_done = 0;
    sample_done = 0;
    switch (main_count % 5)
    {
      case 0:
      {
        // A -> C
        receive_channel_select(C);
        wave_build(&CHA);
        bsp_tim_start(&htim2, &CHA, &CHC);
        while (sample_done == 0)
        {
        }
        memcpy(AtoC.r_data, adc_raw_data, sizeof(AtoC.r_data));
        // corr_find_delay(&AtoC, 30, 250, 50.0f, 0.95f);
        break;
      }
      case 1:
      {
        // C -> A
        receive_channel_select(A);
        wave_build(&CHC);
        bsp_tim_start(&htim2, &CHC, &CHA);
        while (sample_done == 0)
        {
        }
        memcpy(CtoA.r_data, adc_raw_data, sizeof(CtoA.r_data));
        // corr_find_delay(&CtoA, 30, 250, 50.0f, 0.95f);
        pair_corr_find_delay(&AtoC, &CtoA, &lag_ac, &score_ac);
        break;
      }
      case 2:
      {
        // B -> D
        receive_channel_select(D);
        wave_build(&CHB);
        bsp_tim_start(&htim2, &CHB, &CHD);
        while (sample_done == 0)
        {
        }
        memcpy(BtoD.r_data, adc_raw_data, sizeof(BtoD.r_data));
        // corr_find_delay(&BtoD, 30, 250, 50.0f, 0.95f);
        break;
      }
      case 3:
      {
        // D -> B
        receive_channel_select(B);
        wave_build(&CHD);
        bsp_tim_start(&htim2, &CHD, &CHB);
        while (sample_done == 0)
        {
        }
        memcpy(DtoB.r_data, adc_raw_data, sizeof(DtoB.r_data));
        // corr_find_delay(&DtoB, 30, 250, 50.0f, 0.95f);
        pair_corr_find_delay(&BtoD, &DtoB, &lag_bd, &score_bd);
        break;
      }
      case 4:
      {
        // 计算两个方向风速
        lag_ac_filtered = avrg_5_update(&lag_ac_filter, lag_ac);
        lag_bd_filtered = avrg_5_update(&lag_bd_filter, lag_bd);
        dt_ac = lag_ac_filtered / SAMPLE_RATE;
        dt_bd = lag_bd_filtered / SAMPLE_RATE;
        wind_speed_calculate(dt_ac, dt_bd, &v_ac, &v_bd, &v);
        for (uint8_t i = 0; i < 4; i++)
        {
          test_data[1 + i] = *((uint8_t *)&v_ac + i);
          test_data[5 + i] = *((uint8_t *)&v_bd + i);
        }
        bsp_uart_send(&huart1, test_data, 9);
        break;
      }
      default:
        break;
    }

    // wave_done = 0;
    // sample_done = 0;
    // if (main_count == 0)
    // {
    //   receive_channel_select(C);
    //   wave_build(&CHA);
    //   bsp_tim_start(&htim2, &CHA, &CHC);
    //   while (sample_done == 0)
    //   {
    //   }
    // }
    // else if (main_count == 1)
    //   r_get_average();
    // else
    // {
    //   if (send_index < 2048)
    //   {
    //     test_data[1] = UART1_Tx_Data[send_index];
    //     test_data[2] = UART1_Tx_Data[send_index + 1];
    //     test_data[3] = UART1_Tx_Data[send_index + 2];
    //     test_data[4] = UART1_Tx_Data[send_index + 3];
    //     bsp_uart_send(&huart1, test_data, 5);
    //     send_index += 4;
    //   }
    //   // test_data[1] = UART1_Tx_Data[0];
    //   // test_data[2] = UART1_Tx_Data[1];
    //   // bsp_uart_send(&huart1, test_data, 3);
    // }

    // receive_channel_select(C);
    // wave_start(&CHA);
    // while (wave_done == 0)
    // {
    // }
    // sample_start(&CHC);
    // while (sample_done == 0)
    // {
    // }
    // corr_find(&AtoC, 30, 200, 30.0f, 0.9f);

    // for (uint8_t i = 0; i < 4; i++)
    //   test_data[1 + i] = *((uint8_t *)&delay_t_us + i);
    // bsp_uart_send(&huart1, test_data, 5);

    // test_data[1] = (uint8_t)(AtoC.peak_index & 0xFF);
    // test_data[2] = (uint8_t)(AtoC.peak_index >> 8);
    // bsp_uart_send(&huart1, test_data, 3);

    main_task_time_s = DWT_GetDeltaT(&main_dwt_count);
    main_wait_time_s = dt_main_s - main_task_time_s;
    main_count ++;
    DWT_Delay(main_wait_time_s);
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

  /** Configure the main internal regulator output voltage
  */
  if (HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 1;
  RCC_OscInitStruct.PLL.PLLN = 10;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

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
