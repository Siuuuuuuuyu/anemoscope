//
// Created by sy on 2026/10/2.
//

#ifndef ANEMOSCOPE_BSP_GPIO_H
#define ANEMOSCOPE_BSP_GPIO_H

#include "main.h"
#include "gpio.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
} ChannelPin_t;

extern ChannelPin_t CHA;
extern ChannelPin_t CHB;
extern ChannelPin_t CHC;
extern ChannelPin_t CHD;

void bsp_gpio_init();
void bsp_gpio_set_analog(ChannelPin_t *CH);
void bsp_gpio_set_output(ChannelPin_t *CH);

#endif //ANEMOSCOPE_BSP_GPIO_H