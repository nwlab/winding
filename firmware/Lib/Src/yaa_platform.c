/**
 * @file yaa_platform.c
 * @author Software development team
 * @brief Platform implementation for STM32
 * @version 1.0
 * @date 2024-10-17
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
#include <yaa_platform.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

/* ============================================================================
 * Public Variable Declarations
 * ==========================================================================*/

yaa_platform_ctx_t platform_ctx = {
    .get_tick = NULL,
    .udelay_timer = NULL,
    .timer_started = 0,
};

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

yaa_err_t yaa_platform_init(const yaa_platform_params_t *params)
{
    if (params == NULL)
    {
        return YAA_ERR_BADARG;
    }

    platform_ctx.get_tick = params->get_tick;
    platform_ctx.udelay_timer = (TIM_HandleTypeDef *)params->udelay_timer;
    platform_ctx.timer_started = 0;

    return YAA_ERR_OK;
}
