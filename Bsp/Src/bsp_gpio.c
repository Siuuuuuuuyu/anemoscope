//
// Created by sy on 2026/10/2.
//
#include "bsp_gpio.h"

ChannelPin_t CHA;
ChannelPin_t CHB;
ChannelPin_t CHC;
ChannelPin_t CHD;

void bsp_gpio_init()
{
    CHA.port = CHA_A_GPIO_Port;
    CHA.pin = CHA_A_Pin;
    CHB.port = CHB_A_GPIO_Port;
    CHB.pin = CHB_A_Pin;
    CHC.port = CHC_A_GPIO_Port;
    CHC.pin = CHC_A_Pin;
    CHD.port = CHD_A_GPIO_Port;
    CHD.pin = CHD_A_Pin;

    bsp_gpio_set_analog(&CHA);
    bsp_gpio_set_analog(&CHB);
    bsp_gpio_set_analog(&CHC);
    bsp_gpio_set_analog(&CHD);
}

void bsp_gpio_set_analog(ChannelPin_t *CH)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = CH->pin;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(CH->port, &gpio);
}

void bsp_gpio_set_output(ChannelPin_t *CH)
{
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = CH->pin;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(CH->port, &gpio);
}
