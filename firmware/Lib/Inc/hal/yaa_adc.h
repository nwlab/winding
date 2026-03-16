/**
 * @file    yaa_adc.h
 * @author  Software development team
 * @brief   API for ADC peripheral devices
 * @version 1.0
 * @date    2024-11-10
 */

#ifndef YAA_ADC_H
#define YAA_ADC_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h> // for size_t
#include <stdint.h>

/* Core includes. */
#include <yaa_types.h>

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
typedef enum yaa_adc_channel
{
    YAA_ADC_CHANNEL_0 = 0x00, /*!< Operate with ADC channel 0 */
    YAA_ADC_CHANNEL_1,        /*!< Operate with ADC channel 1 */
    YAA_ADC_CHANNEL_2,        /*!< Operate with ADC channel 2 */
    YAA_ADC_CHANNEL_3,        /*!< Operate with ADC channel 3 */
    YAA_ADC_CHANNEL_4,        /*!< Operate with ADC channel 4 */
    YAA_ADC_CHANNEL_5,        /*!< Operate with ADC channel 5 */
    YAA_ADC_CHANNEL_6,        /*!< Operate with ADC channel 6 */
    YAA_ADC_CHANNEL_7,        /*!< Operate with ADC channel 7 */
    YAA_ADC_CHANNEL_8,        /*!< Operate with ADC channel 8 */
    YAA_ADC_CHANNEL_9,        /*!< Operate with ADC channel 9 */
    YAA_ADC_CHANNEL_10,       /*!< Operate with ADC channel 10 */
    YAA_ADC_CHANNEL_11,       /*!< Operate with ADC channel 11 */
    YAA_ADC_CHANNEL_12,       /*!< Operate with ADC channel 12 */
    YAA_ADC_CHANNEL_13,       /*!< Operate with ADC channel 13 */
    YAA_ADC_CHANNEL_14,       /*!< Operate with ADC channel 14 */
    YAA_ADC_CHANNEL_15,       /*!< Operate with ADC channel 15 */
    YAA_ADC_CHANNEL_16,       /*!< Operate with ADC channel 16 */
    YAA_ADC_CHANNEL_17,       /*!< Operate with ADC channel 17 */
    YAA_ADC_CHANNEL_18,        /*!< Operate with ADC channel 18 */
    /** @brief Total count of ADC channel identifiers */
    YAA_ADC_CHANNEL_COUNT
} yaa_adc_channel_t;

/**
 * @brief Handle to an initialized ADC device
 */
typedef struct yaa_adc_ctx *yaa_adc_handle_t;

/**
 * @brief ADC device configuration.
 */
typedef struct yaa_adc_params
{
    /** @brief Channel ID */
    yaa_adc_channel_t channel_id;

} yaa_adc_params_t;

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
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_adc_init(const yaa_adc_params_t *param, yaa_adc_handle_t *handle);

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
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_adc_free(yaa_adc_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // YAA_ADC_H
