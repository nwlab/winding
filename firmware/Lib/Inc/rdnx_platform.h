/**
 * @file    rdnx_platform.h
 * @author  Software development team
 * @brief   API for platform initialization
 * @version 1.0
 * @date    2024-10-17
 */

#ifndef RDNX_PLATFORM_H
#define RDNX_PLATFORM_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h> // for size_t
#include <stdint.h>

/* Core includes. */
#include <rdnx_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

typedef uint32_t (*get_tick_func)(void);

/**
 * @brief Platform configuration.
 */
typedef struct rdnx_platform_params
{
    /**
     * @brief Platform id
     */
    uint32_t id;

    /**
     * @brief Platform/ Application name
     */
    const char *name;

    /**
     * @brief Timer handle for udelay timeout function @ref rdnx_udelay.
     *        See #TIM_HandleTypeDef.
     *        Set to NULL to use DWT.
     */
    void *udelay_timer;

    /**
     * @brief Get tick in ms. See #HAL_GetTick
     */
    get_tick_func get_tick;

} rdnx_platform_params_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Initialize platform
 *
 * @param[in]  params Platform configuration
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_platform_init(const rdnx_platform_params_t *params);

#ifdef __cplusplus
}
#endif

#endif // RDNX_PLATFORM_H
