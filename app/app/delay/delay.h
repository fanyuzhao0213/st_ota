#ifndef __DELAY_H
#define __DELAY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Function prototypes -------------------------------------------------------*/

/**
 * @brief 微秒延时（基于HAL库）
 * @param us 微秒数
 */
void delay_us(uint32_t us);

/**
 * @brief 毫秒延时（基于HAL库）
 * @param ms 毫秒数
 */
void delay_ms(uint32_t ms);

/**
 * @brief 获取系统运行时间（毫秒）
 * @return 系统运行时间（毫秒）
 */
static inline uint32_t Get_Tick_ms(void)
{
    return HAL_GetTick();
}

#ifdef __cplusplus
}
#endif

#endif /* __DELAY_H */

