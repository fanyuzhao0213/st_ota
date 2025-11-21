/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
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
/* Includes ------------------------------------------------------------------*/
#include "usart.h"

/* USER CODE BEGIN 0 */
uart_buf_type uart2;
/* USER CODE END 0 */

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
DMA_HandleTypeDef hdma_usart2_rx;

/* USART1 init function */

void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}
/* USART2 init function */

void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */
  __HAL_UART_ENABLE_IT(&huart2, UART_IT_IDLE);//开启空闲中断
  HAL_UARTEx_ReceiveToIdle_DMA(&huart2,uart2.buf,1024); //通过DMA接收1024字节 并打开空闲中断  
  /* USER CODE END USART2_Init 2 */

}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_9;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_10;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* USER CODE BEGIN USART1_MspInit 1 */
  /* USER CODE END USART1_MspInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspInit 0 */

  /* USER CODE END USART2_MspInit 0 */
    /* USART2 clock enable */
    __HAL_RCC_USART2_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_2;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART2 DMA Init */
    /* USART2_RX Init */
    hdma_usart2_rx.Instance = DMA1_Channel6;
    hdma_usart2_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart2_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart2_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart2_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart2_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart2_rx.Init.Mode = DMA_CIRCULAR;
    hdma_usart2_rx.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_usart2_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart2_rx);

    /* USART2 interrupt Init */
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspInit 1 */

  /* USER CODE END USART2_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA9     ------> USART1_TX
    PA10     ------> USART1_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_9|GPIO_PIN_10);

  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
  else if(uartHandle->Instance==USART2)
  {
  /* USER CODE BEGIN USART2_MspDeInit 0 */

  /* USER CODE END USART2_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART2_CLK_DISABLE();

    /**USART2 GPIO Configuration
    PA2     ------> USART2_TX
    PA3     ------> USART2_RX
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_2|GPIO_PIN_3);

    /* USART2 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART2 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART2_IRQn);
  /* USER CODE BEGIN USART2_MspDeInit 1 */

  /* USER CODE END USART2_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
/**
 * @brief 获取缓冲区中可读数据个数
 * @param buf 缓冲区指针
 * @return 可读数据个数
 */
uint16_t UART_Buf_DataCount(uart_buf_type *buf)
{
    if (buf->wp >= buf->rp) {
        return buf->wp - buf->rp;
    } else {
        return (sizeof(buf->buf) - buf->rp) + buf->wp;
    }
}

/**
 * @brief 获取缓冲区剩余空间
 * @param buf 缓冲区指针
 * @return 剩余空间大小
 */
uint16_t UART_Buf_FreeSpace(uart_buf_type *buf)
{
    return sizeof(buf->buf) - UART_Buf_DataCount(buf) - 1; // 留一个字节区分空和满
}

/**
 * @brief 检查缓冲区是否为空
 * @param buf 缓冲区指针
 * @return 1-空, 0-非空
 */
uint8_t UART_Buf_IsEmpty(uart_buf_type *buf)
{
    return (buf->rp == buf->wp);
}

/**
 * @brief 检查缓冲区是否已满
 * @param buf 缓冲区指针
 * @return 1-满, 0-未满
 */
uint8_t UART_Buf_IsFull(uart_buf_type *buf)
{
    return (UART_Buf_FreeSpace(buf) == 0);
}

/**
 * @brief 写入一个字节到缓冲区
 * @param buf 缓冲区指针
 * @param data 要写入的数据
 * @return 1-成功, 0-缓冲区已满
 */
uint8_t UART_Buf_WriteByte(uart_buf_type *buf, uint8_t data)
{
    if (UART_Buf_IsFull(buf)) {
        return 0; // 缓冲区满，写入失败
    }
    
    buf->buf[buf->wp] = data;
    buf->wp = (buf->wp + 1) % sizeof(buf->buf);
    return 1;
}

/**
 * @brief 从缓冲区读取一个字节
 * @param buf 缓冲区指针
 * @param data 读取的数据存放地址
 * @return 1-成功, 0-缓冲区为空
 */
uint8_t UART_Buf_ReadByte(uart_buf_type *buf, uint8_t *data)
{
    if (UART_Buf_IsEmpty(buf)) {
        return 0; // 缓冲区空，读取失败
    }
    
    *data = buf->buf[buf->rp];
    buf->rp = (buf->rp + 1) % sizeof(buf->buf);
    return 1;
}

