#include "W25Q128.h"
#include "spi.h"
#include "usart.h"

/**
  * @brief  SPI起始（片选拉低）
  * @retval 无
  */
void MySPI_Start(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_RESET); // CS引脚拉低，假设CS在PA4
}

/**
  * @brief  SPI终止（片选拉高）
  * @retval 无
  */
void MySPI_Stop(void)
{
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12, GPIO_PIN_SET); // CS引脚拉高
}

/**
  * @brief  SPI交换一个字节
  * @param  byte: 要发送的字节
  * @retval 接收到的字节
  */
uint8_t MySPI_SwapByte(uint8_t byte)
{
    uint8_t rx_data;
    HAL_SPI_TransmitReceive(&hspi1, &byte, &rx_data, 1, HAL_MAX_DELAY);
    return rx_data;
}

/**
  * 函    数：W25Q128读取ID号
  * 参    数：MID 工厂ID，使用输出参数的形式返回
  * 参    数：DID 设备ID，使用输出参数的形式返回
  * 返 回 值：无
  */
void W25Q128_ReadID(uint8_t *MID, uint16_t *DID)
{
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q64_JEDEC_ID);			//交换发送读取ID的指令
	*MID = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//交换接收MID，通过输出参数返回
	*DID = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//交换接收DID高8位
	*DID <<= 8;									//高8位移到高位
	*DID |= MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//或上交换接收DID的低8位，通过输出参数返回
	MySPI_Stop();								//SPI终止
}

/**
  * 函    数：W25Q128写使能
  * 参    数：无
  * 返 回 值：无
  */
void W25Q128_WriteEnable(void)
{
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q64_WRITE_ENABLE);		//交换发送写使能的指令
	MySPI_Stop();								//SPI终止
}

/**
  * 函    数：W25Q128等待忙
  * 参    数：无
  * 返 回 值：无
  */
void W25Q128_WaitBusy(void)
{
	uint32_t Timeout;
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q64_READ_STATUS_REGISTER_1);				//交换发送读状态寄存器1的指令
	Timeout = 100000;							//给定超时计数时间
	while ((MySPI_SwapByte(W25Q64_DUMMY_BYTE) & 0x01) == 0x01)	//循环等待忙标志位
	{
		Timeout --;								//等待时，计数值自减
		if (Timeout == 0)						//自减到0后，等待超时
		{
			/*超时的错误处理代码，可以添加到此处*/
			break;								//跳出等待，不等了
		}
	}
	MySPI_Stop();								//SPI终止
}

/**
  * 函    数：W25Q128页编程
  * 参    数：Address 页编程的起始地址，范围：0x000000~0xFFFFFF
  * 参    数：DataArray	用于写入数据的数组
  * 参    数：Count 要写入数据的数量，范围：0~256
  * 返 回 值：无
  * 注意事项：写入的地址范围不能跨页
  */
void W25Q128_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count)
{
	uint16_t i;
	
	W25Q128_WriteEnable();						//写使能
	
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q64_PAGE_PROGRAM);		//交换发送页编程的指令
	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	MySPI_SwapByte(Address);					//交换发送地址7~0位
	for (i = 0; i < Count; i ++)				//循环Count次
	{
		MySPI_SwapByte(DataArray[i]);			//依次在起始地址后写入数据
	}
	MySPI_Stop();								//SPI终止
	
	W25Q128_WaitBusy();							//等待忙
}

/**
  * 函    数：W25Q128完整写入数据（自动处理跨页）
  * 参    数：Address 写入的起始地址，范围：0x000000~0xFFFFFF
  * 参    数：DataArray 用于写入数据的数组
  * 参    数：Count 要写入数据的数量
  * 返 回 值：实际写入的数据数量
  * 注意事项：自动处理跨页写入，支持任意长度数据
  */
uint32_t W25Q128_WriteData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
    uint32_t bytes_written = 0;
    uint32_t current_addr = Address;
    uint8_t *current_data = DataArray;
    
    // 检查地址和长度是否有效
    if (Address >= 0xFFFFFF) {
        return 0; // 地址超出范围
    }
    
    if (Count == 0) {
        return 0; // 没有数据要写入
    }
    
    // 限制写入长度不超过芯片容量
    if (Address + Count > 0xFFFFFF) {
        Count = 0xFFFFFF - Address;
    }
    
    while (bytes_written < Count) {
        // 计算当前页剩余空间
        uint32_t page_start = (current_addr / 0xFFFFFF) * 0xFFFFFF;
        uint32_t page_offset = current_addr - page_start;
        uint32_t page_remaining = 0xFFFFFF - page_offset;
        
        // 计算本次写入的长度
        uint32_t write_len = Count - bytes_written;
        if (write_len > page_remaining) {
            write_len = page_remaining;
        }
        
        // 执行页编程
        W25Q128_PageProgram(current_addr, current_data, write_len);
        
        // 更新计数器和指针
        bytes_written += write_len;
        current_addr += write_len;
        current_data += write_len;
    }
    
    return bytes_written;
}


/**
  * 函    数：W25Q128扇区擦除（4KB）
  * 参    数：Address 指定扇区的地址，范围：0x000000~0xFFFFFF
  * 返 回 值：无
  */
void W25Q128_SectorErase(uint32_t Address)
{
	W25Q128_WriteEnable();						//写使能
	
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q64_SECTOR_ERASE_4KB);	//交换发送扇区擦除的指令
	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	MySPI_SwapByte(Address);					//交换发送地址7~0位
	MySPI_Stop();								//SPI终止
	
	W25Q128_WaitBusy();							//等待忙
}

