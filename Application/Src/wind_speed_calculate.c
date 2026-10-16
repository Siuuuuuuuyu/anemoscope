//
// Created by sy on 2026/9/28.
//
#include "wind_speed_calculate.h"

static const float d = 0.025f;
static const float d2 = 0.000625f;
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

// 归一化互相关
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
    *rms = sqrtf(r_energy / w_sum);
    float denominator = sqrtf(r_energy * t_energy);
    if (denominator < 1.0e-6f)
        return 0.0f;
    return numerator / denominator;
}

// 寻找到达延时
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

// 风速计算
void wind_speed_calculate(CrossCorrelation_t *TtoR_1, CrossCorrelation_t *RtoT_1,
                            CrossCorrelation_t *TtoR_2, CrossCorrelation_t *RtoT_2,
                            float *v1, float *v2, float *v)
{
    float dt1 = RtoT_1->delay_t - TtoR_1->delay_t;
    float dt2 = RtoT_2->delay_t - TtoR_2->delay_t;

    *v1 = (c2 * dt1) / (d + sqrtf(d2 + c2 * dt1 * dt1));
    *v2 = (c2 * dt2) / (d + sqrtf(d2 + c2 * dt2 * dt2));
    *v = sqrtf(*v1 * *v1 + *v2 * *v2);
}
