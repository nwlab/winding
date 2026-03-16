/**
 * @file    yaa_spi.h
 * @author  Software development team
 * @brief   API for SPI buses and peripherals
 * @version 0.1
 * @date    2024-11-04
 */
#ifndef YAA_SPI_H
#define YAA_SPI_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h> // for bool
#include <stddef.h>  // for size_t
#include <stdint.h>

/* Core includes. */
#include <hal/yaa_gpio.h>
#include <yaa_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief SPI bus identifier
 */
typedef enum yaa_spi
{
    YAA_SPI_1,
    YAA_SPI_2,
    YAA_SPI_3,
    YAA_SPI_4,
    YAA_SPI_5,
    YAA_SPI_6,
    /** @brief Total count of SPI bus identifiers */
    YAA_SPI_COUNT
} yaa_spi_t;

/**
 * @brief SPI bus mode
 */
typedef enum yaa_spi_mode
{
    /** @brief The bus is an SPI master device */
    YAA_SPI_MASTER,
    /** @brief The bus is an SPI slave device */
    YAA_SPI_SLAVE
} yaa_spi_mode_t;

/**
 * @brief SPI frame size
 *
 * This is the number of bits per word transferred on the SPI bus.  That is,
 * the granularity of data transferred.
 */
typedef enum yaa_spi_data_frame_size
{
    /** @brief 8-bits per word */
    YAA_SPI_DATA_FRAME_SIZE_8 = 8,
    /** @brief 16-bits per word */
    YAA_SPI_DATA_FRAME_SIZE_16 = 16,
} yaa_spi_data_frame_size_t;

/**
 * @brief SPI clock phasing
 */
typedef enum yaa_spi_clk_phase
{
    /**
     * @brief SPI clock phasing = NCPHA (0)
     *
     * Output data changes on the trailing edge of the preceding clock cycle
     * while input data is read on (or shortly after) the leading edge of the
     * clock cycle.  Output data is held valid until the trailing edge of the
     * current clock cycle.
     */
    YAA_SPI_CLK_PHASE_NCPHA = 0,

    /**
     * @brief SPI clock phasing = CPHA (1)
     *
     * Output data changes on the leading edge of the current clock cycle while
     * input data is read on (or shortly after) the trailing edge of the clock
     * cycle. Output data is held valid until the leading edge of the next
     * clock cycle.
     */
    YAA_SPI_CLK_PHASE_CPHA = 1
} yaa_spi_clk_phase_t;

/**
 * @brief SPI clock polarity
 */
typedef enum yaa_spi_clk_polarity
{
    /**
     * @brief SPI clock polarity = NCPOL (0)
     *
     * Clock is idle when low, active when high. The leading edge is its rising
     * edge (low-to-high transition) and the trailing edge is its falling edge
     * (high-to-low transition).
     */
    YAA_SPI_CLK_POLARITY_NCPOL = 0,

    /**
     * @brief SPI clock polarity = CPOL (1)
     *
     * Clock is idle when high, active when low. The leading edge is its
     * falling edge (high-to-low transition) and the trailing edge is its
     * rising edge (low-to-high transition).
     */
    YAA_SPI_CLK_POLARITY_CPOL = 1
} yaa_spi_clk_polarity_t;

/**
 * @brief SPI chip select (slave select) control type
 */
typedef enum yaa_spi_cs_control
{
    /**
     * @brief Hardware control of chip-select line
     *
     * The SPI bus hardware controls the SPI chip-select line for slave
     * devices.  When hardware control is used, only GPIO pins with hardware CS
     * capabilities may be used for the chip-select line.
     */
    YAA_SPI_CS_HW_CTRL = 0,

    /**
     * @brief Software control of chip-select line
     *
     * The software controls the SPI chip-select line for slave
     * devices.  When software control is used, any GPIO pin may be used for
     * the chip-select line.
     */
    YAA_SPI_CS_SW_CTRL
} yaa_spi_cs_control_t;

/**
 * @brief SPI chip select (slave select) polarity
 */
typedef enum yaa_spi_cs_polarity
{
    /**
     * @brief Normal SPI chip-select polarity
     *
     * Slave devices are enabled by pulling the chip-select line low.  They are
     * disabled by pulling the line high.
     */
    YAA_SPI_CS_NCPOL = 0,

    /**
     * @brief Inverted SPI chip-select polarity
     *
     * Slave devices are enabled by pulling the chip-select line high.  They
     * are disabled by pulling the line low.
     */
    YAA_SPI_CS_CPOL
} yaa_spi_cs_polarity_t;

