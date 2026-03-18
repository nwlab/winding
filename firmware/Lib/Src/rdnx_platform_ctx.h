/**
 * @file    rdnx_platform_ctx.h
 * @author  Software development team
 * @brief   API for platform initialization
 * @version 1.0
 * @date    2024-10-17
 */

#ifndef RDNX_PLATFORM_CTX_H
#define RDNX_PLATFORM_CTX_H

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
#include <rdnx_platform.h>
#include <rdnx_types.h>

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
typedef struct rdnx_platform_ctx
{
    /**
     * @brief Timer handle for udelay timeout function @ref rdnx_udelay.
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

} rdnx_platform_ctx_t;

#ifdef __cplusplus
}
#endif

#endif // RDNX_PLATFORM_CTX_H
