#ifndef __W25Q_128_H
#define __W25Q_128_H

#include "main.h"
#include "W25Q128_Ins.h"

/*

| 特性          		| **W25Q64**             | **W25Q128**            | 说明                              |
| ------------   	| ---------------------- | ---------------------- | ------------------------------- |
| **容量**       	| 64Mbit = 8MB           | 128Mbit = 16MB         | 容量差一倍                           |
| **页大小**      	| 256 Bytes              | 256 Bytes              | 相同                              |
| **扇区大小**     	| 4KB                    | 4KB                    | 相同                              |
| **块大小**      	| 64KB                   | 64KB                   | 相同                              |
| **扇区数量**     	| 2048 个            	 | 4096 个                 | 随容量翻倍                           |
| **块数量**      	| 128 个                 | 256 个                  | 随容量翻倍                           |
| **最高SPI速度**  	| 104 MHz                | 104 MHz                | 相同                              |
| **工作电压**     	| 2.7V – 3.6V            | 2.7V – 3.6V            | 相同                              |
| **JEDEC ID** 		| `0xEF 0x40 0x17`       | `0xEF 0x40 0x18`       | 区分芯片型号                          |
| **地址长度**     	| 3字节 (24bit 地址，最大 16MB) | 3字节 (24bit 地址，最大 16MB) | **W25Q128 仍可用 3字节寻址**，因为刚好 16MB |

*/


void W25Q128_ReadID(uint8_t *MID, uint16_t *DID);
void W25Q128_WriteEnable(void);
void W25Q128_WaitBusy(void);
void W25Q128_PageProgram(uint32_t Address, uint8_t *DataArray, uint16_t Count);
void W25Q128_SectorErase(uint32_t Address);
void W25Q128_ReadData(uint32_t Address, uint8_t *DataArray, uint32_t Count);
void W25Q128_Test(void);
uint32_t W25Q128_WriteData(uint32_t Address, uint8_t *DataArray, uint32_t Count);
#endif

