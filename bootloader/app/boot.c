#include "boot.h"
#include "W25Q128.h"
#include "string.h"
#include "usart.h"
#include "flash.h"

OTA_Info_CB OTA_Info;
UpdateA_CB UpdateA;

uint32_t BootStateFlag = 0;   	//记录全局状态标志位，每位表示1种状态
/**
  * @brief  从W25Q128最后一个page读取OTA信息
  * @retval 无
  */
void W25Q128_ReadOTAInfo(void)
{  
    uint8_t rx_buffer[256];
    
    memset(rx_buffer, 0, sizeof(rx_buffer));
    memset(&OTA_Info, 0, OTA_INFOCB_SIZE);
    
    // 读取整个page（256字节）
    W25Q128_ReadData(W25Q128_LAST_ADDR, rx_buffer, sizeof(rx_buffer));
    
    // 从缓冲区拷贝到结构体
    memcpy(&OTA_Info, rx_buffer, OTA_INFOCB_SIZE);
    
    printf("Read OTA Info from 0x%06X:\r\n", W25Q128_LAST_ADDR);
    printf("  OTA_flag: 0x%08X\r\n", OTA_Info.OTA_flag);
    
    for(uint8_t i = 0; i < STM32_OTA_FIRE_NUMBER; i++)
    {
        printf("  FirmwareLen[%d]: %d\r\n", i, OTA_Info.FirmwareLen[i]);
    }
    printf("  OTA_Version: %s\r\n", OTA_Info.OTA_Version);
}

/**
  * @brief  安全写入OTA信息（先读取现有数据，只修改需要的变化）
  * @retval 1-成功, 0-失败
  */
uint8_t W25Q128_WriteOTAInfoSafe(void)
{
    uint8_t tx_buffer[256];
    uint8_t rx_buffer[256];
    uint16_t written = 0;
    
    // 1. 先读取整个page的当前数据
    memset(rx_buffer, 0, sizeof(rx_buffer));
    W25Q128_ReadData(W25Q128_LAST_ADDR, rx_buffer, sizeof(rx_buffer));
    
    // 2. 准备写入数据：保持其他数据不变，只更新OTA信息部分
    memcpy(tx_buffer, rx_buffer, sizeof(tx_buffer));  // 先拷贝原有数据
    memcpy(tx_buffer, &OTA_Info, OTA_INFOCB_SIZE);    // 再更新OTA信息
    
    // 3. 比较数据是否有变化，无变化则直接返回
    if(memcmp(tx_buffer, rx_buffer, OTA_INFOCB_SIZE) == 0) {
        printf("OTA信息无变化，跳过写入\r\n");
        return 1;
    }
    
    // 4. 计算扇区地址并擦除
    uint32_t sector_addr = W25Q128_LAST_ADDR & 0xFFFFF000;
    printf("Erasing sector at 0x%08X...\r\n", sector_addr);
    W25Q128_SectorErase(sector_addr);
    W25Q128_WaitBusy();
    
    // 5. 写入更新后的数据
    written = W25Q128_WriteData(W25Q128_LAST_ADDR, tx_buffer, sizeof(tx_buffer));
    printf("写入完成: %d字节\r\n", written);
    
    return (written == sizeof(tx_buffer));
}

/*
MSR MSP, R0：将参数addr（通过R0传递）写入主栈指针MSP
BX R14：使用链接寄存器R14（LR）返回，R14保存了返回地址
*/
__ASM void MSR_SP(uint32_t addr)
{
	MSR MSP, R0
	BX R14
}

void BootLoader_Clear(void)
{
	HAL_UART_MspDeInit(&huart1);
}

void LOAD_A(uint32_t addr)
{
	pFunction Jump_To_Application;
	
	if((*(uint32_t *)addr >= 0x20000000) && (*(uint32_t *)addr <= 0x20004fff)){
		printf("A分区代码MSP有效性判断通过\r\n");
		MSR_SP(*(uint32_t *)addr);
		Jump_To_Application = (pFunction)*(uint32_t *)(addr + 4);	
		BootLoader_Clear();
		Jump_To_Application();
	}else{
		printf("A分区代码MSP有效性判断不通过，跳转A分区失败！\r\n");
	}
}


