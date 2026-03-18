/**
 * @file    rdnx_i2c.h
 * @author  Software development team
 * @brief   API for I2C bus and peripheral devices
 * @version 1.0
 * @date    2024-10-16
 */

#ifndef RDNX_I2C_H
#define RDNX_I2C_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h> // for bool
#include <stddef.h>  // for size_t
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
 * @brief I2C bus identifier
 */
typedef enum rdnx_i2c
{
    RDNX_I2C_1,
    RDNX_I2C_2,
    RDNX_I2C_3,
    /** @brief Total count of I2C bus identifiers */
    RDNX_I2C_COUNT
} rdnx_i2c_t;

/**
 * @brief I2C bus speed.
 */
typedef enum rdnx_i2c_speed
{
    RDNX_I2C_SPEED_NORMAL,
    RDNX_I2C_SPEED_FAST,
    RDNX_I2C_SPEED_FAST_PLUS,
    RDNX_I2C_SPEED_HIGH,
    RDNX_I2C_SPEED_ULTRA,
    RDNX_I2C_SPEED_COUNT
} rdnx_i2c_speed_t;

/**
 * @brief I2C bus mode.
 */
typedef enum rdnx_i2c_mode
{
    RDNX_I2C_MASTER,
    RDNX_I2C_SLAVE
} rdnx_i2c_mode_t;

/**
 * @brief I2C peripheral address size.
 */
typedef enum rdnx_i2c_addr
{
    RDNX_I2C_ADDR_7BIT,  ///< @brief 7-bit I2C peripheral address
    RDNX_I2C_ADDR_10BIT, ///< @brief 10-bit I2C peripheral address
} rdnx_i2c_addr_t;

/**
 * @brief I2C bus device configuration.
 */
typedef struct rdnx_i2c_params
{
    /** @brief Device ID */
    rdnx_i2c_t device_id;

    /**
     * @brief The HAL I@C bus handler for specific platform.
     *        See #I2C_HandleTypeDef.
     */
    void *i2c;

    /**
     * @brief The "speed" mode of the I2C bus.
     *
     * #RDNX_I2C_SPEED_NORMAL.  Normal I2C clock speed: 100 kbit/s
     * #RDNX_I2C_SPEED_FAST.  Fast mode: 400 kbit/s
     * #RDNX_I2C_SPEED_HIGH.  High-speed mode: 3.4 Mbit/s
     */
    rdnx_i2c_speed_t speed;

    /** @brief I2C device mode.  Master or slave.  */
    rdnx_i2c_mode_t mode;

    /** @brief Address size for connected I2C peripherals. */
    rdnx_i2c_addr_t addr;

    /** @brief Enable to use of DMA support for I2C data transfers. */
    bool dma_enable;

    /** @brief RX/TX timeout */
    uint32_t timeout;
} rdnx_i2c_params_t;

/**
 * @brief Handle to an initialized I2C bus device
 */
typedef struct rdnx_i2c_ctx *rdnx_i2c_handle_t;

/**
 * @brief Register size used by an I2C peripheral device.
 */
typedef enum rdnx_i2c_register_size
{
    /**
     * @brief No peripheral register size
     *
     * Used when reading/writing bytes from/to a peripheral device without the
     * register-access interface.
     */
    RDNX_I2C_REGISTER_NONE,

    /** @brief 8-bit peripheral register size */
    RDNX_I2C_REGISTER_SIZE_8,

    /** @brief 16-bit peripheral register size */
    RDNX_I2C_REGISTER_SIZE_16
} rdnx_i2c_register_size_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Create an I2C bus device.
 *
 * @param[in]  params I2C bus configuration parameters
 * @param[out] handle Pointer to memory which, on success, will contain a
 *                    handle to the configured device.
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_i2c_init(const rdnx_i2c_params_t *param, rdnx_i2c_handle_t *handle);

