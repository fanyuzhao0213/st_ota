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
#define version      "VER-1.0.0-2025-11-15-10.59"
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

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	uint8_t index = 0;
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
	
	#if 1
	memset(&OTA_Info.OTA_Version,0,sizeof(OTA_Info.OTA_Version));
	OTA_Info.OTA_flag = 0xA1A2A3A4;
	for(uint8_t i = 0; i<STM32_OTA_FIRE_NUMBER; i++)
	{
		OTA_Info.FirmwareLen[i] = 0x55;
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
		
		if(BootStateFlag & UPDATA_A_FLAG)
		{
			printf("本次需要更新的大小：%d字节\r\n", OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num]);
			if((OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num] % 4) == 0)		//判断长度是否是4字节（32位）的整数倍，是的话则允许写入，进入if
			{	
				if(FLASH_EraseRange(STM32_A_START_ADDR,STM32_A_END_ADDR) == HAL_OK)		//将A区空间擦除
				{
					printf("FLASH_EraseRange A success!\r\n");
				}
			
				/* 从W25Q64中读取1k并写入片上Flash */
				for(index = 0; index < (OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num] / STM32_PAGE_SIZE); index++)
				{					//先写完整的1k字节
					W25Q128_ReadData(index*STM32_PAGE_SIZE + 64*1024*UpdateA.Updata_A_from_W25Q64_Num, UpdateA.Updata_A_Buff, STM32_PAGE_SIZE);	//读取到1k内容
					FLASH_Write(STM32_A_START_ADDR + index*STM32_PAGE_SIZE,(uint8_t *)UpdateA.Updata_A_Buff,STM32_PAGE_SIZE);					//将A区写入片上Flash位置
				}
				
				//现在写完了前面完整的几k内容，下面写不到1k的内容
				if((OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num] % 1024) != 0)
				{
					memset(UpdateA.Updata_A_Buff,0, STM32_PAGE_SIZE);
					W25Q128_ReadData(index*1024 + UpdateA.Updata_A_from_W25Q64_Num*64*1024, UpdateA.Updata_A_Buff, OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num] % 1024);
					FLASH_Write(STM32_A_START_ADDR  + index*STM32_PAGE_SIZE, (uint8_t *)UpdateA.Updata_A_Buff, (OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num] % 1024));//将A区写入片上Flash位置
				}
				
				if(UpdateA.Updata_A_from_W25Q64_Num == 0)
				{
					//更新完成，清除OTA标志位
					OTA_Info.OTA_flag = 0;
					W25Q128_WriteOTAInfoSafe();
				}
				
				printf("A区更新完成，马上重启系统！\r\n");
				HAL_Delay(100);
				NVIC_SystemReset();				
			}
			else
			{
				printf("待更新APP长度错误！\r\n");
				BootStateFlag  &= ~(UPDATA_A_FLAG);
			}

		}

	  
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