uint8_t BootLoader_Enter_Command(uint8_t timeout)	 //timeout 单位 百毫秒
{
	printf("%d ms 内输入 w ，进入命令行\r\n", timeout * 100);
	while(timeout--)
	{
		HAL_Delay(100);
		if(uart2.buf[0] == 'w')
		{
			ringbuf_advance_tail(&uart2,1);
			return 1;
		}
	}
	return 0;
}

/*-------------------------------------------------*/
/*函数名：串口输出命令行信息                       */
/*参  数：无                                       */
/*返回值：无                                       */
/*-------------------------------------------------*/
void BootLoader_Info(void)
{ 
	printf("\r\n");                  
	printf("[1]擦除A区\r\n");
	printf("[2]串口IAP下载A区程序\r\n");
	printf("[3]设置OTA版本号\r\n");
	printf("[4]查询OTA版本号\r\n");
	printf("[5]向外部Flash下载程序\r\n");
	printf("[6]使用外部Flash内程序\r\n"); 
	printf("[7]重启\r\n");
}

void BootLoader_Branch(void)
{
	if(BootLoader_Enter_Command(30) == 0)
	{
		if(OTA_Info.OTA_flag == OTA_SET_FLAG)
		{
			printf("OTA有更新！\r\n");
	//		BootStateFlag |= UPDATA_A_FLAG;
	//		UpdateA.Updata_A_from_W25Q64_Num = 0;
		}
		else
		{
			printf("OTA无更新，跳转A分区代码\r\n");
			LOAD_A(STM32_A_START_ADDR);
		}
	}
	printf("进入BootLoader命令行\r\n");           //串口0输出信息
	BootLoader_Info();                               //串口输出命令行信息
}


