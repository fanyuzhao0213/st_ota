#include "delay.h"

/**
 * @brief 微秒延时
 * @param us 微秒数
 */
 //TODO 有问题待后续解决
void delay_us(uint32_t us)
{
    uint32_t start_tick = DWT->CYCCNT;
    uint32_t delay_ticks = us * (SystemCoreClock / 1000000);
    
    while ((DWT->CYCCNT - start_tick) < delay_ticks) {
        // 空循环等待
    }
}

/**
 * @brief 毫秒延时
 * @param ms 毫秒数
 */
void delay_ms(uint32_t ms)
{
    HAL_Delay(ms);
}

