//
// Created by sy on 2026/9/28.
//

#ifndef ANEMOSCOPE_BSP_DMA_H
#define ANEMOSCOPE_BSP_DMA_H

#include "main.h"
#include "dma.h"
#include "tim.h"
#include "bsp_gpio.h"

extern TIM_HandleTypeDef htim6;
extern DMA_HandleTypeDef hdma_tim6_up;
extern volatile uint8_t wave_done;

void bsp_dma_register_callback(DMA_HandleTypeDef *hdma);
void bsp_dma_complete(DMA_HandleTypeDef *hdma);

#endif //ANEMOSCOPE_BSP_DMA_H