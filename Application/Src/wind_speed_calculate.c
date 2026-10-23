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

static float corr_x_centered[PAIR_LENGTH];
static float corr_y_sum[PAIR_SPAN + 1];
static float corr_y_sq_sum[PAIR_SPAN + 1];

CrossCorrelation_t AtoC = {0};
CrossCorrelation_t CtoA = {0};
CrossCorrelation_t BtoD = {0};
CrossCorrelation_t DtoB = {0};

Median3Filter_t lag_ac_median3filter = {0};
Median3Filter_t lag_bd_median3filter = {0};
Avrg5Filter_t lag_ac_avrg5filter = {0};
Avrg5Filter_t lag_bd_avrg5filter = {0};

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
            if (fraction < -0.2f || fraction > 0.2f)
                return 0.0f;
            return fraction;
        }
        return 0.0f;
    }
    return 0.0f;
}

// 3点中值
float median_3_updata(Median3Filter_t *filter, float value)
{
    if (!filter->initialized)
    {
        filter->x0 = value;
        filter->x1 = value;
        filter->x2 = value;
        filter->initialized = 1;
        return value;
    }
    filter->x0 = filter->x1;
    filter->x1 = filter->x2;
    filter->x2 = value;
    float min_value = fminf(filter->x0, fminf(filter->x1, filter->x2));
    float max_value = fmaxf(filter->x0, fmaxf(filter->x1, filter->x2));
    return filter->x0 + filter->x1 + filter->x2 - min_value - max_value;
}

// 5点平均
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

static void pair_corr_prepare(const uint16_t *x_data, const uint16_t *y_data, float *x_energy)
{
    const int x_start = PAIR_START;
    const int y_start = PAIR_START - PAIR_MAX_LAG;

    float x_sum = 0.0f;
    for (int i = 0; i < PAIR_LENGTH; i++)
    {
        float x = (float)x_data[x_start + i] * CORR_SCALE;
        x_sum += x;
    }
    float x_mean = x_sum / (float)PAIR_LENGTH;

    float xx = 0.0f;
    for (int i = 0; i < PAIR_LENGTH; i++)
    {
        float x = (float)x_data[x_start + i] * CORR_SCALE;
        float xc = x - x_mean;
        corr_x_centered[i] = xc;
        xx += xc * xc;
    }
    *x_energy = xx;

    corr_y_sum[0] = 0.0f;
    corr_y_sq_sum[0] = 0.0f;
    for (int i = 0; i < PAIR_SPAN; i++)
    {
        float y = (float)y_data[y_start + i] * CORR_SCALE;
        corr_y_sum[i + 1] = corr_y_sum[i] + y;
        corr_y_sq_sum[i + 1] = corr_y_sq_sum[i] + y * y;
    }
}

// 两段接收信号进行归一化互相关
static float pair_normalized_cross_corr(const uint16_t *y_data, int lag, float x_energy)
{
    const int y_base = PAIR_START - PAIR_MAX_LAG;
    const int y_offset = lag + PAIR_MAX_LAG;

    float y_sum = corr_y_sum[y_offset + PAIR_LENGTH] - corr_y_sum[y_offset];
    float y_sq_sum = corr_y_sq_sum[y_offset + PAIR_LENGTH] - corr_y_sq_sum[y_offset];

    float y_mean = y_sum / (float)PAIR_LENGTH;

    float yy =
        y_sq_sum - y_sum * y_mean;

    if (yy <= CORR_EPSILON || x_energy <= CORR_EPSILON)
        return -2.0f;

    float xy = 0.0f;
    for (int i = 0; i < PAIR_LENGTH; i++)
    {
        float y = (float)y_data[y_base + y_offset + i] * CORR_SCALE;
        xy += corr_x_centered[i] * (y - y_mean);
    }

    float denominator = sqrtf(x_energy * yy);

    if (denominator <= CORR_EPSILON)
        return -2.0f;

    return xy / denominator;
}

// 两段接收信号寻找时间差
void pair_corr_find_delay(CrossCorrelation_t *TtoR, CrossCorrelation_t *RtoT, float *lag)
{
    float corr[2 * PAIR_MAX_LAG + 1];
    float best_score = -2.0f;
    int best_index = -1;
    const int corr_count = 2 * PAIR_MAX_LAG + 1;
    float x_energy = 0.0f;

    pair_corr_prepare(TtoR->r_data, RtoT->r_data, &x_energy);

    for (int i = 0; i < corr_count; i ++)
    {
        int k = i - PAIR_MAX_LAG;
        corr[i] = pair_normalized_cross_corr(RtoT->r_data, k, x_energy);

        if (corr[i] > best_score)
        {
            best_score = corr[i];
            best_index = i;
        }
    }
    if (best_index < 0)
    {
        *lag = 0.0f;
        return;
    }
    // float fraction = 0.0f;
    float fraction = parabolic_interpolation_3(corr, best_index, corr_count);
    int best_lag = best_index - PAIR_MAX_LAG;
    *lag = (float)best_lag + fraction;
}

// 风速计算
void wind_speed_calculate(float dt1, float dt2, float *v1, float *v2, float *v)
{
    *v1 = (c2 * dt1) / (d + sqrtf(d2 + c2 * dt1 * dt1));
    *v2 = (c2 * dt2) / (d + sqrtf(d2 + c2 * dt2 * dt2));
    *v = sqrtf(*v1 * *v1 + *v2 * *v2);
}
