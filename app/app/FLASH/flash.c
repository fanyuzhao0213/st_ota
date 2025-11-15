#include "flash.h"
#include <string.h>
#include "usart.h"

/**
  * @brief  Flash初始化（解锁）
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_Init(void)
{
    // 解锁Flash
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 清除Flash标志（F1系列可用的标志）
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_BSY | FLASH_FLAG_PGERR | FLASH_FLAG_EOP);
    
    return HAL_OK;
}

/**
  * @brief  Flash反初始化（上锁）
  * @retval 无
  */
void FLASH_DeInit(void)
{
    HAL_FLASH_Lock();
}

/**
  * @brief  擦除指定扇区
  * @param  sector: 扇区号
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_EraseSector(FlashSector_t sector)
{
    FLASH_EraseInitTypeDef erase_init;
    uint32_t sector_error;
    
    if (sector >= FLASH_TOTAL_SECTORS) {
        return HAL_ERROR;
    }
    
    // 先解锁
    if (FLASH_Init() != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 配置擦除参数（F1系列按页擦除）
    erase_init.TypeErase = FLASH_TYPEERASE_PAGES;
    erase_init.Banks = FLASH_BANK_1;
    erase_init.PageAddress = FLASH_GetSectorAddress(sector);
    erase_init.NbPages = FLASH_SECTOR_SIZE / FLASH_PAGE_SIZE; // 使用HAL的定义
    
    // 执行擦除
    if (HAL_FLASHEx_Erase(&erase_init, &sector_error) != HAL_OK) {
        HAL_FLASH_Lock(); // 出错时上锁
        return HAL_ERROR;
    }
    
    // 操作完成上锁
    HAL_FLASH_Lock();
    return HAL_OK;
}

/**
  * @brief  擦除地址范围内的所有扇区
  * @param  start_addr: 起始地址
  * @param  end_addr: 结束地址
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_EraseRange(uint32_t start_addr, uint32_t end_addr)
{
    FlashSector_t start_sector, end_sector, sector;
    
    // 地址检查
    if (start_addr < FLASH_START_ADDR || end_addr > FLASH_END_ADDR || start_addr > end_addr) {
        return HAL_ERROR;
    }
    
    start_sector = FLASH_GetSectorFromAddress(start_addr);
    end_sector = FLASH_GetSectorFromAddress(end_addr);
    
    // 擦除范围内的所有扇区
    for (sector = start_sector; sector <= end_sector; sector++) {
        if (FLASH_EraseSector(sector) != HAL_OK) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
  * @brief  写入数据到Flash
  * @param  address: 写入地址（必须4字节对齐）
  * @param  data: 数据指针
  * @param  size: 数据大小（字节）
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_Write(uint32_t address, uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint32_t *data_32 = (uint32_t*)data;
    uint32_t word_count = (size + 3) / 4; // 计算32位字数量
    
    // 地址对齐检查
    if (address % 4 != 0) {
        return HAL_ERROR;
    }
    
    // 地址范围检查
    if (address < FLASH_START_ADDR || (address + size) > FLASH_END_ADDR) {
        return HAL_ERROR;
    }
    
    // 先解锁
    if (FLASH_Init() != HAL_OK) {
        return HAL_ERROR;
    }
    
    // 按32位字写入
    for (i = 0; i < word_count; i++) {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + i * 4, data_32[i]) != HAL_OK) {
            HAL_FLASH_Lock(); // 出错时上锁
            return HAL_ERROR;
        }
    }
    
    // 操作完成上锁
    HAL_FLASH_Lock();
    return HAL_OK;
}

/**
  * @brief  写入32位字
  * @param  address: 写入地址（必须4字节对齐）
  * @param  data: 32位数据
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_WriteWord(uint32_t address, uint32_t data)
{
    if (address % 4 != 0) {
        return HAL_ERROR;
    }
    
    // 解锁
    if (FLASH_Init() != HAL_OK) {
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data);
    
    // 上锁
    HAL_FLASH_Lock();
    return status;
}

/**
  * @brief  写入16位半字
  * @param  address: 写入地址（必须2字节对齐）
  * @param  data: 16位数据
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_WriteHalfWord(uint32_t address, uint16_t data)
{
    if (address % 2 != 0) {
        return HAL_ERROR;
    }
    
    // 解锁
    if (FLASH_Init() != HAL_OK) {
        return HAL_ERROR;
    }
    
    HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD, address, data);
    
    // 上锁
    HAL_FLASH_Lock();
    return status;
}

/**
  * @brief  读取数据
  * @param  address: 读取地址
  * @param  data: 数据缓冲区
  * @param  size: 读取大小
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_Read(uint32_t address, uint8_t *data, uint32_t size)
{
    uint32_t i;
    
    // 地址范围检查
    if (address < FLASH_START_ADDR || (address + size) > FLASH_END_ADDR) {
        return HAL_ERROR;
    }
    
    // 直接内存读取（不需要解锁）
    for (i = 0; i < size; i++) {
        data[i] = *(volatile uint8_t*)(address + i);
    }
    
    return HAL_OK;
}

/**
  * @brief  读取32位字
  * @param  address: 读取地址
  * @retval 32位数据
  */