/**
  * 函    数：W25Q128读取数据
  * 参    数：Address 读取数据的起始地址，范围：0x000000~0xFFFFFF
  * 参    数：DataArray 用于接收读取数据的数组，通过输出参数返回
  * 参    数：Count 要读取数据的数量，范围：0~0x800000
  * 返 回 值：无
  */
void W25Q128_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count)
{
	uint32_t i;
	MySPI_Start();								//SPI起始
	MySPI_SwapByte(W25Q64_READ_DATA);			//交换发送读取数据的指令
	MySPI_SwapByte(Address >> 16);				//交换发送地址23~16位
	MySPI_SwapByte(Address >> 8);				//交换发送地址15~8位
	MySPI_SwapByte(Address);					//交换发送地址7~0位
	for (i = 0; i < Count; i ++)				//循环Count次
	{
		DataArray[i] = MySPI_SwapByte(W25Q64_DUMMY_BYTE);	//依次在起始地址后读取数据
	}
	MySPI_Stop();								//SPI终止
}


#include <string.h>   // 用于 memset
uint8_t test_data[1024] = {0};
void W25Q128_Test(void)
{
    uint8_t MID;
    uint16_t DID;
    uint8_t TxBuffer[256];
    uint8_t RxBuffer[256];
    uint32_t i;

    printf("==== W25Q128 Test Start ====\r\n");

    /* 1. 读 JEDEC ID */
    W25Q128_ReadID(&MID, &DID);
	printf("W25Q64 JEDEC ID: MID=0x%02X, DID=0x%04X\r\n", MID, DID);
    // 常见ID判断
    if (MID == 0xEF)   // Winbond 厂家 ID
    {
        switch (DID)
        {
            case 0x4017: printf("Detected: W25Q64 (64M-bit / 8M-Byte)\r\n"); break;
            case 0x4018: printf("Detected: W25Q128 (128M-bit / 16M-Byte)\r\n"); break;
            default:     printf("Unknown Winbond Flash, DID=0x%04X\r\n", DID); break;
        }
    }
    else
    {
        printf("Unknown Manufacturer ID: 0x%02X\r\n", MID);
    }
	
//    /* 2. 扇区擦除 */
//    printf("Erase Sector at 0x000000 ...\r\n");
//    W25Q128_SectorErase(0x000000);

//    /* 3. 扇区擦除后读头部数据 */
//    memset(RxBuffer, 0, sizeof(RxBuffer));
//    W25Q128_ReadData(0x000000, RxBuffer, 16);
//    printf("Read Sector Head after Erase (0x000000): ");
//    for (i = 0; i < 16; i++) printf("%02X ", RxBuffer[i]);
//    printf("\r\n");

//    /* 3.1 扇区擦除后读尾部数据 (扇区大小 4KB，尾部地址 0x0FFF) */
//    memset(RxBuffer, 0, sizeof(RxBuffer));
//    W25Q128_ReadData(0x0FFF - 15, RxBuffer, 16);
//    printf("Read Sector Tail after Erase (0x0FF0 ~ 0x0FFF): ");
//    for (i = 0; i < 16; i++) printf("%02X ", RxBuffer[i]);
//    printf("\r\n");

//    /* 4. 写使能 + 页编程 (写到 0x000000) */
//    for (i = 0; i < 256; i++)
//    {
//        TxBuffer[i] = i;   // 填充测试数据 0x00 ~ 0xFF
//    }
//    printf("Page Program 256 bytes at 0x000000 ...\r\n");
//    W25Q128_PageProgram(0x000000, TxBuffer, 256);

//    /* 5. 读出刚写的数据 */
//    memset(RxBuffer, 0, sizeof(RxBuffer));
//    W25Q128_ReadData(0x000000, RxBuffer, 256);
//    printf("Read back first 256 bytes (0x000000): \r\n");
//    for (i = 0; i < 256; i++) printf("%02X ", RxBuffer[i]);
//    printf("\r\n");

//    /* 6. 校验写入的数据 */
//    if (memcmp(TxBuffer, RxBuffer, 256) == 0)
//        printf("Page Program Verify at 0x000000: PASS\r\n");
//    else
//        printf("Page Program Verify at 0x000000: FAIL\r\n");

	uint16_t written =0;
	memset(test_data,0xBB,1024);
	// 方法1：直接写入任意长度数据（自动跨页）
    written = W25Q128_WriteData(0x1000, test_data, sizeof(test_data));
    printf("W25Q128_WriteData write data of page: %d byte\r\n", written);
	
	
    /* 7. 测试在最后一页写入 (0xFFFFFF - 255 ~ 0xFFFFFF) */
    for (i = 0; i < 256; i++)
    {
        TxBuffer[i] = 0xA0 + (i & 0x0F);  // 填充另一组测试数据
    }
    printf("Page Program 256 bytes at LAST PAGE (0xFFFF00) ...\r\n");
    W25Q128_PageProgram(0xFFFFFF - 255, TxBuffer, 256);

    /* 8. 读回最后一页数据 */
    memset(RxBuffer, 0, sizeof(RxBuffer));
    W25Q128_ReadData(0xFFFFFF - 255, RxBuffer, 256);
    printf("Read back last page first 256 bytes (0xFFFF00): \r\n");
    for (i = 0; i < 256; i++) printf("%02X ", RxBuffer[i]);
    printf("\r\n");

    /* 9. 校验最后一页写入的数据 */
    if (memcmp(TxBuffer, RxBuffer, 256) == 0)
        printf("Page Program Verify at LAST PAGE: PASS\r\n");
    else
        printf("Page Program Verify at LAST PAGE: FAIL\r\n");

    printf("==== W25Q128 Test End ====\r\n");
}
