/**
 * @file rdnx_sal.c
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
#include <../Src/rdnx_platform_ctx.h>
#include <rdnx_macro.h>
#include <rdnx_sal.h>
#include <rdnx_types.h>

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

typedef struct rdnx_mutex_struct
{
    uint32_t primask;
    uint32_t lock_count;
} rdnx_mutex_struct_t;

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

extern rdnx_platform_ctx_t platform_ctx;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

#pragma GCC push_options
#pragma GCC optimize("O3")

uint8_t rdnx_udelay(uint32_t delay_us)
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
            return RDNX_ERR_NORESOURCE;
#endif // DWT
        }
    }
#endif // __arm__
    return RDNX_ERR_OK;
}
#pragma GCC pop_options

RDNX_WEAK uint8_t rdnx_mdelay(uint32_t delay_ms)
{
    while (delay_ms > 1000)
    {
        (void)rdnx_udelay(1000);
        delay_ms -= 1000;
    }
    return rdnx_udelay(delay_ms * 1000);
}

RDNX_WEAK void *rdnx_alloc(size_t size)
{
    return malloc(size);
}

RDNX_WEAK void rdnx_free(void *ptr)
{
    free(ptr);
}

RDNX_WEAK uint8_t rdnx_mutex_create(rdnx_mutex_t *mutex)
{
    rdnx_mutex_struct_t *mutex_ctx = rdnx_alloc(sizeof(rdnx_mutex_struct_t));
    if (mutex_ctx == NULL)
    {
        return RDNX_ERR_NOMEM;
    }

    mutex_ctx->primask = 0;
    mutex_ctx->lock_count = 0;

    *mutex = mutex_ctx;

    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_mutex_destroy(rdnx_mutex_t mutex)
{
    if (mutex == NULL)
    {
        return RDNX_ERR_INVAL;
    }

    rdnx_mutex_struct_t *m = (rdnx_mutex_struct_t *)mutex;

    if (m->lock_count != 0)
    {
        return RDNX_ERR_BUSY;
    }

    rdnx_free(m);

    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_mutex_lock(rdnx_mutex_t mutex, uint32_t wait_ms)
{
    RDNX_UNUSED(wait_ms);
    if (mutex == NULL)
    {
        return RDNX_ERR_INVAL;
    }

    rdnx_mutex_struct_t *m = (rdnx_mutex_struct_t *)mutex;

    if (m->lock_count == 0)
    {
        m->primask = __get_PRIMASK();
        __disable_irq();
    }

    m->lock_count++;

    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_mutex_unlock(rdnx_mutex_t mutex)
{
    if (mutex == NULL)
    {
        return RDNX_ERR_INVAL;
    }

    rdnx_mutex_struct_t *m = (rdnx_mutex_struct_t *)mutex;

    if (m->lock_count > 0)
    {
        m->lock_count--;

        if (m->lock_count == 0)
        {
            if (m->primask == 0)
            {
                __enable_irq();
            }
            else
            {
                __disable_irq();
            }
        }
    }

    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_sem_create(rdnx_sem_t *semaphore, uint32_t initial_count, uint32_t max_count)
{
    RDNX_UNUSED(semaphore);
    RDNX_UNUSED(initial_count);
    RDNX_UNUSED(max_count);
    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_sem_destroy(rdnx_sem_t semaphore)
{
    RDNX_UNUSED(semaphore);
    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_sem_take(rdnx_sem_t semaphore, uint32_t wait_ms)
{
    RDNX_UNUSED(semaphore);
    RDNX_UNUSED(wait_ms);
    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_sem_give(rdnx_sem_t semaphore)
{
    RDNX_UNUSED(semaphore);
    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_sem_getcount(rdnx_sem_t semaphore, uint32_t *count)
{
    RDNX_UNUSED(semaphore);
    RDNX_UNUSED(count);
    return RDNX_ERR_OK;
}

RDNX_WEAK uint8_t rdnx_sem_reset(rdnx_sem_t semaphore)
{
    RDNX_UNUSED(semaphore);
    return RDNX_ERR_OK;
}

bool rdnx_is_isr(void)
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
rdnx_boot_reason_t rdnx_boot_reason_get(void)
{
    rdnx_boot_reason_t reset_cause;

    if (__HAL_RCC_GET_FLAG(RCC_FLAG_LPWRRST))
    {
        reset_cause = RDNX_BOOT_REASON_LOW_POWER_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_WWDGRST))
    {
        reset_cause = RDNX_BOOT_REASON_WINDOW_WATCHDOG_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST))
    {
        reset_cause = RDNX_BOOT_REASON_INDEPENDENT_WATCHDOG_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST))
    {
        // This reset is induced by calling the ARM CMSIS
        // `NVIC_SystemReset()` function!
        reset_cause = RDNX_BOOT_REASON_SOFTWARE_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PORRST))
    {
        reset_cause = RDNX_BOOT_REASON_POWER_ON_POWER_DOWN_RESET;
    }
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_PINRST))
    {
        reset_cause = RDNX_BOOT_REASON_EXTERNAL_RESET_PIN_RESET;
    }
    // Needs to come *after* checking the `RCC_FLAG_PORRST` flag in order to
    // ensure first that the reset cause is NOT a POR/PDR reset. See note
    // below.
    else if (__HAL_RCC_GET_FLAG(RCC_FLAG_BORRST))
    {
        reset_cause = RDNX_BOOT_REASON_BROWNOUT_RESET;
    }
    else
    {
        reset_cause = RDNX_BOOT_REASON_UNKNOWN;
    }

    // Clear all the reset flags or else they will remain set during future
    // resets until system power is fully removed.
    __HAL_RCC_CLEAR_RESET_FLAGS();

    return reset_cause;
}

uint8_t rdnx_reboot(void)
{
#if defined(__arm__)
    NVIC_SystemReset();
#endif
    return RDNX_ERR_OK;
}

uint8_t rdnx_device_uuid(uint8_t *uuid)
{
    if (NULL == uuid)
    {
        return RDNX_ERR_BADARG;
    }

    uint32_t *uid = (uint32_t *)uuid;
#if defined(__arm__)
    uid[0] = HAL_GetUIDw0();
    uid[1] = HAL_GetUIDw1();
    uid[2] = HAL_GetUIDw2();
#endif
    return RDNX_ERR_OK;
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
