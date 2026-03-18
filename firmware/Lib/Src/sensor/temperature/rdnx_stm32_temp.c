/**
 * @file    rdnx_stm32_temp.c
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

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdio.h> // for printf

/* Core includes. */
#include <rdnx_macro.h>
#include <rdnx_types.h>

/* Sensor abstraction */
#include <sensor/rdnx_temperature.h>
#include <sensor/temperature/rdnx_stm32_temp.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

#define TMPSENSOR_USE_INTREF 1 /* 1 - Use Internal Reference Voltage; 0 - Not use; */

/** Temperature sensor slope in millivolts per degree Celsius (mV/°C) */
#define TMPSENSOR_AVGSLOPE_mV 25 // 2.5 mV/°C × 10 to work with centi-degrees

/** Sensor voltage at 25°C in millivolts */
#define TMPSENSOR_V25_mV 760 // 0.76 V × 1000

/** 12-bit ADC maximum value */
#define TMPSENSOR_ADCMAX 4095U

/** Typical reference voltage in millivolts */
#define TMPSENSOR_ADCREFVOL_mV 3300 // 3.3 V × 1000

/** Internal reference voltage in millivolts */
#define TMPSENSOR_ADCVREFINT_mV 1210 // 1.21 V × 1000

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

/* ============================================================================
 * Private Variable Declarations
 * ==========================================================================*/

static uint16_t ADCRaw[2]; /* Raw values from ADC */
static uint8_t ADCCMPLT = 0;

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

static rdnx_err_t stm32_init(void *ctx);
static rdnx_err_t stm32_read(void *ctx, rdnx_temperature_t *value);

/* ============================================================================
 * Global Function Definition
 * ==========================================================================*/

rdnx_temperature_if_t rdnx_stm32_temp_init(const rdnx_stm32_temperature_param_t *param)
{
    rdnx_temperature_if_t iface = {
        .init = stm32_init,
        .read = stm32_read
    };

    iface.ctx = RDNX_CAST(void *, param);

    return iface;
}

/* ============================================================================
 * Private Function Definition
 * ==========================================================================*/

static rdnx_err_t stm32_init(void *ctx)
{
    const rdnx_stm32_temperature_param_t *param = RDNX_CAST(const rdnx_stm32_temperature_param_t *, ctx);

    if (param == NULL || param->hadc == NULL || param->htim == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    HAL_ADC_Start_DMA(param->hadc, (uint32_t *)ADCRaw, 2);
    HAL_TIM_Base_Start(param->htim); /* This timer starts ADC conversion */

    return RDNX_ERR_OK;
}

/**
 * @brief Read STM32 ADC temperature sensor (integer, centi-degrees)
 *
 * @param[in]  ctx   Pointer to STM32 driver context
 * @param[out] value Temperature in centi-degrees (°C × 100)
 *
 * @return RDNX_ERR_OK on success, RDNX_ERR_BUSY if conversion not complete
 */
static rdnx_err_t stm32_read(void *ctx, rdnx_temperature_t *value)
{
    if (!ADCCMPLT)
    {
        return RDNX_ERR_BUSY;
    }

    const rdnx_stm32_temperature_param_t *param = RDNX_CAST(const rdnx_stm32_temperature_param_t *, ctx);
    if (param == NULL || param->hadc == NULL || param->htim == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    /* ---------------------------------------------------------------
     * Convert ADC values to temperature in centi-degrees
     * --------------------------------------------------------------- */

#if (TMPSENSOR_USE_INTREF)
    // Compute Vdda (mV) from internal reference
    uint32_t Vdda_mV = (TMPSENSOR_ADCMAX * TMPSENSOR_ADCVREFINT_mV) / ADCRaw[0];
#else
    uint32_t Vdda_mV = TMPSENSOR_ADCREFVOL_mV; // typical 3300 mV
#endif

    // Sensor voltage in millivolts
    uint32_t sensor_mV = ((uint32_t)ADCRaw[1] * Vdda_mV) / TMPSENSOR_ADCMAX;

    // Temperature in centi-degrees: ((Vsense - V25) / AvgSlope) + 25°C
    int32_t temp_centi = ((int32_t)(sensor_mV - TMPSENSOR_V25_mV) * 100) / TMPSENSOR_AVGSLOPE_mV + 2500;

    // Store result
    *value = (rdnx_temperature_t)temp_centi;

    /* ---------------------------------------------------------------
     * Optional debug print
     * --------------------------------------------------------------- */
    // printf("Reference: ADCRaw[0] = %u\r\n", ADCRaw[0]);
    // printf("Sensor:    ADCRaw[1] = %u\r\n", ADCRaw[1]);
    // printf("Temperature: %d.%02d℃\r\n", (int)(temp_centi / 100), (int)(temp_centi % 100));

    ADCCMPLT = 0; /* Clear flag */
    return RDNX_ERR_OK;
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc)
{
    if (hadc->Instance == ADC1) /* Check if the interrupt comes from ACD1 */
    {
        /* Set flag to true */
        ADCCMPLT = 255;
    }
}
