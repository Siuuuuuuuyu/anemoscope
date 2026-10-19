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
float t_raw_data[REF_LENGTH]; // 原始正弦模板
float t_ref_data[REF_LENGTH]; // 加窗后模板
static float weight[REF_LENGTH]; // 窗权值

static float t_energy = 0.0f;
static float w_sum = 0.0f;
static float t_mean = 0.0f;

CrossCorrelation_t AtoC = {0};
CrossCorrelation_t CtoA = {0};
CrossCorrelation_t BtoD = {0};
CrossCorrelation_t DtoB = {0};

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

// 加窗正弦模板生成
void t_ref_generate()
{
    w_sum = 0.0f;
    float weighted_sum = 0.0f;
    t_energy = 0.0f;

    for (uint8_t i = 0; i < REF_LENGTH; i ++)
    {
        t_raw_data[i] = sinf(2.0f * PI * TX_FREQUENCY * (float)i / SAMPLE_RATE);

        float phase = 2.0f * PI * (float)i / (float)(REF_LENGTH - 1);
        weight[i] = 0.5f * (1.0f - cosf(phase));

        w_sum += weight[i];
        weighted_sum += weight[i] * t_raw_data[i];
    }
    t_mean = weighted_sum / w_sum;
    for (uint8_t i = 0; i < REF_LENGTH; i ++)
    {
        t_ref_data[i] = t_raw_data[i] - t_mean;
        t_energy += weight[i] * t_ref_data[i] * t_ref_data[i];
    }
    // memcpy(UART1_Tx_Data, t_ref_data, REF_LENGTH * sizeof(float));
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
    // memcpy(UART1_Tx_Data, r_data, ADC_SAMPLES * sizeof(float));
}

// 依据模板进行归一化互相关
static float normalized_cross_correlation(uint16_t k, float *rms)
{
    float weighted_sum = 0.0f;
    for (uint8_t i = 0; i < REF_LENGTH; i ++)
        weighted_sum += weight[i] * (float)adc_raw_data[k + i];
    float r_mean = weighted_sum / w_sum;
    float numerator = 0.0f;
    float r_energy = 0.0f;
    for (uint16_t i = 0; i < REF_LENGTH; i ++)
    {
        float r = (float)adc_raw_data[k + i] - r_mean;
        float s = t_ref_data[i];
        float w = weight[i];
        numerator += w * r * s;
        r_energy += w * r * r;
    }
    // rms门限用于过滤噪声
    *rms = sqrtf(r_energy / w_sum);
    float denominator = sqrtf(r_energy * t_energy);
    if (denominator < 1.0e-6f)
        return 0.0f;
    // score用于判断波形是否符合
    return numerator / denominator;
}

// 依据模板寻找到达延时
void corr_find_delay(CrossCorrelation_t *TtoR, uint16_t search_start, uint16_t search_end, float min_rms_count, float min_score)
{
    if (search_end > ADC_SAMPLES - REF_LENGTH + 1)
        search_end = ADC_SAMPLES - REF_LENGTH + 1;
    TtoR->peak_index = 0;
    TtoR->score = 0.0f;
    TtoR->window_rms = 0.0f;
    TtoR->peak_fraction = 0.0f;
    TtoR->delay_t = 0.0f;

    for (uint16_t i = search_start; i < search_end; i ++)
    {
        float rms;
        float score = fabsf(normalized_cross_correlation(i, &rms));
        if (rms < min_rms_count)
            continue;

        if (score > min_score)
        {
            // 抛物线插值
            float rms_dummy;
            float y_left = fabsf(normalized_cross_correlation(i - 1, &rms_dummy));
            float y_center = score;
            float y_right = fabsf(normalized_cross_correlation(i + 1, &rms_dummy));

            float denominator = y_left - 2.0f * y_center + y_right;
            if (denominator < - 1.0e-6f)
            {
                float fraction = 0.5f * (y_left - y_right) / denominator;
                if (fraction >= -0.5f && fraction <= 0.5f)
                    TtoR->peak_fraction = fraction;
                else
                    TtoR->peak_fraction = 0.0f;
            }

            TtoR->peak_index = i;
            TtoR->score = score;
            TtoR->window_rms = rms;
            TtoR->delay_t = ((float)TtoR->peak_index + TtoR->peak_fraction) / SAMPLE_RATE;
            return;
        }
    }
}

// 两段接收信号进行归一化互相关
static float pair_corr(const uint16_t *x_data, const uint16_t *y_data, int lag)
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

// 两段接收信号寻找时间差
void pair_corr_find_delay(CrossCorrelation_t *TtoR, CrossCorrelation_t *RtoT, float *lag)
{
    float corr[2 * PAIR_MAX_LAG + 1];
    float best_score = -2.0f;
    int best_index = -1;
    const int corr_count = 2 * PAIR_MAX_LAG + 1;

    for (int i = 0; i < corr_count; i ++)
    {
        int k = i - PAIR_MAX_LAG;
        corr[i] = pair_corr(TtoR->r_data, RtoT->r_data, k);

        if (corr[i] > best_score)
        {
            best_score = corr[i];
            best_index = i;
        }
    }
    float fraction = 0.0f;
    if (best_index > 0 && best_index < corr_count - 1)
    {
        float y_left   = corr[best_index - 1];
        float y_center = corr[best_index];
        float y_right  = corr[best_index + 1];
        float denominator = y_left - 2.0f * y_center + y_right;
        if (denominator < -1.0e-6f)
        {
            fraction = 0.5f * (y_left - y_right) / denominator;

            // 防止插值结果跑到相邻采样点
            if (fraction < -0.5f || fraction > 0.5f)
                fraction = 0.0f;
        }
    }
    if (best_index < 0)
    {
        *lag = 0.0f;
        return;
    }
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
