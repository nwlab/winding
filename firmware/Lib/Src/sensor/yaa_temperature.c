/**
 * @file    yaa_temperature.c
 * @author  Software development team
 * @brief   Generic temperature sensor driver implementation.
 *
 * This file implements the common temperature sensor abstraction layer.
 * It binds a concrete temperature sensor interface to a lightweight handle
 * and forwards calls to the underlying driver implementation.
 *
 * @version 1.0
 * @date    2026-01-30
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes */
#include <stddef.h>
#include <string.h>

/* Core includes */
#include "yaa_types.h"
#include "yaa_macro.h"
#include <yaa_sal.h>

/* Module includes */
#include "sensor/yaa_temperature.h"

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Internal temperature sensor context.
 *
 * This structure represents an internal temperature sensor instance.
 * It stores a reference to the driver interface provided during
 * initialization.
 *
 * The structure is opaque to the user and accessed only via
 * @ref yaa_temperature_handle_t.
 */
typedef struct yaa_temperature_ctx
{
    /** Pointer to the temperature sensor driver interface */
    yaa_temperature_if_t iface;

} yaa_temperature_ctx_t;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/**
 * @brief Initialize a temperature sensor instance.
 *
 * This function validates the provided temperature sensor interface,
 * calls the driver-specific initialization routine, and returns an
 * opaque handle representing the initialized sensor.
 *
 * @param[in]  iface  Pointer to a temperature sensor driver interface.
 * @param[out] handle Pointer to a temperature sensor handle.
 *
 * @return YAA_OK on success, or an error code on failure.
 */
yaa_err_t yaa_temperature_init(const yaa_temperature_if_t *iface,
                                 yaa_temperature_handle_t *handle)
{
    if (!iface || !handle)
    {
        return YAA_ERR_BADARG;
    }

    if (iface->init == NULL)
    {
        return YAA_ERR_NORESOURCE;
    }

    yaa_err_t err = iface->init(iface->ctx);

    if (err == YAA_ERR_OK)
    {
        yaa_temperature_ctx_t *ctx = (yaa_temperature_ctx_t *)yaa_alloc(sizeof(yaa_temperature_ctx_t));
        if (ctx == NULL)
        {
            return YAA_ERR_NOMEM;
        }

        ctx->iface = *iface;

        *handle = YAA_CAST(yaa_temperature_handle_t, ctx);
    }

    return err;
}

/**
 * @brief Read temperature value from a sensor.
 *
 * This function forwards the read request to the driver-specific
 * implementation associated with the provided temperature handle.
 *
 * @param[in]  handle Temperature sensor handle.
 * @param[out] value  Measured temperature in degrees Celsius.
 *
 * @return YAA_OK on success, or an error code on failure.
 */
yaa_err_t yaa_temperature_read(yaa_temperature_handle_t handle,
                                 yaa_temperature_t *value)
{
    const yaa_temperature_ctx_t *ctx =
        YAA_CAST(const yaa_temperature_ctx_t *, handle);

    if (!ctx || !value)
    {
        return YAA_ERR_BADARG;
    }

    if (!ctx->iface.read)
    {
        return YAA_ERR_NORESOURCE;
    }

    return ctx->iface.read(ctx->iface.ctx, value);
}
