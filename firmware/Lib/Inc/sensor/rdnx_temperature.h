/**
 * @file    rdnx_temperature.h
 * @author  Software development team
 * @brief   Generic temperature sensor driver interface.
 *
 * This module provides a hardware-agnostic interface for temperature sensors.
 * Platform- or device-specific drivers must implement the interface callbacks
 * and provide them during initialization.
 *
 * @version 1.0
 * @date    2026-02-03
 */

#ifndef RDNX_TEMPERATURE_H
#define RDNX_TEMPERATURE_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
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

/**
 * @brief Temperature value in degrees Celsius × 100 (centi-degrees).
 *
 * Example:
 * - 37.18°C → 3718
 * - 40.77°C → 4077
 *
 * This allows integer-only calculations while keeping 2 decimal precision.
 */
typedef int16_t rdnx_temperature_t;

/**
 * @brief Temperature sensor driver interface.
 *
 * This structure must be provided by a concrete temperature sensor driver
 * (e.g. STM32 internal sensor, I2C sensor, 1-Wire sensor).
 *
 * The @ref ctx pointer is passed unchanged to all callback functions and
 * typically points to a driver-specific context structure.
 */
typedef struct
{
    /** Driver-specific context */
    void *ctx;

    /**
     * @brief Initialize the temperature sensor.
     *
     * @param[in] ctx Driver-specific context pointer.
     *
     * @return RDNX_OK on success, or an error code on failure.
     */
    rdnx_err_t (*init)(void *ctx);

    /**
     * @brief Read temperature value.
     *
     * @param[in]  ctx   Driver-specific context pointer.
     * @param[out] value Measured temperature in degrees Celsius.
     *
     * @return RDNX_OK on success, or an error code on failure.
     */
    rdnx_err_t (*read)(void *ctx, rdnx_temperature_t *value);

} rdnx_temperature_if_t;

/**
 * @brief Opaque temperature sensor handle.
 *
 * This handle represents an initialized temperature sensor instance.
 * Its internal structure is hidden from the user.
 */
typedef struct rdnx_temperature_ctx *rdnx_temperature_handle_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Initialize a temperature sensor instance.
 *
 * This function binds a temperature sensor interface to an internal
 * driver context and performs sensor initialization.
 *
 * @param[in]  iface  Pointer to a temperature sensor interface.
 * @param[out] handle Pointer to a temperature sensor handle.
 *
 * @return RDNX_OK on success, or an error code on failure.
 */
rdnx_err_t rdnx_temperature_init(const rdnx_temperature_if_t *iface,
                                 rdnx_temperature_handle_t *handle);

/**
 * @brief Read temperature from a sensor.
 *
 * @param[in]  handle Temperature sensor handle.
 * @param[out] value  Measured temperature in degrees Celsius.
 *
 * @return RDNX_OK on success, or an error code on failure.
 */
rdnx_err_t rdnx_temperature_read(rdnx_temperature_handle_t handle,
                                 rdnx_temperature_t *value);

#ifdef __cplusplus
}
#endif

#endif /* RDNX_TEMPERATURE_H */