uint32_t FLASH_ReadWord(uint32_t address)
{
    return *(volatile uint32_t*)address;
}

/**
  * @brief  读取16位半字
  * @param  address: 读取地址
  * @retval 16位数据
  */
uint16_t FLASH_ReadHalfWord(uint32_t address)
{
    return *(volatile uint16_t*)address;
}

/**
  * @brief  读取8位字节
  * @param  address: 读取地址
  * @retval 8位数据
  */
uint8_t FLASH_ReadByte(uint32_t address)
{
    return *(volatile uint8_t*)address;
}

/**
  * @brief  验证写入的数据
  * @param  address: 验证地址
  * @param  data: 原始数据
  * @param  size: 数据大小
  * @retval HAL状态
  */
HAL_StatusTypeDef FLASH_Verify(uint32_t address, uint8_t *data, uint32_t size)
{
    uint32_t i;
    uint8_t read_data;
    
    for (i = 0; i < size; i++) {
        read_data = FLASH_ReadByte(address + i);
        if (read_data != data[i]) {
            return HAL_ERROR;
        }
    }
    
    return HAL_OK;
}

/**
  * @brief  获取扇区起始地址
  * @param  sector: 扇区号
  * @retval 扇区起始地址
  */
uint32_t FLASH_GetSectorAddress(FlashSector_t sector)
{
    switch(sector) {
        case FLASH_SECTOR_0: return 0x08000000;
        case FLASH_SECTOR_1: return 0x08004000;
        case FLASH_SECTOR_2: return 0x08008000;
        case FLASH_SECTOR_3: return 0x0800C000;
        default: return 0;
    }
}

/**
  * @brief  根据地址获取扇区号
  * @param  address: 地址
  * @retval 扇区号
  */
FlashSector_t FLASH_GetSectorFromAddress(uint32_t address)
{
    if (address >= 0x0800C000) return FLASH_SECTOR_3;
    if (address >= 0x08008000) return FLASH_SECTOR_2;
    if (address >= 0x08004000) return FLASH_SECTOR_1;
    return FLASH_SECTOR_0;
}

/**
  * @brief  获取扇区大小
  * @param  sector: 扇区号
  * @retval 扇区大小（字节）
  */
uint32_t FLASH_GetSectorSize(FlashSector_t sector)
{
    return FLASH_SECTOR_SIZE;
}

/**
  * @brief  简单的Flash功能测试
  */
void FLASH_QuickTest(void)
{
    uint8_t test_data[32];
    uint8_t read_back[32];
    uint32_t i;
    
    printf("\r\n==== Quick Flash Test ====\r\n");
    
    // 准备测试数据
    for (i = 0; i < sizeof(test_data); i++) {
        test_data[i] = 0xA0 + i;  // 0xA0, 0xA1, 0xA2, ...
    }
    
    // 测试地址（扇区3的末尾）
    uint32_t test_addr = FLASH_GetSectorAddress(FLASH_SECTOR_3) + FLASH_SECTOR_SIZE - 64;
    
    printf("Test Address: 0x%08X\r\n", test_addr);
    
    // 擦除扇区
    if (FLASH_EraseSector(FLASH_SECTOR_3) != HAL_OK) {
        printf("Erase failed!\r\n");
        return;
    }
    
    // 写入数据
    if (FLASH_Write(test_addr, test_data, sizeof(test_data)) != HAL_OK) {
        printf("Write failed!\r\n");
        return;
    }
    
    // 读取验证
    if (FLASH_Read(test_addr, read_back, sizeof(read_back)) != HAL_OK) {
        printf("Read failed!\r\n");
        return;
    }
    
    // 简单比较
    if (memcmp(test_data, read_back, sizeof(test_data)) == 0) {
        printf("Quick test: PASSED ?\r\n");
    } else {
        printf("Quick test: FAILED ?\r\n");
        
        printf("Expected: ");
        for (i = 0; i < 16; i++) printf("%02X ", test_data[i]);
        printf("\r\nGot:      ");
        for (i = 0; i < 16; i++) printf("%02X ", read_back[i]);
        printf("\r\n");
    }
    
    printf("==== Quick Test End ====\r\n\r\n");
}

/**
  * @brief  Flash信息显示
  */
void FLASH_ShowInfo(void)
{
    printf("\r\n==== STM32F103C8T6 Flash Info ====\r\n");
    printf("Flash Start Address: 0x%08lX\r\n", FLASH_START_ADDR);
    printf("Flash Size: %d KB\r\n", FLASH_SIZE / 1024);
    printf("Sector Size: %d KB\r\n", FLASH_SECTOR_SIZE / 1024);
    printf("Total Sectors: %d\r\n", FLASH_TOTAL_SECTORS);
    printf("=====================================\r\n\r\n");
}

