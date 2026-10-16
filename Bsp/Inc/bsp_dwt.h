//
// Created by sy on 2026/9/27.
//

#ifndef ANEMOSCOPE_BSP_DWT_H
#define ANEMOSCOPE_BSP_DWT_H

#include "main.h"

typedef struct
{
    uint32_t s;
    uint16_t ms;
    uint16_t us;
} DWT_Time_t;

void DWT_Init(uint32_t CPU_Freq_mHz);
float DWT_GetDeltaT(uint32_t *cnt_last);
double DWT_GetDeltaT64(uint32_t *cnt_last);
float DWT_GetTimeline_s(void);
float DWT_GetTimeline_ms(void);
uint64_t DWT_GetTimeline_us(void);
void DWT_SysTimeUpdate(void);
void DWT_Delay(float Delay);
void DWT_Delay_ms(float Delay);
void DWT_Delay_us(float Delay);

extern DWT_Time_t SysTime;

#endif //ANEMOSCOPE_BSP_DWT_H