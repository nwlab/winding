/**
 * @file    rdnx_adc.h
 * @author  Software development team
 * @brief   API for ADC peripheral devices
 * @version 1.0
 * @date    2024-11-10
 */

#ifndef RDNX_ADC_H
#define RDNX_ADC_H

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
 * Public Macro Definitions
 * ==========================================================================*/

/**
 * @brief  Default supply voltage in mV
 */
#ifndef ADC_SUPPLY_VOLTAGE
#define ADC_SUPPLY_VOLTAGE 3300
#endif

/**
 * @brief  Multipliers for VBAT measurement
 */
#ifndef ADC_VBAT_MULTI
#define ADC_VBAT_MULTI 2
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief ADC identifier
 */
typedef enum rdnx_adc_channel
{
    RDNX_ADC_CHANNEL_0 = 0x00, /*!< Operate with ADC channel 0 */
    RDNX_ADC_CHANNEL_1,        /*!< Operate with ADC channel 1 */
    RDNX_ADC_CHANNEL_2,        /*!< Operate with ADC channel 2 */
    RDNX_ADC_CHANNEL_3,        /*!< Operate with ADC channel 3 */
    RDNX_ADC_CHANNEL_4,        /*!< Operate with ADC channel 4 */
    RDNX_ADC_CHANNEL_5,        /*!< Operate with ADC channel 5 */
    RDNX_ADC_CHANNEL_6,        /*!< Operate with ADC channel 6 */
    RDNX_ADC_CHANNEL_7,        /*!< Operate with ADC channel 7 */
    RDNX_ADC_CHANNEL_8,        /*!< Operate with ADC channel 8 */
    RDNX_ADC_CHANNEL_9,        /*!< Operate with ADC channel 9 */
    RDNX_ADC_CHANNEL_10,       /*!< Operate with ADC channel 10 */
    RDNX_ADC_CHANNEL_11,       /*!< Operate with ADC channel 11 */
    RDNX_ADC_CHANNEL_12,       /*!< Operate with ADC channel 12 */
    RDNX_ADC_CHANNEL_13,       /*!< Operate with ADC channel 13 */
    RDNX_ADC_CHANNEL_14,       /*!< Operate with ADC channel 14 */
    RDNX_ADC_CHANNEL_15,       /*!< Operate with ADC channel 15 */
    RDNX_ADC_CHANNEL_16,       /*!< Operate with ADC channel 16 */
    RDNX_ADC_CHANNEL_17,       /*!< Operate with ADC channel 17 */
    RDNX_ADC_CHANNEL_18,        /*!< Operate with ADC channel 18 */
    /** @brief Total count of ADC channel identifiers */
    RDNX_ADC_CHANNEL_COUNT
} rdnx_adc_channel_t;

/**
 * @brief Handle to an initialized ADC device
 */
typedef struct rdnx_adc_ctx *rdnx_adc_handle_t;

/**
 * @brief ADC device configuration.
 */
typedef struct rdnx_adc_params
{
    /** @brief Channel ID */
    rdnx_adc_channel_t channel_id;

} rdnx_adc_params_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Create an ADC device
 *
 * @param[in]  params ADC configuration parameters
 * @param[out] handle Pointer to memory which, on success, will contain a
 *                    handle to the configured device.
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_adc_init(const rdnx_adc_params_t *param, rdnx_adc_handle_t *handle);

/**
 * @brief Free an ADC device
 *
 * After this call completes, the ADC device will be invalid.
 *
 * Attempting to free an ADC device while it is in use (by a read or write
 * operation) will result in undefined behavior.
 *
 * @param[in] handle Handle to the ADC device
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_adc_free(rdnx_adc_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // RDNX_ADC_H
