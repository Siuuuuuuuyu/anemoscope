//
// Created by sy on 2026/9/28.
//
#include "wind_speed_calculate.h"

// 25.455844122715710878430397035775mm
static const float d = 0.025455844122716f;
static const float d2 = 0.000648f;
// static const float h = 0.010f;
// static const float L = 0.03201562118716f;
static const float c2 = 120604.787524f;

float r_data[ADC_SAMPLES];

CrossCorrelation_t AtoC = {0};
CrossCorrelation_t CtoA = {0};
CrossCorrelation_t BtoD = {0};
CrossCorrelation_t DtoB = {0};

Avrg5Filter_t lag_ac_filter = {0};
Avrg5Filter_t lag_bd_filter = {0};

void receive_channel_select(ReceiveChannel_t CH)
{
    GPIO_PinState mux_a_state = (CH & 0x01) ? GPIO_PIN_SET : GPIO_PIN_RESET;
    GPIO_PinState mux_b_state = (CH & 0x02) ? GPIO_PIN_SET : GPIO_PIN_RESET;

    HAL_GPIO_WritePin(MUX_A_GPIO_Port, MUX_A_Pin, mux_a_state);
    HAL_GPIO_WritePin(MUX_B_GPIO_Port, MUX_B_Pin, mux_b_state);
}

// 开始采样
void sample_start(ChannelPin_t *CH)
{
    bsp_gpio_set_analog(CH);
    bsp_adc_start();
    __HAL_TIM_SET_COUNTER(&htim1, 0);
    __HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
    HAL_TIM_Base_Start(&htim1);
}

// 接收波形处理
void r_get_average()
{
    float r_sum = 0;
    for (uint16_t i = 0; i < ADC_SAMPLES; i ++)
        r_sum += (float)adc_raw_data[i];

    float r_avrg = r_sum / ADC_SAMPLES;
    for (uint16_t i = 0; i < ADC_SAMPLES; i ++)
        r_data[i] = (float)adc_raw_data[i] - r_avrg;
    memcpy(UART1_Tx_Data, r_data, ADC_SAMPLES * sizeof(float));
}

// 两段接收信号进行归一化互相关
static float pair_normalized_cross_corr(const uint16_t *x_data, const uint16_t *y_data, int lag)
{
    float sum_x = 0.0f;
    float sum_y = 0.0f;
    for (int i = 0; i < PAIR_LENGTH; i ++)
    {
        sum_x += (float)x_data[PAIR_START + i];
        sum_y += (float)y_data[PAIR_START + i + lag];
    }
    const float mean_x = sum_x / PAIR_LENGTH;
    const float mean_y = sum_y / PAIR_LENGTH;

    float xx = 0.0f;
    float yy = 0.0f;
    float xy = 0.0f;
    for (int i = 0; i < PAIR_LENGTH; i ++)
    {
        const float a = (float)x_data[PAIR_START + i] - mean_x;
        const float b = (float)y_data[PAIR_START + i + lag] - mean_y;

        xy += a * b;
        xx += a * a;
        yy += b * b;
    }
    float denominator = sqrtf(xx * yy);

    if (denominator < 1.0e-6f)
        return -2.0f;
    return xy / denominator;
}

// 3点抛物线插值
static float parabolic_interpolation_3(const float *corr, int best_index, int corr_count)
{
    if (best_index > 0 && best_index < corr_count - 1)
    {
        float y_left   = corr[best_index - 1];
        float y_center = corr[best_index];
        float y_right  = corr[best_index + 1];
        float denominator = y_left - 2.0f * y_center + y_right;
        if (denominator < -1.0e-6f)
        {
            float fraction = 0.5f * (y_left - y_right) / denominator;
            if (fraction < -0.5f || fraction > 0.5f)
                return 0.0f;
            return fraction;
        }
        return 0.0f;
    }
    return 0.0f;
}

float avrg_5_update(Avrg5Filter_t *filter, float value)
{
    if (!filter->initialized)
    {
        filter->x0 = value;
        filter->x1 = value;
        filter->x2 = value;
        filter->x3 = value;
        filter->x4 = value;
        filter->initialized = 1;

        return value;
    }

    filter->x0 = filter->x1;
    filter->x1 = filter->x2;
    filter->x2 = filter->x3;
    filter->x3 = filter->x4;
    filter->x4 = value;

    float avrg = (filter->x0 + filter->x1 + filter->x2 + filter->x3 + filter->x4) / 5.0f;
    return avrg;
}

// 两段接收信号寻找时间差
void pair_corr_find_delay(CrossCorrelation_t *TtoR, CrossCorrelation_t *RtoT, float *lag, float *score)
{
    float corr[2 * PAIR_MAX_LAG + 1];
    float best_score = -2.0f;
    int best_index = -1;
    const int corr_count = 2 * PAIR_MAX_LAG + 1;

    for (int i = 0; i < corr_count; i ++)
    {
        int k = i - PAIR_MAX_LAG;
        corr[i] = pair_normalized_cross_corr(TtoR->r_data, RtoT->r_data, k);

        if (corr[i] > best_score)
        {
            best_score = corr[i];
            best_index = i;
        }
    }
    float fraction = parabolic_interpolation_3(corr, best_index, corr_count);

    if (best_index < 0)
    {
        *lag = 0.0f;
        return;
    }
    int best_lag = best_index - PAIR_MAX_LAG;
    *lag = (float)best_lag + fraction;
    *score = best_score;
}

// 风速计算
void wind_speed_calculate(float dt1, float dt2, float *v1, float *v2, float *v)
{
    *v1 = (c2 * dt1) / (d + sqrtf(d2 + c2 * dt1 * dt1));
    *v2 = (c2 * dt2) / (d + sqrtf(d2 + c2 * dt2 * dt2));
    *v = sqrtf(*v1 * *v1 + *v2 * *v2);
}
