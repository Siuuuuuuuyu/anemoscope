//
// Created by sy on 2026/10/21.
//

#ifndef ANEMOSCOPE_BSP_TIM_H
#define ANEMOSCOPE_BSP_TIM_H

#include "main.h"
#include "tim.h"
#include "wave_generate.h"
#include "wind_speed_calculate.h"

#define WAVE_START_TICKS    20
#define SAMPLE_START_TICKS  120

void bsp_tim_start(TIM_HandleTypeDef *htim, ChannelPin_t *tx, ChannelPin_t *rx);

#endif //ANEMOSCOPE_BSP_TIM_H