/**
 * @file yaa_sal.c
 * @author Software development team
 * @brief Hardware application layer implementation for STM32 platform
 * @version 1.0
 * @date 2024-09-12
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdlib.h>

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Core includes. */
#include <../Src/yaa_platform_ctx.h>
#include <yaa_macro.h>
#include <yaa_sal.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

extern yaa_platform_ctx_t platform_ctx;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

#pragma GCC push_options
#pragma GCC optimize("O3")

uint8_t yaa_udelay(uint32_t delay_us)
{
#if defined(__arm__)
    if (delay_us > 0)
    {
        if (platform_ctx.udelay_timer != NULL)
        {
            // start timer
            if (platform_ctx.timer_started == 0)
            {
                HAL_TIM_Base_Start(platform_ctx.udelay_timer);
                platform_ctx.timer_started = ~0;
            }
            // set the counter value a 0
            __HAL_TIM_SET_COUNTER(platform_ctx.udelay_timer, 0);
            // wait for the counter to reach the us input in the parameter
            while (__HAL_TIM_GET_COUNTER(platform_ctx.udelay_timer) < delay_us)
            {
            };
        }
        else
        {
#if defined(DWT)
            volatile uint32_t cycles = (SystemCoreClock / 1000000L) * delay_us;
            if (platform_ctx.timer_started == 0)
            {
                CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
                DWT->CTRL |= 1;
                platform_ctx.timer_started = ~0;
            }
            volatile uint32_t start = DWT->CYCCNT;
            while (DWT->CYCCNT - start < cycles)
            {
            };
#else
            return YAA_ERR_NORESOURCE;
#endif // DWT
        }
    }
#endif // __arm__
    return YAA_ERR_OK;
}
#pragma GCC pop_options

YAA_WEAK uint8_t yaa_mdelay(uint32_t delay_ms)
{
    while (delay_ms > 1000)
    {
        (void)yaa_udelay(1000);
        delay_ms -= 1000;
    }
    return yaa_udelay(delay_ms * 1000);
}

YAA_WEAK void *yaa_alloc(size_t size)
{
    return malloc(size);
}

YAA_WEAK void yaa_free(void *ptr)
{
    free(ptr);
}

YAA_WEAK uint8_t yaa_mutex_create(yaa_mutex_t *mutex)
{
    YAA_UNUSED(mutex);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_mutex_destroy(yaa_mutex_t mutex)
{
    YAA_UNUSED(mutex);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_mutex_lock(yaa_mutex_t mutex, uint32_t wait_ms)
{
    YAA_UNUSED(mutex);
    YAA_UNUSED(wait_ms);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_mutex_unlock(yaa_mutex_t mutex)
{
    YAA_UNUSED(mutex);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_sem_create(yaa_sem_t *semaphore, uint32_t initial_count, uint32_t max_count)
{
    YAA_UNUSED(semaphore);
    YAA_UNUSED(initial_count);
    YAA_UNUSED(max_count);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_sem_destroy(yaa_sem_t semaphore)
{
    YAA_UNUSED(semaphore);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_sem_take(yaa_sem_t semaphore, uint32_t wait_ms)
{
    YAA_UNUSED(semaphore);
    YAA_UNUSED(wait_ms);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_sem_give(yaa_sem_t semaphore)
{
    YAA_UNUSED(semaphore);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_sem_getcount(yaa_sem_t semaphore, uint32_t *count)
{
    YAA_UNUSED(semaphore);
    YAA_UNUSED(count);
    return YAA_ERR_OK;
}

YAA_WEAK uint8_t yaa_sem_reset(yaa_sem_t semaphore)
{
    YAA_UNUSED(semaphore);
    return YAA_ERR_OK;
}

bool yaa_is_isr(void)
{
#if defined(__arm__)
    return (SCB->ICSR & SCB_ICSR_VECTACTIVE_Msk) != 0;
#else
    return false;
#endif
}

/**
 *  @brief      Obtain the STM32 system reset cause
 *  @param      None
 *  @return     The system reset cause
 */
yaa_boot_reason_t yaa_boot_reason_get(void)
{
    yaa_boot_reason_t reset_cause;

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
    {
        reset_cause = YAA_BOOT_REASON_LOW_POWER_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
    {
        reset_cause = YAA_BOOT_REASON_WINDOW_WATCHDOG_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
    {
        reset_cause = YAA_BOOT_REASON_INDEPENDENT_WATCHDOG_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
    {
        // This reset is induced by calling the ARM CMSIS
        // `NVIC_SystemReset()` function!
        reset_cause = YAA_BOOT_REASON_SOFTWARE_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
    {
        reset_cause = YAA_BOOT_REASON_POWER_ON_POWER_DOWN_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
    {
        reset_cause = YAA_BOOT_REASON_EXTERNAL_RESET_PIN_RESET;
    }
    // Needs to come *after* checking the `RCC_FLAG_PORRST` flag in order to
    // ensure first that the reset cause is NOT a POR/PDR reset. See note
    // below.
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))
    {
        reset_cause = YAA_BOOT_REASON_BROWNOUT_RESET;
    }
    else
    {
        reset_cause = YAA_BOOT_REASON_UNKNOWN;
    }

    // Clear all the reset flags or else they will remain set during future
    // resets until system power is fully removed.
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return reset_cause;
}

uint8_t yaa_reboot(void)
{
#if defined(__arm__)
    NVIC_SystemReset();
#endif
    return YAA_ERR_OK;
}

uint8_t yaa_device_uuid(uint8_t *uuid)
{
    if (NULL == uuid)
    {
        return YAA_ERR_BADARG;
    }

    uint32_t *uid = (uint32_t *)uuid;
#if defined(__arm__)
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
#endif
    return YAA_ERR_OK;
}

void HAL_Delay(uint32_t Delay)
{
#if defined(__arm__)
    /* Delay for amount of milliseconds */
    /* Check if we are called from ISR */
    if (__get_IPSR() == 0)
    {
        /* Called from thread mode */
        uint32_t tickstart = HAL_GetTick();

        /* Count interrupts */
        while ((HAL_GetTick() - tickstart) < Delay)
        {
#ifdef DELAY_SLEEP
            /* Go sleep, wait systick interrupt */
            __WFI();
#endif
        }
    }
    else
    {
        /* Called from interrupt mode */
        while (Delay)
        {
            /* Check if timer reached zero after we last checked COUNTFLAG bit */
            if (SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)
            {
                Delay--;
            }
        }
    }
#endif
}
