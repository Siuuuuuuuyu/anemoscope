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

#define SAMPLE_RATE     1600000.0f
#define TX_FREQUENCY    40000.0f

#define PAIR_START      100
#define PAIR_LENGTH     360
#define PAIR_MAX_LAG    18

#define CORR_SCALE      (1.0f / 4095.0f)
#define CORR_EPSILON    1.0e-8f
#define PAIR_SPAN       (PAIR_LENGTH + 2 * PAIR_MAX_LAG)

#define zero_bias_ac    0.0f
#define zero_bias_bd    0.0f

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
    uint16_t r_data[ADC_SAMPLES];
} CrossCorrelation_t;

typedef struct
{
    float x0;
    float x1;
    float x2;
    uint8_t initialized;
} Median3Filter_t;

typedef struct
{
    float x0;
    float x1;
    float x2;
    float x3;
    float x4;
    uint8_t initialized;
} Avrg5Filter_t;

void receive_channel_select(ReceiveChannel_t CH);
void sample_start(ChannelPin_t *CH);
void r_get_average();
void wind_speed_calculate_diagonal(float dt1, float dt2, float *v1, float *v2, float *v);
void wind_speed_calculate_quadrilateral(float dt, float *v);
void pair_corr_find_delay(CrossCorrelation_t *TtoR, CrossCorrelation_t *RtoT, float *lag);
float median_3_updata(Median3Filter_t *filter, float value);
float avrg_5_update(Avrg5Filter_t *filter, float value);

extern CrossCorrelation_t AtoC;
extern CrossCorrelation_t CtoA;
extern CrossCorrelation_t BtoD;
extern CrossCorrelation_t DtoB;

extern Median3Filter_t lag_ac_median3filter;
extern Median3Filter_t lag_bd_median3filter;
extern Avrg5Filter_t lag_ac_avrg5filter;
extern Avrg5Filter_t lag_bd_avrg5filter;

extern Median3Filter_t v_ac_median3filter;
extern Median3Filter_t v_bd_median3filter;
extern Median3Filter_t v_median3filter;
extern Avrg5Filter_t v_ac_avrg5filter;
extern Avrg5Filter_t v_bd_avrg5filter;
extern Avrg5Filter_t v_avrg5filter;

extern CrossCorrelation_t AtoB;
extern CrossCorrelation_t CtoB;
extern CrossCorrelation_t AtoD;
extern CrossCorrelation_t CtoD;
extern CrossCorrelation_t BtoA;
extern CrossCorrelation_t DtoA;
extern CrossCorrelation_t BtoC;
extern CrossCorrelation_t DtoC;

#endif //ANEMOSCOPE_WIND_SPEED_CALCULATE_H