/**
 * @brief Free an I2C bus device.
 *
 * After this call completes, the I2C bus device will be invalid.
 *
 * Attempting to free an I2C bus device while it is in use (by a read or write
 * operation) will result in undefined behavior.
 *
 * @param[in] handle Handle to the I2C bus device
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_i2c_free(rdnx_i2c_handle_t handle);

/**
 * @brief Checks if an I2C device is ready for communication.
 *
 * This function attempts to check if the specified I2C device, identified by
 * its address, is ready to communicate over the I2C bus. It will attempt
 * the specified number of trials within the given timeout period.
 *
 * The function uses the STM32 HAL library's `HAL_I2C_IsDeviceReady` function
 * to perform the readiness check.
 *
 * @param[in] handle        Handle to the I2C bus device
 * @param[in] dev_address   Peripheral device address
 * @param[in] trials        Number of trials
 * @param[in] timeout       The timeout (in ms) to wait for each trial.
 *
 * @return RDNX_ERR_NORESOURCE if there was a timeout during I2C operations,
 *         RDNX_ERR_OK otherwise.
 */
rdnx_err_t rdnx_i2c_isready(rdnx_i2c_handle_t handle, uint16_t dev_address, uint32_t trials, uint32_t timeout);

/**
 * @brief Write data to an I2C peripheral device.
 *
 * This is a synchronous operation that will block until the operation
 * completes.
 *
 * If a register address is provided, data is written to a peripheral device
 * register or sequence of consecutive registers by first writing the register
 * address followed by the data buffer.
 *
 * If a register address is not provided, data is written to the device without
 * first sending a register address.
 *
 * @param[in] handle         Handle to the I2C bus device
 * @param[in] dev_address    Peripheral device address
 * @param[in] reg_address    Peripheral device register to be read
 * @param[in] reg_size       Peripheral device register size.
 *                            #RDNX_I2C_REGISTER_NONE means no register address
 *                            has been provided.
 * @param[in] buffer         Pointer to memory containing the data to write to
 *                           the peripheral device
 * @param[in] size           Number of bytes (in *buffer*) to be written to the
 *                           peripheral device.
 * @param[in] stop_condition If `true`, generate an I2C STOP condition after
 *                           the data has been written.
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_i2c_write(rdnx_i2c_handle_t handle, uint16_t dev_address, uint16_t reg_address,
                          rdnx_i2c_register_size_t reg_size, const uint8_t *buffer, size_t size, bool stop_condition);

/**
 * @brief Read data from an I2C peripheral device.
 *
 * This is a synchronous operation that will block until the operation
 * completes.
 *
 * If a register address is provided, data is read from a peripheral device
 * register or sequence of consecutive registers by first writing the register
 * address without a stop condition and then reading the data buffer.
 *
 * If a register address is not provided, data is read from the device without
 * first sending a register address.
 *
 * @param[in] handle         Handle to the I2C bus device
 * @param[in] dev_address    Peripheral device address
 * @param[in] reg_address    Peripheral device register to be read
 * @param[in] reg_size       Peripheral device register size.
 *                           #RDNX_I2C_REGISTER_NONE means no register address
 *                           has been provided.
 * @param[out] buffer        Pointer to memory which, upon successful
 *                           completion, will contain the received data.
 * @param[in] size           Number of bytes to receive from the peripheral
 *                           device
 * @param[in] stop_condition If `true`, generate an I2C STOP condition after
 *                           the data has been received.
 *
 * @return #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_i2c_read(rdnx_i2c_handle_t handle, uint16_t dev_address, uint16_t reg_address,
                         rdnx_i2c_register_size_t reg_size, uint8_t *buffer, size_t size, bool stop_condition);

/**
 * @brief Set read/write timeout
 *
 * @param[in] handle         Handle to the I2C bus device
 * @param[in] timeout        RX/TX operation timeout
 */
uint8_t rdnx_i2c_set_timeout(rdnx_i2c_handle_t handle, uint32_t timeout);

/**
 * @brief Scans trough the I2C for valid addresses,
 *        then prints the out with printf().
 *
 * @param[in]  handle         Handle to the I2C bus device
 *
 * @return Count of the device on the I2C bus
 */
uint8_t rdnx_i2c_detect(rdnx_i2c_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // RDNX_I2C_H