/*-------------------------------------------------*/
/*函数名：BootLoader处理串口数据                   */
/*参  数：data：数据指针      datalen：数据长度    */
/*返回值：无                                       */
/*-------------------------------------------------*/
void BootLoader_Event(uint8_t *data, uint16_t datalen)
{
    uint8_t ack_byte = 0x06;
    uint8_t nck_byte = 0x15;
    int temp = 0;
	int i = 0;
    /*--------------------------------------------------*/
    /*          没有任何事件，判断顶层命令              */
    /*--------------------------------------------------*/
    if(BootStateFlag == 0)
    {
        ringbuf_advance_tail(&uart2, 1);  // 可以提到外面，避免重复
        switch(data[0])
        {
            case '1':
                printf("擦除A区\r\n");
				STM32_EraseFromPageToEnd(20);
//				if(FLASH_EraseRange(STM32_A_START_ADDR,STM32_A_END_ADDR) == HAL_OK)		//将A区空间擦除
//				{
//					printf("FLASH_EraseRange A success!\r\n");
//				}
//                else
//                    printf("FLASH_EraseRange A failed!\r\n");
				BootLoader_Info(); 
                break;
                
            case '2':
                printf("通过Xmodem协议，串口IAP下载A区程序，请使用bin格式文件\r\n");
                BootStateFlag |= (IAP_XMODEMC_FLAG | IAP_XMODEMD_FLAG);
                UpdateA.XmodemTimer = 0;
                UpdateA.XmodemNB = 0;
                break;
                
            case '3':
                printf("设置版本号\r\n");
                // TODO: 实现版本号设置
				BootStateFlag |= IAP_WRITE_VERSION_FLAG;
                break;    
            case '4':
                printf("查询版本号\r\n");
				W25Q128_ReadOTAInfo(); 
				printf("版本号:%s\r\n",OTA_Info.OTA_Version);
				BootLoader_Info();                                                         //串口输出命令行信息  
			break;        
            case '5':
                printf("向外部Flash下载程序，输入需要使用的块编号（1~9）\r\n");
                // TODO: 实现外部Flash下载
				BootStateFlag|= IAP_DOWNLOAD_TO_FLASH_FLAG;
                break;
                
            case '6':
                printf("使用外部Flash内的程序，输入需要使用的块编号（1~9）\r\n");
                // TODO: 实现外部Flash程序使用
				BootStateFlag|= IAP_USE_FLASH_PROGRAM_FLAG;
                break;
                
            case '7':
                printf("重启\r\n");
                HAL_Delay(100);
                NVIC_SystemReset();
                break;
                
            default:
                printf("未知命令\r\n");
                break;
        }
    }
    /*--------------------------------------------------*/
    /*          发生Xmodem事件，处理该事件              */
    /*--------------------------------------------------*/
	else if(BootStateFlag & IAP_XMODEMD_FLAG)
	{
		if((datalen == 133) && (data[0] == 0x01))  // Xmodem数据包
		{
			BootStateFlag &= ~IAP_XMODEMC_FLAG;
			// CRC校验
			UpdateA.XmodemCRC = Xmodem_CRC16(&data[3], 128);
			uint16_t received_crc = (data[131] << 8) | data[132];
			if(UpdateA.XmodemCRC == received_crc)
			{
				UpdateA.XmodemNB++;
				// 计算缓冲区位置
				uint32_t buff_pos = ((UpdateA.XmodemNB - 1) % (STM32_PAGE_SIZE / 128)) * 128;
				// 复制数据到缓冲区
				memcpy(&UpdateA.Updata_A_Buff[buff_pos], &data[3], 128);
				
				// 检查是否需要写入Flash（缓冲区满）
				if((UpdateA.XmodemNB % (STM32_PAGE_SIZE / 128)) == 0)
				{
					if(BootStateFlag & IAP_DOWNLOAD_TO_FLASH_XMODEM_FLAG)
					{
						// 计算当前批次对应的逻辑单元索引
						uint16_t currentBatch = (UpdateA.XmodemNB / 8) - 1;
						for(i = 0; i < 4; i++)
						{
							// 正确的页编号计算
							uint16_t pageNumber = currentBatch * 4 + i + UpdateA.Updata_A_from_W25Q64_Num * 64 * 4;
							uint32_t actualAddress = pageNumber * 256;
							W25Q128_PageProgram(actualAddress, &UpdateA.Updata_A_Buff[i*256], 256);
						}
					}
					else
					{
						uint32_t flash_addr = STM32_A_START_ADDR + ((UpdateA.XmodemNB / (STM32_PAGE_SIZE / 128)) - 1) * STM32_PAGE_SIZE;
						printf("ota flash 1K addr at 0x%08X...\r\n", flash_addr);
						if(FLASH_Write(flash_addr, (uint8_t *)UpdateA.Updata_A_Buff, STM32_PAGE_SIZE) != HAL_OK)
						{
							printf("FLASH_Write failed!\r\n");
						}
					}
				}
				HAL_UART_Transmit(&huart2, &ack_byte, 1, HAL_MAX_DELAY);
			}
			else
			{
				HAL_UART_Transmit(&huart2, &nck_byte, 1, HAL_MAX_DELAY);
			}
			
			ringbuf_advance_tail(&uart2, 133);
		}
		else if((datalen == 1) && (data[0] == 0x04))  // EOT结束传输
		{
			HAL_UART_Transmit(&huart2, &ack_byte, 1, HAL_MAX_DELAY);
			
			// 处理剩余数据
			if((UpdateA.XmodemNB % (STM32_PAGE_SIZE / 128)) != 0)
			{
				if(BootStateFlag & IAP_DOWNLOAD_TO_FLASH_XMODEM_FLAG)
				{
					// 计算剩余数据占用的页数
					uint32_t remaining_packets = UpdateA.XmodemNB % 8;
					uint32_t pages_needed = (remaining_packets * 128 + 255) / 256; // 向上取整
					uint16_t currentBatch = UpdateA.XmodemNB / 8;
					for(i = 0; i < pages_needed; i++)
					{
						uint16_t pageNumber = currentBatch * 4 + i + UpdateA.Updata_A_from_W25Q64_Num * 64 * 4;
						uint32_t actualAddress = pageNumber * 256;
						uint32_t write_size = (i == pages_needed - 1) ? 
											 (remaining_packets * 128 - i * 256) : 256;
						W25Q128_PageProgram(actualAddress, &UpdateA.Updata_A_Buff[i*256], write_size);
					}
				}
				else
				{
					uint32_t remaining_bytes = (UpdateA.XmodemNB % (STM32_PAGE_SIZE / 128)) * 128;
					uint32_t flash_addr = STM32_A_START_ADDR + ((UpdateA.XmodemNB / (STM32_PAGE_SIZE / 128))) * STM32_PAGE_SIZE;
					if(FLASH_Write(flash_addr, (uint8_t *)UpdateA.Updata_A_Buff, remaining_bytes) != HAL_OK)
					{
						printf("FLASH_Write failed!\r\n");
					}
				}
			}
			
			BootStateFlag &= ~IAP_XMODEMD_FLAG;
			if(BootStateFlag & IAP_DOWNLOAD_TO_FLASH_XMODEM_FLAG){                           //判断如果是命令5启动Xmodem的话，进入if
				BootStateFlag &=~ IAP_DOWNLOAD_TO_FLASH_XMODEM_FLAG;                       //清除FLAG
				OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num] = UpdateA.XmodemNB * 128;   //计算并保存本次传输的程序大小
				printf("OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num]:%d\r\n",OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num]);
				W25Q128_WriteOTAInfoSafe();
				HAL_Delay(100); 
				BootLoader_Info();                                      //输出命令行信息
			}else{                                                      //判断如果不是命令5启动Xmodem的话，那就是串口IAP启动的，进入else
				HAL_Delay(100);
				NVIC_SystemReset();                                     //重启
			}
		}
	}
	/*--------------------------------------------------*/
	/*         发生设置版本号事件，处理改事件           */
	/*--------------------------------------------------*/
	else if(BootStateFlag & IAP_WRITE_VERSION_FLAG){                              //进入分支，处理设置版本号事件
		if(datalen==26){                                                //判断版本号长度是不是26，正确进入if
			if(sscanf((char *)data,"VER-%d.%d.%d-%d/%d/%d-%d:%d",&temp,&temp,&temp,&temp,&temp,&temp,&temp,&temp)==8){  //判断版本号格式，正确进入if
				memset(OTA_Info.OTA_Version,0,32);                          //清除OTA_Info.OTA_ver缓冲区
				memcpy(OTA_Info.OTA_Version,data,26);                       //将串口发送过来的版本号拷贝到OTA_Info.OTA_ver缓冲区       
				W25Q128_WriteOTAInfoSafe();
				printf("版本正确\r\n");
				BootStateFlag &=~ IAP_WRITE_VERSION_FLAG;        		//清除标志位
				BootLoader_Info();                                      //输出命令行信息
			}
			else printf("版本号格式错误\r\n");                      //判断版本号格式错误，串口0提示
		}else printf("版本号长度错误\r\n");                          //判断版本号长度错误，串口0提示
		
		ringbuf_advance_tail(&uart2, datalen);
	}
	/*--------------------------------------------------*/
	/*            发生 DOWNLOAD_TO_FLASH 事件，处理改事件             */
	/*--------------------------------------------------*/
	else if(BootStateFlag & IAP_DOWNLOAD_TO_FLASH_FLAG){                   				//进入分支，处理DOWNLOAD_TO_FLASH，将程序文件写入W25Q64
		if(datalen==1)																	//数据长度是1正确，进入if，表示W25Q64的块标号
		{
			if((data[0]>=0x31)&&(data[0]<=0x39))                       					//判断W25Q64的块标号，范围1~9，正确进入if
			{	
				UpdateA.Updata_A_from_W25Q64_Num = data[0] - 0x30 - 1;                				//将块标号由字符1~9编号，转换成数字1~9
				printf("UpdateA.Updata_A_from_W25Q64_Num :%d\r\n",UpdateA.Updata_A_from_W25Q64_Num);
				BootStateFlag |= (IAP_XMODEMC_FLAG|IAP_XMODEMD_FLAG|IAP_DOWNLOAD_TO_FLASH_XMODEM_FLAG); //置位3个标志位
				UpdateA.XmodemTimer = 0;                                             	//Xmodem发送大写C 间隔时间变量清零
				UpdateA.XmodemNB= 0;                                                 	//保持接收Xmodem协议数据包个数的变量清零
				OTA_Info.FirmwareLen[UpdateA.Updata_A_from_W25Q64_Num] = 0;      		//W25Q64的块标号对应的程序大小变量清零
				W25Q128_EraseBlockByNumber(UpdateA.Updata_A_from_W25Q64_Num - 1);		//擦除相应的W25Q64块
				printf("通过Xmodem协议，向外部Flash第%d个块下载程序，请使用bin格式文件\r\n",UpdateA.Updata_A_from_W25Q64_Num); //串口0输出信息
				BootStateFlag &=~ IAP_DOWNLOAD_TO_FLASH_FLAG;                                          //清除标志位
			}else printf("编号错误\r\n");                            	//判断W25Q64的块标号，范围1~9，错误进入else，串口0输出信息
		}else printf("数据长度错误\r\n");                            		//判断数据长度，不是1的话错误进入else，串口0输出信息
		
		ringbuf_advance_tail(&uart2, datalen);
	}
	/*--------------------------------------------------*/
	/*            发生 USE_FLASH_PROGRAM 事件，处理改事件             */
	/*--------------------------------------------------*/
	else if(BootStateFlag & IAP_USE_FLASH_PROGRAM_FLAG)  		//进入分支，处理命令6，使用W25Q64内的程序
	{	
		if(datalen==1)
		{ 
			if((data[0]>=0x31)&&(data[0]<=0x39))        				//判断W25Q64的块标号，范围1~9，正确进入if
			{	
				UpdateA.Updata_A_from_W25Q64_Num = data[0] - 0x30 - 1; 		//将块标号由字符1~9编号，转换成数字1~9
				BootStateFlag |= UPDATA_A_FLAG;           				 //置位标志位，表明需要更新A区
				BootStateFlag &=~ IAP_USE_FLASH_PROGRAM_FLAG;              //清除标志位
			}else printf("编号错误\r\n");             //判断W25Q64的块标号，范围1~9，错误进入else，串口0输出信息
		}else printf("数据长度错误\r\n");             //判断数据长度，不是1的话错误进入else，串口0输出信息
	}
}