/**
 * @brief 写入多个字节到缓冲区
 * @param buf 缓冲区指针
 * @param data 要写入的数据数组
 * @param len 要写入的数据长度
 * @return 实际写入的数据长度
 */
uint16_t UART_Buf_WriteBytes(uart_buf_type *buf, uint8_t *data, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        if (!UART_Buf_WriteByte(buf, data[i])) {
            break; // 缓冲区满，停止写入
        }
    }
    return i; // 返回实际写入的字节数
}

/**
 * @brief 从缓冲区读取多个字节
 * @param buf 缓冲区指针
 * @param data 读取的数据存放数组
 * @param len 要读取的数据长度
 * @return 实际读取的数据长度
 */
uint16_t UART_Buf_ReadBytes(uart_buf_type *buf, uint8_t *data, uint16_t len)
{
    uint16_t i;
    for (i = 0; i < len; i++) {
        if (!UART_Buf_ReadByte(buf, &data[i])) {
            break; // 缓冲区空，停止读取
        }
    }
    return i; // 返回实际读取的字节数
}

/**
 * @brief 查看缓冲区数据但不移动读指针
 * @param buf 缓冲区指针
 * @param data 数据存放地址
 * @param offset 查看的偏移量(0表示第一个可读字节)
 * @return 1-成功, 0-失败(偏移量超出数据范围)
 */
uint8_t UART_Buf_Peek(uart_buf_type *buf, uint8_t *data, uint16_t offset)
{
    if (offset >= UART_Buf_DataCount(buf)) {
        return 0; // 偏移量超出范围
    }
    
    uint16_t index = (buf->rp + offset) % sizeof(buf->buf);
    *data = buf->buf[index];
    return 1;
}

/**
 * @brief 丢弃指定数量的数据
 * @param buf 缓冲区指针
 * @param len 要丢弃的数据长度
 * @return 实际丢弃的数据长度
 */
uint16_t UART_Buf_SkipBytes(uart_buf_type *buf, uint16_t len)
{
    uint16_t available = UART_Buf_DataCount(buf);
    uint16_t skip_len = (len > available) ? available : len;
    
    buf->rp = (buf->rp + skip_len) % sizeof(buf->buf);
    return skip_len;
}

/**
 * @brief 清空缓冲区
 * @param buf 缓冲区指针
 */
void UART_Buf_Clear(uart_buf_type *buf)
{
    buf->rp = 0;
    buf->wp = 0;
}

/* ===================== 内部工具 ===================== */
/* 移动写指针 */
void ringbuf_advance_head(uart_buf_type *rb, uint16_t len)
{
    rb->wp = (rb->wp + len) % sizeof(rb->buf);
}
/* 移动读指针 */
void ringbuf_advance_tail(uart_buf_type *rb, uint16_t len)
{
    rb->rp = (rb->rp + len) % sizeof(rb->buf);
}

void print_array_hex(const char *tag, const uint8_t *array, uint16_t len)
{
    if (array == NULL || len == 0) {
        return;
    }
    
    printf("[%s] :[%d]\r\n", tag, len);
	printf("======START=====\r\n");
	#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (uint16_t i = 0; i < len; i++) {
        printf("%02X ", array[i]);
    }
	printf("\r\n");
	#endif 
    printf("======END=====\r\n");
}

void print_string(const char *tag, const uint8_t *array, uint16_t len)
{
    if (array == NULL || len == 0) {
        return;
    }
    
    printf("[%s] :[%d]\r\n", tag, len);
	printf("======START=====\r\n");
	#if LOG_LEVEL >= LOG_LEVEL_DEBUG
    for (uint16_t i = 0; i < len; i++) {
        printf("%s ", (char*)array[i]);
    }
	printf("\r\n");
	#endif 
    printf("======END=====\r\n");
}


/**
  * @brief  通过UART2发送字符串（带超时）
  * @param  data: 要发送的字符串指针
  * @param  len: 要发送的数据长度
  * @param  timeout: 超时时间(ms)
  * @retval HAL状态
  */
HAL_StatusTypeDef UART2_SendString_Timeout(const char *data, uint16_t len, uint32_t timeout)
{
    if (data == NULL || len == 0) {
        return HAL_ERROR;
    }
	printf("ESP8266 send: \r\n");
	for(uint16_t i = 0; i < len; i++) 
	{
		printf("%c",data[i]);
	}
	printf("\r\n");
    return HAL_UART_Transmit(&huart2, (uint8_t *)data, len, timeout);
}


int fputc(int ch, FILE *f) {
	HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
return ch;
}
/* USER CODE END 1 */
