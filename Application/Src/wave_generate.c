//
// Created by sy on 2026/9/27.
//
#include "wave_generate.h"

static uint32_t wave_data[WAVE_COUNT * 2];

void wave_build(ChannelPin_t *CH)
{
    for (uint16_t i = 0; i < WAVE_COUNT; i++)
    {
        wave_data[i * 2] = CH->pin;
        wave_data[i * 2 + 1] = ((uint32_t)CH->pin << 16);
    }
}

void wave_stop(ChannelPin_t *CH)
{
    __HAL_TIM_DISABLE_DMA(&htim6, TIM_DMA_UPDATE);
    HAL_TIM_Base_Stop(&htim6);
    HAL_DMA_Abort(&hdma_tim6_up);

    HAL_GPIO_WritePin(CH->port, CH->pin, GPIO_PIN_RESET);
}

void wave_start(ChannelPin_t *CH)
{
    bsp_gpio_set_output(CH);

    wave_build(CH);
    wave_stop(CH);

    HAL_DMA_Start_IT(&hdma_tim6_up, (uint32_t)wave_data, (uint32_t)&CH->port->BSRR, WAVE_COUNT * 2);
    __HAL_TIM_SET_COUNTER(&htim6, 0);
    __HAL_TIM_CLEAR_FLAG(&htim6, TIM_FLAG_UPDATE);
    __HAL_TIM_ENABLE_DMA(&htim6, TIM_DMA_UPDATE);
    HAL_TIM_Base_Start(&htim6);
}
