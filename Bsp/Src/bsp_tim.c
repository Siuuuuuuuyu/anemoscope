//
// Created by sy on 2026/10/21.
//
#include "bsp_tim.h"

static ChannelPin_t *tx_channel;
static ChannelPin_t *rx_channel;

void bsp_tim_start(TIM_HandleTypeDef *htim, ChannelPin_t *tx, ChannelPin_t *rx)
{
    tx_channel = tx;
    rx_channel = rx;
    if (htim->Instance == TIM2)
    {
        __HAL_TIM_SET_COUNTER(&htim2, 0);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, WAVE_START_TICKS);
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, SAMPLE_START_TICKS);
        __HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE | TIM_FLAG_CC1 | TIM_FLAG_CC2);

        HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_1);
        HAL_TIM_OC_Start_IT(&htim2, TIM_CHANNEL_2);
    }
    else
        return;
}

void HAL_TIM_OC_DelayElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM2)
        return;

    if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_1)
    {
        HAL_TIM_OC_Stop_IT(htim, TIM_CHANNEL_1);
        wave_start(tx_channel);
    }
    else if (htim->Channel == HAL_TIM_ACTIVE_CHANNEL_2)
    {
        HAL_TIM_OC_Stop_IT(htim, TIM_CHANNEL_1);
        HAL_TIM_OC_Stop_IT(htim, TIM_CHANNEL_2);
        sample_start(rx_channel);
    }
}