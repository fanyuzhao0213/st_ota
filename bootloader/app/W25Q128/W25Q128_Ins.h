#ifndef __W25Q64_INS_H
#define __W25Q64_INS_H

/* =================== 写控制类 =================== */
#define W25Q64_WRITE_ENABLE                         0x06    // 写使能 (必须在写/擦前执行)
#define W25Q64_WRITE_DISABLE                        0x04    // 写禁止 (禁止写入或擦除)

/* =================== 状态寄存器相关 =================== */
#define W25Q64_READ_STATUS_REGISTER_1               0x05    // 读状态寄存器1 (bit0:忙/空闲, bit1:写使能锁存)
#define W25Q64_READ_STATUS_REGISTER_2               0x35    // 读状态寄存器2 (bit9:QE，四线模式使能位)
#define W25Q64_WRITE_STATUS_REGISTER                0x01    // 写状态寄存器 (设置保护位、QE位等)

/* =================== 写入/编程 =================== */
#define W25Q64_PAGE_PROGRAM                         0x02    // 页编程 (1~256字节，不能跨页)
#define W25Q64_QUAD_PAGE_PROGRAM                    0x32    // 四线页编程 (加快写入速度)

/* =================== 擦除操作 =================== */
#define W25Q64_BLOCK_ERASE_64KB                     0xD8    // 擦除64KB块
#define W25Q64_BLOCK_ERASE_32KB                     0x52    // 擦除32KB块
#define W25Q64_SECTOR_ERASE_4KB                     0x20    // 擦除4KB扇区 (常用，最小擦除单元)
#define W25Q64_CHIP_ERASE                           0xC7    // 整片擦除 (耗时最长)

/* =================== 擦除控制 =================== */
#define W25Q64_ERASE_SUSPEND                        0x75    // 擦除暂停
#define W25Q64_ERASE_RESUME                         0x7A    // 擦除恢复

/* =================== 低功耗/模式控制 =================== */
#define W25Q64_POWER_DOWN                           0xB9    // 进入掉电模式 (省电，功耗低)
#define W25Q64_HIGH_PERFORMANCE_MODE                0xA3    // 高性能模式 (用于高速操作)
#define W25Q64_CONTINUOUS_READ_MODE_RESET           0xFF    // 连续读模式复位

/* =================== ID/设备信息读取 =================== */
#define W25Q64_RELEASE_POWER_DOWN_HPM_DEVICE_ID     0xAB    // 释放掉电/高性能模式 & 读设备ID
#define W25Q64_MANUFACTURER_DEVICE_ID               0x90    // 读取厂家ID + 设备ID
#define W25Q64_READ_UNIQUE_ID                       0x4B    // 读取唯一ID (64位唯一序列号)
#define W25Q64_JEDEC_ID                             0x9F    // 读取 JEDEC 标准ID (MID+DID)

/* =================== 数据读取类 =================== */
#define W25Q64_READ_DATA                            0x03    // 普通读取 (低速)
#define W25Q64_FAST_READ                            0x0B    // 快速读取 (1线+1 dummy)
#define W25Q64_FAST_READ_DUAL_OUTPUT                0x3B    // 双输出快速读 (2线输出)
#define W25Q64_FAST_READ_DUAL_IO                    0xBB    // 双I/O快速读 (2线地址+数据)
#define W25Q64_FAST_READ_QUAD_OUTPUT                0x6B    // 四输出快速读 (4线输出)
#define W25Q64_FAST_READ_QUAD_IO                    0xEB    // 四I/O快速读 (4线地址+数据)
#define W25Q64_OCTAL_WORD_READ_QUAD_IO              0xE3    // 四I/O八字节读取 (高速)

/* =================== 其它 =================== */
#define W25Q64_DUMMY_BYTE                           0xFF    // 空操作字节 (用于时钟占位)

#endif /* __W25Q64_INS_H */
