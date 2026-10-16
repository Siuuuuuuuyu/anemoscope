//
// Created by sy on 2026/9/28.
//
#include "bsp_dma.h"

volatile uint8_t wave_done = 0;

void bsp_dma_register_callback(DMA_HandleTypeDef *hdma)
{
    if (hdma == &hdma_tim6_up)
        HAL_DMA_RegisterCallback(&hdma_tim6_up, HAL_DMA_XFER_CPLT_CB_ID, bsp_dma_complete);
    else
        return;
}

void bsp_dma_complete(DMA_HandleTypeDef *hdma)
{
    if (hdma == &hdma_tim6_up)
    {
        __HAL_TIM_DISABLE_DMA(&htim6, TIM_DMA_UPDATE);
        HAL_TIM_Base_Stop(&htim6);

        wave_done = 1;
    }
    else
        return;
}
