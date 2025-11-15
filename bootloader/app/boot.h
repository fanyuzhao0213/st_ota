#ifndef __BOOT_H
#define __BOOT_H

#include "main.h"

typedef void (*pFunction)(void);

#define STM32_OTA_FIRE_NUMBER			11

#define STM32_FLASH_STARTADDR			(0x08000000)                                                 	//STM32 Flash起始地址
#define STM32_PAGE_SIZE					(1024)                                                       	//一页（扇区）大小
#define STM32_PAGE_NUM					(64)                                                         	//总页数（扇区数）
#define STM32_B_PAGE_NUM				(20)                                                         	//bootloader区大小
#define STM32_A_PAGE_NUM				(STM32_PAGE_NUM - STM32_B_PAGE_NUM)                          	//程序块大小
#define STM32_A_START_PAGE				STM32_B_PAGE_NUM                                             	//程序块起始页编号（扇区编号）
#define STM32_A_START_ADDR				(STM32_FLASH_STARTADDR + STM32_B_PAGE_NUM * STM32_PAGE_SIZE) 	//程序块起始地址
#define STM32_A_END_ADDR				(STM32_FLASH_STARTADDR + STM32_PAGE_NUM * STM32_PAGE_SIZE - 1)	//程序块结束地址


#define  UPDATA_A_FLAG      				(0x00000001)        //状态标志位，置位表明需要更新A了
#define  IAP_XMODEMC_FLAG  					(0x00000002)        //状态标志位，置位表明Xmdoem协议第一阶段发送大写C       
#define  IAP_XMODEMD_FLAG   				(0x00000004)        //状态标志位，置位表明Xmdoem协议第二阶段处理数据包     

#pragma pack(push, 1)  // 保存当前对齐方式，设置为1字节对齐
typedef struct{
	uint32_t OTA_flag;
	uint32_t FirmwareLen[STM32_OTA_FIRE_NUMBER];
	uint8_t	OTA_Version[32];
}OTA_Info_CB;

typedef struct{
	uint8_t Updata_A_Buff[STM32_PAGE_SIZE];	//更新A区时，用于保存从W25Q64中读取的数据,c8t6支持的最大flash写入为1k，所以缓存区给1k大小
	uint32_t Updata_A_from_W25Q64_Num;		// 要更新的固件在W25Q64中的分区号
	uint32_t XmodemTimer;                     //用于记录Xmdoem协议第一阶段发送大写C 的时间间隔                   
	uint32_t XmodemNB;                        //用于记录Xmdoem协议接收到多少数据包了
	uint16_t XmodemCRC;	                      //用于保存Xmdoem协议包计算的CRC16校验值
}UpdateA_CB;


#pragma pack(pop)      // 恢复之前的对齐方式

#define OTA_INFOCB_SIZE					(sizeof(OTA_Info_CB))
#define OTA_SET_FLAG	          		(0xAABB1122)
#define W25Q128_LAST_ADDR    			(0xFFFFFF - 255)          		// 最后一个page起始地址

extern OTA_Info_CB OTA_Info;
extern UpdateA_CB UpdateA;

extern uint32_t BootStateFlag;   	//记录全局状态标志位，每位表示1种状态

void BootLoader_Branch(void);
void W25Q128_ReadOTAInfo(void);
uint8_t W25Q128_WriteOTAInfoSafe(void);
void BootLoader_Event(uint8_t *data, uint16_t datalen);
uint16_t Xmodem_CRC16(uint8_t *data, uint16_t datalen);
#endif
