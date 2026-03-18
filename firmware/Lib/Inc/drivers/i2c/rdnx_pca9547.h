/**
 * @file    rdnx_pca9547.h
 * @author  Software development team
 * @brief   Driver for PCA9547 I2C multiplexer.
 * @version 1.0
 * @date    2026-02-04
 */

#ifndef RDNX_PCA9547_H
#define RDNX_PCA9547_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Core includes. */
#include <hal/rdnx_i2c.h>
#include <rdnx_macro.h>
#include <rdnx_types.h>
#include <hal/rdnx_gpio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

#define PCA9547_DEFAULT_ADDRESS (0x70 << 1)  // Default I2C address for PCA9547, shifted for 8-bit address

#ifndef PCA9547_TIMEOUT_I2C
#define PCA9547_TIMEOUT_I2C 100 /* timeout in ms for I2C operations */
#endif

/* "PCA9547_TIMEOUT_I2C must be less than 4294967295" */
RDNX_STATIC_ASSERT(PCA9547_TIMEOUT_I2C < UINT32_MAX);
/* "PCA9547_TIMEOUT_I2C cannot be negative" */
RDNX_STATIC_ASSERT(PCA9547_TIMEOUT_I2C > 0);

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Parameters for the PCA9547 multiplexer
 */
typedef struct pca9547_params
{
    /**
     * @brief I2C handle
     */
    rdnx_i2c_handle_t i2c;

    /** @brief GPIO port for reset pin */
    rdnx_gpio_port_t rst_port;

    /** @brief GPIO pin for reset pin */
    rdnx_gpio_pin_t  rst_pin;

    /** @brief GPIO pin inverse */
    bool rst_inverse;

    /**
     * @brief I2C address of the multiplexer
     */
    uint8_t address;
} pca9547_params_t;

/**
 * @brief Handle to an initialized PCA9547 multiplexer
 */
typedef struct pca9547_ctx *pca9547_handle_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

rdnx_err_t pca9547_init(const pca9547_params_t *param, pca9547_handle_t *handle);
rdnx_err_t pca9547_destroy(pca9547_handle_t handle);
rdnx_err_t pca9547_select_channel(pca9547_handle_t handle, uint8_t channel);
rdnx_err_t pca9547_deselect_all(pca9547_handle_t handle);
rdnx_err_t pca9547_reset(pca9547_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // RDNX_PCA9547_H
