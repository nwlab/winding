/**
 * @file yaa_wdg.c
 * @author Software development team
 * @brief Watchdog implementation for STM32 platform
 * @version 1.0
 * @date 2024-09-24
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
#include <yaa_macro.h>
#include <yaa_types.h>
#include <yaa_wdg.h>

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

#ifdef HAL_IWDG_MODULE_ENABLED
static IWDG_HandleTypeDef IwdgHandle;
static uint8_t is_setup = 0;
#endif

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint8_t yaa_wdg_init(uint32_t reload_time)
{
#if 0
    // Disable IWDG if core is halted
    DBGMCU->APB1FZ |= DBGMCU_APB1_FZ_DBG_IWDG_STOP;
#endif
#ifdef HAL_IWDG_MODULE_ENABLED
    IwdgHandle.Instance = IWDG;
    IwdgHandle.Init.Prescaler = IWDG_PRESCALER_32; // 32Khz/32 = 1Khz(1ms)
    IwdgHandle.Init.Reload = reload_time;

    if (HAL_IWDG_Init(&IwdgHandle) != HAL_OK)
    {
        return YAA_ERR_FAIL;
    }

    is_setup = ~0;
    return YAA_ERR_OK;
#else
    YAA_UNUSED(reload_time);
    return YAA_ERR_NORESOURCE;
#endif
}

uint8_t yaa_wdg_start(void)
{
#ifdef HAL_IWDG_MODULE_ENABLED
    if (HAL_IWDG_Init(&IwdgHandle) != HAL_OK)
    {
        return YAA_ERR_FAIL;
    }
    return YAA_ERR_OK;
#else
    return YAA_ERR_NORESOURCE;
#endif
}

uint8_t yaa_wdg_refresh(void)
{
#ifdef HAL_IWDG_MODULE_ENABLED
    if (is_setup == 0)
    {
        return YAA_ERR_NOTFOUND;
    }

    if (HAL_IWDG_Refresh(&IwdgHandle) != HAL_OK)
    {
        return YAA_ERR_FAIL;
    }
    return YAA_ERR_OK;
#else
    return YAA_ERR_NORESOURCE;
#endif
}

uint8_t yaa_wdg_get_reset(void)
{
#ifdef HAL_IWDG_MODULE_ENABLED
    if (__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
    {
        // IWDGRST flag set
        // Clear reset flags
        __HAL_RCC_CLEAR_RESET_FLAGS();

        return YAA_ERR_OK;
    }
    else
    {
        // IWDGRST flag is not set
        return YAA_ERR_FAIL;
    }
#else
    return YAA_ERR_NORESOURCE;
#endif
}
