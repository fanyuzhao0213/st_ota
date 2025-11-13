/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */
#include "stdio.h"
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
/* USER CODE BEGIN Private defines */
extern DMA_HandleTypeDef hdma_usart2_rx;
typedef struct
{
    uint16_t rp;
    uint16_t wp;
    uint8_t buf[1024];
}uart_buf_type;
extern uart_buf_type uart2;
/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);
void MX_USART2_UART_Init(void);

/* USER CODE BEGIN Prototypes */
uint16_t UART_Buf_DataCount(uart_buf_type *buf);
uint16_t UART_Buf_FreeSpace(uart_buf_type *buf);
uint8_t UART_Buf_IsEmpty(uart_buf_type *buf);
uint8_t UART_Buf_IsFull(uart_buf_type *buf);
uint8_t UART_Buf_WriteByte(uart_buf_type *buf, uint8_t data);
uint8_t UART_Buf_ReadByte(uart_buf_type *buf, uint8_t *data);
uint16_t UART_Buf_WriteBytes(uart_buf_type *buf, uint8_t *data, uint16_t len);
uint16_t UART_Buf_ReadBytes(uart_buf_type *buf, uint8_t *data, uint16_t len);
uint8_t UART_Buf_Peek(uart_buf_type *buf, uint8_t *data, uint16_t offset);
uint16_t UART_Buf_SkipBytes(uart_buf_type *buf, uint16_t len);
void UART_Buf_Clear(uart_buf_type *buf);
/* 移动写指针 */
void ringbuf_advance_head(uart_buf_type *rb, uint16_t len);
/* 移动读指针 */
void ringbuf_advance_tail(uart_buf_type *rb, uint16_t len);
void print_array_hex(const char *tag, const uint8_t *array, uint16_t len);
/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

