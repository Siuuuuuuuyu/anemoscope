//
// Created by sy on 2026/9/27.
//

#ifndef ANEMOSCOPE_WAVE_GENERATE_H
#define ANEMOSCOPE_WAVE_GENERATE_H

#include "main.h"
#include "bsp_dwt.h"
#include "bsp_gpio.h"
#include "bsp_adc.h"

#define WAVE_COUNT 3

extern TIM_HandleTypeDef htim1;
extern TIM_HandleTypeDef htim6;
extern DMA_HandleTypeDef hdma_tim6_up;

void wave_build(ChannelPin_t *CH);
void wave_start(ChannelPin_t *CH);

#endif //ANEMOSCOPE_WAVE_GENERATE_H