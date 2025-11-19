/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "dma.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "delay.h"
#include "W25Q128.h"
#include "flash.h"
#include "boot.h"
#include "string.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
#define version      "VER-1.1.0-2025-11-15-10.59"
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
// 获取芯片信息的函数
void Print_System_Info(void)
{
    // 打印系统启动信息
    printf("========================================\r\n");
    printf("====[BOOT] System Startup Successful!====\r\n");
    printf("========================================\r\n");
}

/**
  * @brief  验证Flash中写入的固件数据是否正确
  * @param  flash_addr: Flash中的起始地址
  * @param  w25q64_addr: W25Q64中的起始地址  
  * @param  length: 数据长度
  * @retval HAL状态
  */
HAL_StatusTypeDef Verify_Firmware_Data(uint32_t flash_addr, uint32_t w25q64_addr, uint32_t length)
{
    uint8_t flash_buf[256];
    uint8_t w25q64_buf[256];
    uint32_t verified = 0;
    uint32_t error_count = 0;
    
    printf("验证数据中...\r\n");
    
    while(verified < length)
    {
        uint32_t chunk_size = (length - verified) > 256 ? 256 : (length - verified);
        
        // 从Flash读取
        memcpy(flash_buf, (void*)(flash_addr + verified), chunk_size);
        
        // 从W25Q64读取
        W25Q128_ReadData(w25q64_addr + verified, w25q64_buf, chunk_size);
        
        // 比较数据
        if(memcmp(flash_buf, w25q64_buf, chunk_size) != 0)
        {
            error_count++;
            if(error_count <= 3)  // 只显示前3个错误
            {
                printf("验证错误 at offset 0x%08X\r\n", verified);
            }
        }
        
        verified += chunk_size;
        
        // 进度显示
        if((verified % 4096) == 0)  // 每4KB显示一次进度
        {
            printf("验证进度：%d/%d字节\r\n", verified, length);
        }
    }
    
    if(error_count > 0)
    {
        printf("数据验证失败：发现%d处错误\r\n", error_count);
        return HAL_ERROR;
    }
    
    return HAL_OK;
}


