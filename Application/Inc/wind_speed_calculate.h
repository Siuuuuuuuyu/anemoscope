//
// Created by sy on 2026/9/28.
//

#ifndef ANEMOSCOPE_WIND_SPEED_CALCULATE_H
#define ANEMOSCOPE_WIND_SPEED_CALCULATE_H

#include "main.h"
#include "tim.h"
#include "bsp_gpio.h"
#include "bsp_adc.h"
#include "arm_math.h"

#define SAMPLE_RATE     640000.0f
#define TX_FREQUENCY    40000.0f
// #define REF_LENGTH      224 // 7 * 2 * 16
#define REF_LENGTH      224

extern float t_raw_data[REF_LENGTH];
extern float t_ref_data[REF_LENGTH];

typedef enum
{
    A = 0,
    B = 1,
    C = 2,
    D = 3
} ReceiveChannel_t;

typedef struct
{
    uint16_t peak_index;
    float peak_fraction;
    float score;
    float window_rms;
    float delay_t;
} CrossCorrelation_t;

void receive_channel_select(ReceiveChannel_t CH);
void sample_start(ChannelPin_t *CH);
void t_ref_generate();
void r_get_average();
void corr_find_delay(CrossCorrelation_t *TtoR, uint16_t search_start, uint16_t search_end, float min_rms_count, float min_score);
void wind_speed_calculate(CrossCorrelation_t *TtoR_1, CrossCorrelation_t *RtoT_1,
                            CrossCorrelation_t *TtoR_2, CrossCorrelation_t *RtoT_2,
                            float *v1, float *v2, float *v);

extern CrossCorrelation_t AtoC;
extern CrossCorrelation_t CtoA;
extern CrossCorrelation_t BtoD;
extern CrossCorrelation_t DtoB;

#endif //ANEMOSCOPE_WIND_SPEED_CALCULATE_H