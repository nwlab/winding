/**
 * @file    rdnx_stm32_temp.h
 * @author  Software development team
 * @brief   STM32 ADC-based temperature sensor driver.
 *
 * This module provides an STM32-specific implementation of the generic
 * temperature sensor interface using the ADC peripheral.
 * It can be used to access the internal temperature sensor or
 * external analog temperature sensors connected to an ADC channel.
 *
 * @version 1.0
 * @date    2026-01-30
 */

#ifndef STM32_TEMP_H
#define STM32_TEMP_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Core includes. */
#include <rdnx_macro.h>
#include <rdnx_types.h>
#include "stm32f4xx_hal.h"   // or generic stm32 header

/* Sensor abstraction */
#include <sensor/rdnx_temperature.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief STM32 temperature sensor configuration parameters.
 *
 * This structure defines the configuration required to initialize
 * an ADC-based temperature sensor on an STM32 device.
 *
 * The ADC instance, sampling time, and calibration must be configured
 * externally before using this driver.
 */
typedef struct
{
    ADC_HandleTypeDef *hadc;

    TIM_HandleTypeDef *htim;

    /**
     * @brief ADC channel used for temperature measurement.
     *
     * This may refer to:
     * - The internal STM32 temperature sensor channel, or
     * - An external analog temperature sensor connected to an ADC pin.
     */
    uint32_t channel;

} rdnx_stm32_temperature_param_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Create an STM32 temperature sensor interface.
 *
 * This function creates and returns a temperature sensor interface
 * compatible with the generic @ref rdnx_temperature_if_t abstraction.
 *
 * The returned interface must be passed to
 * @ref rdnx_temperature_init to obtain a temperature sensor handle.
 *
 * @param[in] param Pointer to STM32 temperature sensor parameters.
 *
 * @return Temperature sensor interface structure.
 *
 * @note This function does not start ADC conversions.
 * @note The ADC peripheral must be initialized before calling this function.
 */
rdnx_temperature_if_t rdnx_stm32_temp_init(const rdnx_stm32_temperature_param_t *param);

#ifdef __cplusplus
}
#endif

#endif /* STM32_TEMP_H */
