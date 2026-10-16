//
// Created by sy on 2026/9/28.
//

#ifndef ANEMOSCOPE_BSP_ADC_H
#define ANEMOSCOPE_BSP_ADC_H

#include "main.h"
#include "adc.h"
#include "tim.h"
#include "string.h"
#include "bsp_uart.h"

#define ADC_SAMPLES 512

extern volatile uint8_t sample_done;
extern uint16_t adc_buffer[ADC_SAMPLES];
extern uint16_t adc_raw_data[ADC_SAMPLES];

void bsp_adc_init();
void bsp_adc_start();

#endif //ANEMOSCOPE_BSP_ADC_H