/**
 * @brief SPI bus device configuration
 *
 * This structure describes the SPI bus device itself.  Peripheral devices
 * attached to the SPI bus are configured separately.  See also
 * #yaa_spi_params_t.
 */
typedef struct yaa_spibus_params
{
    /** @brief SPI bus identifier */
    yaa_spi_t device_id;

    /**
     * @brief The HAL SPI bus handler for specific platform.
     *        See #SPI_HandleTypeDef.
     */
    void *spi;

    /** @brief SPI bus mode */
    yaa_spi_mode_t mode;

    /**
     * @brief Bus frequency
     *
     * The data rate, in bits per second
     */
    uint32_t frequency;

    /**
     * @brief Frame size
     *
     * The size of a single data word (8- or 16-bits)
     *
     * **Note:** This configures the granularity of data transferred, not the
     * amount of data that may be transferred in a single operation.
     */
    yaa_spi_data_frame_size_t data_frame_size;

    /** @brief SPI peripheral clock phasing */
    yaa_spi_clk_phase_t cpha;

    /** @brief SPI peripheral clock polarity */
    yaa_spi_clk_polarity_t cpol;

    /** @brief SPI bus chip select (slave select) control type */
    yaa_spi_cs_control_t cs_type;

    /**
     * @brief Timeout value, in milliseconds, to end synchronous transmit
     *        receive operations
     */
    uint32_t timeout;
} yaa_spibus_params_t;

/**
 * @brief Handle to an initialized SPI bus device
 */
typedef struct yaa_spibus_struct *yaa_spibus_handle_t;

/**
 * @brief SPI peripheral device configuration
 *
 * This structure describes a peripheral device attached to an SPI bus.  The
 * SPI bus itself is configured separately.  See also #yaa_spibus_params_t.
 */
typedef struct yaa_spi_params
{
    /**
     * @brief The chip select GPIO handle
     */
    yaa_gpio_handle_t cs_handle;

    /**
     * @brief Polarity of the chip select signal
     */
    yaa_spi_cs_polarity_t cs_pol;

    /**
     * @brief Delay before transmission and releasing chip select
     *
     * At the start of data transfer, this is the amount of time, in
     * microseconds, between when the module selects the peripheral (via
     * the chip select line) and the start of data transfer.
     *
     * At the end of data transfer, this is the amount of time, in
     * microseconds, between the end of data and when the module releases
     * the peripheral (via the chip select line).
     */
    uint32_t cs_delay;

} yaa_spi_params_t;

/**
 * @brief Handle to an initialized SPI device
 */
typedef struct yaa_spi_struct *yaa_spi_handle_t;

/**
 * @brief Create an SPI bus device
 *
 * @param[in]  bus_params Pointer to memory containing configuration parameters
 *                        for the SPI bus
 * @param[out] bus_handle Pointer to memory which, on success, will contain a
 *                        handle to the configured device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_spibus_init(const yaa_spibus_params_t *bus_params, yaa_spibus_handle_t *bus_handle);

/**
 * @brief Free an SPI bus device.
 *
 * After this call completes, the SPI bus device will be invalid.
 *
 * Attempting to free an SPI bus device while it is in use (by a read or write
 * operation) will result in undefined behavior.
 *
 * @param[in] handle Handle to the SPI bus device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_spibus_free(yaa_spibus_handle_t handle);

/**
 * @brief Create an SPI peripheral device on an SPI bus
 *
 * @param[in]  spi_params Pointer to memory containing configuration parameters
 *                        for the SPI peripheral device
 * @param[in]  bus_handle Handle to the SPI bus device
 * @param[out] spi_handle Pointer to memory which, on success, will contain a
 *                        handle to the configured device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_spi_init(const yaa_spi_params_t *spi_params, yaa_spibus_handle_t bus_handle,
                         yaa_spi_handle_t *spi_handle);

/**
 * @brief Synchronously transmit data to and receive data from an SPI
 *        peripheral device
 *
 * Simultaneously transmit data to and receive data from an SPI peripheral
 * device.
 *
 * If the number of bytes to receive is smaller than the number of bytes to
 * transmit, data received after the requested amount will be discarded.
 *
 * If the number of bytes to transmit is smaller than the number of bytes to
 * receive, after the requested amount of data has been transmitted, subsequent
 * data transmitted will be platform-specific.
 *
 * @param[in]  handle       Handle to the SPI peripheral device
 * @param[in]  tx_data      Pointer to the data to be sent.  The size must be
 *                          at least as large as #tx_data_size.
 * @param[out] rx_data      Pointer to memory which, on completion, will hold
 *                          the received data.  The size must be at least as
 *                          large as #rx_data_size.
 * @param[in]  tx_data_size Number of bytes to transmit
 * @param[in]  rx_data_size Number of bytes to receive
 *
 * @retval #YAA_ERR_OK      If the request was queued for later processing
 * @retval #YAA_ERR_BUSY    The request queue is full, try again later
 */
