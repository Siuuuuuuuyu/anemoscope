//
// Created by sy on 2026/9/28.
//
#include "bsp_adc.h"

uint16_t adc_buffer[ADC_SAMPLES];
volatile uint8_t sample_done = 0;
uint16_t adc_raw_data[ADC_SAMPLES];

void bsp_adc_init()
{
    if (HAL_ADCEx_Calibration_Start(&hadc1, ADC_SINGLE_ENDED) != HAL_OK)
        Error_Handler();
}

void bsp_adc_start()
{
    if (HAL_ADC_Start_DMA(&hadc1, (uint32_t *)adc_buffer, ADC_SAMPLES) != HAL_OK)
        Error_Handler();
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1)
    {
        HAL_TIM_Base_Stop(&htim1);
        HAL_ADC_Stop_DMA(&hadc1);

        // memcpy(UART1_Tx_Data, adc_buffer, ADC_SAMPLES * sizeof(uint16_t));
        memcpy(adc_raw_data, adc_buffer, ADC_SAMPLES * sizeof(uint16_t));

        sample_done = 1;
    }
}

// void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *hadc)
// {
//     if (hadc->Instance == ADC1)
//     {
//
//     }
// }
