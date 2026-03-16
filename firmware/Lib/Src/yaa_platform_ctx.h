/**
 * @file    yaa_platform_ctx.h
 * @author  Software development team
 * @brief   API for platform initialization
 * @version 1.0
 * @date    2024-10-17
 */

#ifndef YAA_PLATFORM_CTX_H
#define YAA_PLATFORM_CTX_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h> // for bool
#include <stddef.h>  // for size_t
#include <stdint.h>

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Core includes. */
#include <yaa_platform.h>
#include <yaa_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Platform context.
 */
typedef struct yaa_platform_ctx
{
    /**
     * @brief Timer handle for udelay timeout function @ref yaa_udelay.
     *        See #TIM_HandleTypeDef.
     */
    TIM_HandleTypeDef *udelay_timer;

    /**
     * @brief True if HAL timer #udelay_timer is started.
     */
    uint8_t timer_started;

    /**
     * @brief Get tick in ms. See #HAL_GetTick
     */
    get_tick_func get_tick;

} yaa_platform_ctx_t;

#ifdef __cplusplus
}
#endif

#endif // YAA_PLATFORM_CTX_H