yaa_err_t yaa_spi_transmitreceive(yaa_spi_handle_t handle, const uint8_t *tx_data, uint8_t *rx_data,
                                    size_t tx_data_size, size_t rx_data_size);

/**
 * @brief Asynchronously transmit data to and receive data from an SPI
 *        peripheral device
 *
 * Simultaneously transmit data to and receive data from an SPI peripheral
 * device.
 *
 * If the number of bytes to receive is smaller than the number of bytes to
 * transmit, data received after the requested amount will be discarded.
 *
 * If the number of bytes to transmit is smaller than the number of bytes to
 * receive, after the requested amount of data has been transmitted, subsequent
 * data transmitted will be platform-specific.
 *
 * @param[in]  handle           Handle to the SPI peripheral device
 * @param[in]  tx_data          Pointer to the data to be sent.  The size must
 *                              be at least as large as #tx_data_size.
 * @param[out] rx_data          Pointer to memory which, on completion, will
 *                              hold the received data.  The size must be at
 *                              least as large as #rx_data_size.
 * @param[in]  tx_data_size     Number of bytes to transmit
 * @param[in]  rx_data_size     Number of bytes to receive
 * @param[in]  callback         Notification object.  The notification will be
 *                              activated when the transmit operation
 * completes.
 * @param[in]  ctx              Notification object.  The notification will be
 *                              activated when the receive operation completes.
 *
 * @retval #YAA_ERR_OK      If the request was queued for later processing
 * @retval #YAA_ERR_PENDING If the request has begun asynchronously
 * @retval #YAA_ERR_BUSY    The request queue is full, try again later
 */
yaa_err_t yaa_spi_transmitreceive_async(yaa_spi_handle_t handle, const uint8_t *tx_data, uint8_t *rx_data,
                                          size_t tx_data_size, size_t rx_data_size, void (*callback)(void *),
                                          void *ctx);

/**
 * @brief Transmit data to and receive data from an SPI
 *        peripheral device
 *
 * @param[in]  handle       Handle to the SPI peripheral device
 * @param[in]  data         Pointer to the data to be sent/receive.
 * @param[in]  data_size    Number of bytes to transmit/receive
 *
 * @retval #YAA_ERR_OK      If the request was queued for later processing
 * @retval #YAA_ERR_BUSY    The request queue is full, try again later
 */
yaa_err_t yaa_spi_write_read(yaa_spi_handle_t handle, uint8_t *data, size_t data_size);

/**
 * @brief Free an SPI peripheral device
 *
 * After this call completes, the SPI peripheral device handle will be invalid.
 *
 * Attempting to release an SPI peripheral device while it is being used for an
 * asynchronous transmit or receive operation will result in undefined
 * behavior.
 *
 * @param[in] handle Handle to the SPI peripheral device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_spi_release(yaa_spi_handle_t handle);

/**
 * @brief Configure hold-mode for a peripheral device's chip select line
 *
 * When set to `true`, the peripheral's chip select line will remain active
 * between sequential data transfer operations on a single device.
 *
 * When set to `true`, the chip select line will only be deactivated when
 * performing a data transfer operation on a different peripheral device.
 *
 * Use of hold-mode may improve performance on some peripheral devices when
 * many operations may be executed in sequence.  Some peripheral devices may
 * require it to implement a communication protocol involving sequences of
 * distinct operations (e.g. a transmit followed by a receive).
 *
 * @param[in] handle Handle to the SPI peripheral device
 * @param[in] hold   `true` to enable chip select hold mode.  `false` to
 *                   disable it.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_spi_cs_hold(yaa_spi_handle_t handle, bool hold);

/**
 * @brief Abort transfer with an SPI peripheral device
 *
 * @param[in] handle Handle to the SPI peripheral device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_spi_abort_transfer(yaa_spi_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // YAA_SPI_H
