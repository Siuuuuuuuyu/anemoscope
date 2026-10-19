//
// Created by sy on 2026/9/27.
//

#ifndef ANEMOSCOPE_BSP_UART_H
#define ANEMOSCOPE_BSP_UART_H

#include "main.h"
#include "usart.h"

extern uint8_t UART1_Rx_Buff[8];
extern uint8_t UART1_Tx_Data[2048];

typedef void (*UART_Callback)(uint16_t size);

typedef struct
{
    UART_HandleTypeDef *UART_Handler;
    uint8_t *Rx_Buffer;
    uint16_t Rx_Buffer_Size;
    UART_Callback Callback_Function;
}Struct_UART_Manage_Object;

void uart1_callback(uint16_t size);
void bsp_uart_init(UART_HandleTypeDef *huart, uint8_t *Rx_Buffer, uint16_t Rx_Buffer_Size, UART_Callback Callback_Function);
void bsp_uart_send(UART_HandleTypeDef *huart, uint8_t *Data, uint16_t Length);


#endif //ANEMOSCOPE_BSP_UART_H