/*-------------------------------------------------*/
/*函数名：Xmodem_CRC16校验                         */
/*参  数：data：数据指针  datalen：数据长度        */
/*返回值：校验后的数据                             */
/*-------------------------------------------------*/
uint16_t Xmodem_CRC16(uint8_t *data, uint16_t datalen)
{
	uint8_t i;                                               //用于for循环
	uint16_t Crcinit = 0x0000;                               //Xmdoem CRC校验的初始值，必须是0x0000
	uint16_t Crcipoly = 0x1021;                              //Xmdoem CRC校验的多项式，必须是0x1021
	
	while(datalen--){                                        //根据datalen大小，有多少字节循环多少次
		Crcinit = (*data << 8) ^ Crcinit;                    //先将带校验的字节，挪到高8位
		for(i=0;i<8;i++){                                    //每个字节8个二进制位，循环8次
			if(Crcinit&0x8000)                               //判断BIT15是1还是0,是1的话，进入if
				Crcinit = (Crcinit << 1) ^ Crcipoly;         //是1的话，先左移，再异或多项式
			else                                             //判断BIT15是1还是0,是0的话，进入else
				Crcinit = (Crcinit << 1);                    //是0的话，只左移
		}
		data++;                                              //下移，计算一个字节数据
	}
	return Crcinit;                                          //返回校验后的数据
}