void my_update_fireware_ota(void)
{
	if(BootStateFlag & UPDATA_A_FLAG)
	{
		// 1. 参数准备和验证
		uint32_t firmware_len = OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num];
		uint32_t w25q64_base_addr = UpdateA.Updata_A_from_W25Q64_Num * 64 * 1024;  // 64KB块偏移
		uint32_t complete_pages, remaining_bytes;
		
		complete_pages = firmware_len / STM32_PAGE_SIZE;
		remaining_bytes = firmware_len % STM32_PAGE_SIZE;
		
		printf("本次需要更新的大小：%d字节\r\n", firmware_len);
		printf("本次需要更新的页数：%d页\r\n", complete_pages);
		printf("本次需要剩余更新的大小：%d字节\r\n", remaining_bytes);
		
		// 2. 固件长度验证
		if((firmware_len % 4) != 0)
		{
			printf("错误：固件长度必须是4字节对齐！当前长度：%d\r\n", firmware_len);
			BootStateFlag &= ~UPDATA_A_FLAG;
			return;
		}
		
		if(firmware_len == 0 || firmware_len > (FLASH_END_ADDR - STM32_A_START_ADDR))
		{
			printf("错误：固件长度无效或超出Flash范围！\r\n");
			BootStateFlag &= ~UPDATA_A_FLAG;
			return;
		}
		
		STM32_EraseFromPageToEnd(20);
		printf("Flash擦除完成\r\n");
		
		// 5. 写入完整页数据
		if(complete_pages > 0)
		{
			printf("开始写入完整页数据...\r\n");
			for(uint32_t i = 0; i < complete_pages; i++)
			{
				// 清零缓冲区
				memset(UpdateA.Updata_A_Buff, 0, STM32_PAGE_SIZE);
				
				// 从W25Q64读取
				W25Q128_ReadData(w25q64_base_addr + i * STM32_PAGE_SIZE, 
								UpdateA.Updata_A_Buff, STM32_PAGE_SIZE);
				
				// 写入Flash
				if(FLASH_Write(STM32_A_START_ADDR + i * STM32_PAGE_SIZE,
							  (uint8_t *)UpdateA.Updata_A_Buff, 
							  STM32_PAGE_SIZE) != HAL_OK)
				{
					printf("错误：第%d页写入失败！\r\n", i);
					BootStateFlag &= ~UPDATA_A_FLAG;
					return;
				}
				
				// 进度显示
				if((i % 16) == 0)  // 每16页显示一次进度
				{
					printf("写入进度：%d/%d字节\r", 
						  (i + 1) * STM32_PAGE_SIZE, firmware_len);
				}
			}
			printf("完整页数据写入完成：%d页\r\n", complete_pages);
		}
		
		// 6. 写入剩余数据
		if(remaining_bytes > 0)
		{
			printf("开始写入剩余数据：%d字节\r\n", remaining_bytes);
			
			// 清零缓冲区
			memset(UpdateA.Updata_A_Buff, 0, STM32_PAGE_SIZE);
			
			// 从W25Q64读取剩余数据
			W25Q128_ReadData(w25q64_base_addr + complete_pages * STM32_PAGE_SIZE,
							UpdateA.Updata_A_Buff, remaining_bytes);
			
			// 写入Flash
			if(FLASH_Write(STM32_A_START_ADDR + complete_pages * STM32_PAGE_SIZE,
						  (uint8_t *)UpdateA.Updata_A_Buff, 
						  remaining_bytes) != HAL_OK)
			{
				printf("错误：剩余数据写入失败！\r\n");
				BootStateFlag &= ~UPDATA_A_FLAG;
				return;
			}
			printf("剩余数据写入完成\r\n");
		}
		
		// 7. 数据验证（可选但推荐）
		printf("开始数据验证...\r\n");
		if(Verify_Firmware_Data(STM32_A_START_ADDR, w25q64_base_addr, firmware_len) != HAL_OK)
		{
			printf("警告：数据验证失败！\r\n");
			// 这里可以选择继续或中止
		}
		else
		{
			printf("数据验证通过\r\n");
		}
		
		// 8. 更新完成处理
		if(UpdateA.Updata_A_from_W25Q64_Num == 0)
		{
			OTA_Info.OTA_flag = 0;
			if(W25Q128_WriteOTAInfoSafe() != HAL_OK)
			{
				printf("警告：OTA信息保存失败！\r\n");
			}
		}
		
		printf("A区更新完成，马上重启系统！\r\n");
		HAL_Delay(500);  // 给串口输出留出时间
		NVIC_SystemReset();
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
//  MX_TIM1_Init();
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */
//	// 开启更新中断
//	HAL_TIM_Base_Start_IT(&htim1);
	Print_System_Info();
	#if 0
	memset(&OTA_Info.OTA_Version,0,sizeof(OTA_Info.OTA_Version));
	OTA_Info.OTA_flag = 0xA1A2A3A4;
	for(uint8_t i = 0; i<STM32_OTA_FIRE_NUMBER; i++)
	{
		OTA_Info.FirmwareLen[i] = 0x00;
	}
	memcpy(&OTA_Info.OTA_Version,version,25);
	W25Q128_WriteOTAInfoSafe();
	#endif
	W25Q128_ReadOTAInfo();
	BootLoader_Branch();
  /* USER CODE END 2 */
  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */
//	HAL_Delay(500);
//	HAL_GPIO_TogglePin(LED2_GPIO_Port,LED2_Pin);
	  if(uart2.rp != uart2.wp)
	  {
//		  printf("rev: %d byte!\r\n",UART_Buf_DataCount(&uart2));
//		  print_array_hex("rev: %d byte",&uart2.buf[uart2.rp],UART_Buf_DataCount(&uart2));
		  BootLoader_Event(&uart2.buf[uart2.rp],UART_Buf_DataCount(&uart2));
	  }
	  
		/*--------------------------------------------------*/
		/*                 Xmodem协议发送C                  */
		/*--------------------------------------------------*/
		if(BootStateFlag&IAP_XMODEMC_FLAG){     //如果IAP_XMODEMC_FLAG标志位置位，表明需要发送C
			if(UpdateA.XmodemTimer>=100){     //计算间隔时间，到时进入if
				HAL_UART_Transmit(&huart2, (uint8_t*)"C", 2, HAL_MAX_DELAY);
				UpdateA.XmodemTimer = 0;      //清除计算间隔时间的变量
			}
			UpdateA.XmodemTimer++;            //计算间隔时间的变量++
		}	
		HAL_Delay(10);
		my_update_fireware_ota();
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */
  /* USER CODE END Callback 0 */
  /* USER CODE BEGIN Callback 1 */
    if (htim == (&htim1))
    {
		HAL_GPIO_TogglePin(LED1_GPIO_Port,LED1_Pin);
    }
  /* USER CODE END Callback 1 */
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
