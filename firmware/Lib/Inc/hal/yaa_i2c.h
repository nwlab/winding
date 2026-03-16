/**
 * @file    yaa_i2c.h
 * @author  Software development team
 * @brief   API for I2C bus and peripheral devices
 * @version 1.0
 * @date    2024-10-16
 */

#ifndef YAA_I2C_H
#define YAA_I2C_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h> // for bool
#include <stddef.h>  // for size_t
#include <stdint.h>

/* Core includes. */
#include <yaa_types.h>

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
typedef enum yaa_i2c
{
    YAA_I2C_1,
    YAA_I2C_2,
    YAA_I2C_3,
    /** @brief Total count of I2C bus identifiers */
    YAA_I2C_COUNT
} yaa_i2c_t;

/**
 * @brief I2C bus speed.
 */
typedef enum yaa_i2c_speed
{
    YAA_I2C_SPEED_NORMAL,
    YAA_I2C_SPEED_FAST,
    YAA_I2C_SPEED_FAST_PLUS,
    YAA_I2C_SPEED_HIGH,
    YAA_I2C_SPEED_ULTRA,
    YAA_I2C_SPEED_COUNT
} yaa_i2c_speed_t;

/**
 * @brief I2C bus mode.
 */
typedef enum yaa_i2c_mode
{
    YAA_I2C_MASTER,
    YAA_I2C_SLAVE
} yaa_i2c_mode_t;

/**
 * @brief I2C peripheral address size.
 */
typedef enum yaa_i2c_addr
{
    YAA_I2C_ADDR_7BIT,  ///< @brief 7-bit I2C peripheral address
    YAA_I2C_ADDR_10BIT, ///< @brief 10-bit I2C peripheral address
} yaa_i2c_addr_t;

/**
 * @brief I2C bus device configuration.
 */
typedef struct yaa_i2c_params
{
    /** @brief Device ID */
    yaa_i2c_t device_id;

    /**
     * @brief The HAL I@C bus handler for specific platform.
     *        See #I2C_HandleTypeDef.
     */
    void *i2c;

    /**
     * @brief The "speed" mode of the I2C bus.
     *
     * #YAA_I2C_SPEED_NORMAL.  Normal I2C clock speed: 100 kbit/s
     * #YAA_I2C_SPEED_FAST.  Fast mode: 400 kbit/s
     * #YAA_I2C_SPEED_HIGH.  High-speed mode: 3.4 Mbit/s
     */
    yaa_i2c_speed_t speed;

    /** @brief I2C device mode.  Master or slave.  */
    yaa_i2c_mode_t mode;

    /** @brief Address size for connected I2C peripherals. */
    yaa_i2c_addr_t addr;

    /** @brief Enable to use of DMA support for I2C data transfers. */
    bool dma_enable;

    /** @brief RX/TX timeout */
    uint32_t timeout;
} yaa_i2c_params_t;

/**
 * @brief Handle to an initialized I2C bus device
 */
typedef struct yaa_i2c_ctx *yaa_i2c_handle_t;

/**
 * @brief Register size used by an I2C peripheral device.
 */
typedef enum yaa_i2c_register_size
{
    /**
     * @brief No peripheral register size
     *
     * Used when reading/writing bytes from/to a peripheral device without the
     * register-access interface.
     */
    YAA_I2C_REGISTER_NONE,

    /** @brief 8-bit peripheral register size */
    YAA_I2C_REGISTER_SIZE_8,

    /** @brief 16-bit peripheral register size */
    YAA_I2C_REGISTER_SIZE_16
} yaa_i2c_register_size_t;

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
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_i2c_init(const yaa_i2c_params_t *param, yaa_i2c_handle_t *handle);

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
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_i2c_free(yaa_i2c_handle_t handle);

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
 * @return YAA_ERR_NORESOURCE if there was a timeout during I2C operations,
 *         YAA_ERR_OK otherwise.
 */
yaa_err_t yaa_i2c_isready(yaa_i2c_handle_t handle, uint16_t dev_address, uint32_t trials, uint32_t timeout);

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
 *                            #YAA_I2C_REGISTER_NONE means no register address
 *                            has been provided.
 * @param[in] buffer         Pointer to memory containing the data to write to
 *                           the peripheral device
 * @param[in] size           Number of bytes (in *buffer*) to be written to the
 *                           peripheral device.
 * @param[in] stop_condition If `true`, generate an I2C STOP condition after
 *                           the data has been written.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_i2c_write(yaa_i2c_handle_t handle, uint16_t dev_address, uint16_t reg_address,
                          yaa_i2c_register_size_t reg_size, const uint8_t *buffer, size_t size, bool stop_condition);

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
 *                           #YAA_I2C_REGISTER_NONE means no register address
 *                           has been provided.
 * @param[out] buffer        Pointer to memory which, upon successful
 *                           completion, will contain the received data.
 * @param[in] size           Number of bytes to receive from the peripheral
 *                           device
 * @param[in] stop_condition If `true`, generate an I2C STOP condition after
 *                           the data has been received.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_i2c_read(yaa_i2c_handle_t handle, uint16_t dev_address, uint16_t reg_address,
                         yaa_i2c_register_size_t reg_size, uint8_t *buffer, size_t size, bool stop_condition);

/**
 * @brief Set read/write timeout
 *
 * @param[in] handle         Handle to the I2C bus device
 * @param[in] timeout        RX/TX operation timeout
 */
uint8_t yaa_i2c_set_timeout(yaa_i2c_handle_t handle, uint32_t timeout);

/**
 * @brief Scans trough the I2C for valid addresses,
 *        then prints the out with printf().
 *
 * @param[in]  handle         Handle to the I2C bus device
 *
 * @return Count of the device on the I2C bus
 */
uint8_t yaa_i2c_detect(yaa_i2c_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // YAA_I2C_H
