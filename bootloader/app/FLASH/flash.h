#ifndef __FLASH_H
#define __FLASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include "stm32f1xx_hal.h"

/* STM32F103C8T6 Flash定义 */
#define FLASH_START_ADDR          0x08000000UL    // Flash起始地址
#define FLASH_SIZE                (64 * 1024)     // 64KB
#define FLASH_END_ADDR            (FLASH_START_ADDR + FLASH_SIZE - 1)

/* 扇区定义 - STM32F103C8T6 */
typedef enum {
    FLASH_SECTOR_0 = 0,   // 0x08000000 - 0x08003FFF, 16KB
    FLASH_SECTOR_1,       // 0x08004000 - 0x08007FFF, 16KB  
    FLASH_SECTOR_2,       // 0x08008000 - 0x0800BFFF, 16KB
    FLASH_SECTOR_3        // 0x0800C000 - 0x0800FFFF, 16KB
} FlashSector_t;

#define FLASH_SECTOR_SIZE         (16 * 1024)     // 每个扇区16KB
#define FLASH_TOTAL_SECTORS       4               // 总扇区数
#define FLASH_PROGRAM_SIZE        1024            // 编程页大小（避免与HAL冲突）

/* 函数声明 */
HAL_StatusTypeDef FLASH_Init(void);
void FLASH_DeInit(void);
HAL_StatusTypeDef FLASH_EraseSector(FlashSector_t sector);
HAL_StatusTypeDef FLASH_EraseRange(uint32_t start_addr, uint32_t end_addr);
HAL_StatusTypeDef FLASH_Write(uint32_t address, uint8_t *data, uint32_t size);
HAL_StatusTypeDef FLASH_WriteWord(uint32_t address, uint32_t data);
HAL_StatusTypeDef FLASH_WriteHalfWord(uint32_t address, uint16_t data);
HAL_StatusTypeDef FLASH_Read(uint32_t address, uint8_t *data, uint32_t size);
uint32_t FLASH_ReadWord(uint32_t address);
uint16_t FLASH_ReadHalfWord(uint32_t address);
uint8_t FLASH_ReadByte(uint32_t address);
HAL_StatusTypeDef FLASH_Verify(uint32_t address, uint8_t *data, uint32_t size);
uint32_t FLASH_GetSectorAddress(FlashSector_t sector);
FlashSector_t FLASH_GetSectorFromAddress(uint32_t address);
uint32_t FLASH_GetSectorSize(FlashSector_t sector);
HAL_StatusTypeDef STM32_EraseFromPageToEnd(uint16_t start_page);
void FLASH_QuickTest(void);
void FLASH_ShowInfo(void);
#ifdef __cplusplus
}
#endif

#endif /* __FLASH_H */

