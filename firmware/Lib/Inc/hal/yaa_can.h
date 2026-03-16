/**
 * @file    yaa_can.h
 * @author  Software development team
 * @brief   API for CAN bus and peripheral devices
 * @version 1.0
 * @date    2024-10-16
 */

#ifndef YAA_CAN_H
#define YAA_CAN_H

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

/* Special address description flags for the CAN_ID */
#define YAA_CAN_EFF_FLAG 0x80000000U /* EFF/SFF is set in the MSB */
#define YAA_CAN_RTR_FLAG 0x40000000U /* remote transmission request */
#define YAA_CAN_ERR_FLAG 0x20000000U /* error frame */

/* Valid bits in CAN ID for frame formats */
#define YAA_CAN_SFF_MASK 0x000007FFU /* standard frame format (SFF) */
#define YAA_CAN_EFF_MASK 0x1FFFFFFFU /* extended frame format (EFF) */
#define YAA_CAN_ERR_MASK 0x1FFFFFFFU /* omit EFF, RTR, ERR flags */

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief CAN bus identifier
 */
typedef enum yaa_can
{
    YAA_CAN_1,
    YAA_CAN_2,
    /** @brief Total count of CAN bus identifiers */
    YAA_CAN_COUNT
} yaa_can_t;

/*
 * Controller Area Network Identifier structure.
 *
 * bit 0-28 : CAN identifier (11/29 bit)
 * bit 29   : error frame flag (0 = data frame, 1 = error frame)
 * bit 30   : remote transmission request flag (1 = rtr frame)
 * bit 31   : frame format flag (0 = standard 11 bit, 1 = extended 29 bit)
 */
typedef uint32_t yaa_canid_t;

/*
 * Controller Area Network Error Frame Mask structure
 *
 * bit 0-28 : Error class mask
 * bit 29-31: Set to zero
 */
typedef uint32_t yaa_can_err_mask_t;

/**
 * struct can_frame - basic CAN frame structure.
 * @can_id:  the CAN ID of the frame and CAN_*_FLAG flags, see above.
 * @can_dlc: the data length field of the CAN frame
 * @data:    the CAN frame payload.
 */
typedef struct yaa_can_frame
{
    yaa_canid_t can_id; /* 32 bit CAN_ID + EFF/RTR/ERR flags */
    uint8_t can_dlc;     /* data length code: 0 .. 8 */
    uint8_t data[8] __attribute__((aligned(8)));
} yaa_can_frame_t;

/**
 * struct can_filter - CAN ID based filter.
 * @can_id:   relevant bits of CAN ID which are not masked out.
 * @can_mask: CAN mask (see description)
 *
 * Description:
 * A filter matches, when
 *
 *          <received_can_id> & mask == can_id & mask
 *
 * The filter can be inverted (YAA_CAN_INV_FILTER bit set in can_id) or it can
 * filter for error frames (YAA_CAN_ERR_FLAG bit set in mask).
 */
typedef struct yaa_can_filter
{
    yaa_canid_t can_id;
    yaa_canid_t can_mask;
} yaa_can_filter_t;

#define YAA_CAN_INV_FILTER 0x20000000U /* to be set in yaa_can_filter.can_id */

/**
 * @brief Handle to an initialized CAN bus device
 */
typedef struct yaa_can_ctx *yaa_can_handle_t;

/**
 * @brief CAN bus device configuration.
 */
typedef struct yaa_can_params
{
    /** @brief Device ID */
    yaa_can_t device_id;

    /**
     * @brief The HAL CAN bus handler for specific platform.
     *        See #CAN_HandleTypeDef.
     */
    void *can_handle;

    /**
     * @brief CAN bus baudrate
     */
    uint32_t baudrate;

    /**
     * @brief Transmit timeout
     */
    uint32_t timeout;

} yaa_can_params_t;

/**
 * @brief   Structure to store status information of CAN driver
 */
typedef struct yaa_can_status_info
{
    /** @brief Number of messages queued for transmission or awaiting
     * transmission completion */
    uint32_t msgs_to_tx;
    /** @brief Number of messages in RX queue waiting to be read */
    uint32_t msgs_to_rx;
    /** @brief Current value of Transmit Error Counter */
    uint32_t tx_error_counter;
    /** @brief Current value of Receive Error Counter */
    uint32_t rx_error_counter;
    /** @brief Number of messages that failed transmissions */
    uint32_t tx_failed_count;
    /** @brief Number of messages that were lost due to a full RX queue */
    uint32_t rx_missed_count;
    /** @brief Number of instances arbitration was lost */
    uint32_t arb_lost_count;
    /** @brief Number of instances a bus error has occurred */
    uint32_t bus_error_count;
} yaa_can_status_info_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Create an CAN bus device
 *
 * @param[in]  params CAN bus configuration parameters
 * @param[out] handle Pointer to memory which, on success, will contain a
 *                    handle to the configured device.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_init(const yaa_can_params_t *param, yaa_can_handle_t *handle);

/**
 * @brief Free an CAN bus device
 *
 * After this call completes, the CAN bus device will be invalid.
 *
 * Attempting to free an CAN bus device while it is in use (by a read or write
 * operation) will result in undefined behavior.
 *
 * @param[in] handle Handle to the CAN bus device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_free(yaa_can_handle_t handle);

/**
 * @brief Set CAN bus filter
 *
 * @param[in] handle        Handle to the CAN bus device.
 * @param[in] bank_num      Specifies the filter bank which will be
 * initialized.
 * @param[in] filter_id     Specifies the filter identification number.
 * @param[in] filter_mask   Specifies the filter mask number or identification
 * number.
 * @param[in] filter_mode   Specifies the filter mode to be initialized.
 * @param[in] filter_scale  Specifies the filter scale.
 * @param[in] fifo          Specifies the FIFO (0 or 1U) which will be assigned
 * to the filter.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_set_filter(yaa_can_handle_t handle, uint8_t bank_num, yaa_canid_t filter_id,
                               yaa_canid_t filter_mask, uint32_t filter_mode, uint32_t filter_scale, uint32_t fifo);

/**
 * @brief   Start the CAN driver
 *
 * @param[in] handle        Handle to the CAN bus device.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_start(yaa_can_handle_t handle);

/**
 * @brief   Stop the CAN driver
 *
 * @param[in] handle        Handle to the CAN bus device.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_stop(yaa_can_handle_t handle);

/**
 * @brief Write data to an CAN peripheral device
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
 * @param[in] handle         Handle to the CAN bus device
 * @param[in] canid          CAN identifier
 * @param[in] buffer         Pointer to memory containing the data to write to
 *                           the peripheral device
 * @param[in] size           Number of bytes (in *buffer*) to be written to the
 *                           peripheral device.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_write(yaa_can_handle_t handle, yaa_canid_t canid, const uint8_t *buffer, size_t size);

/**
 * @brief Read data from an CAN peripheral device
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
 * @param[in]  handle         Handle to the CAN bus device
 * @param[in]  canid          CAN identifier
 * @param[out] buffer         Pointer to memory which, upon successful
 *                            completion, will contain the received data.
 * @param[out] size           Number of bytes to receive from the peripheral
 *                            device
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_read(yaa_can_handle_t handle, yaa_canid_t canid, uint8_t *buffer, size_t *size);

/**
 * @brief Register receive callback
 *
 * @param[in]  handle         Handle to the CAN bus device
 * @param[in]  rx_cb          Callback to be called when the RX event occurs.
 * @param[in]  rx_ctx         Parameter to be passed when the callback is
 * called.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_register_rx_callback(yaa_can_handle_t handle,
                                         void (*rx_cb)(void *context, const yaa_can_frame_t *frame), void *rx_ctx);

/**
 * @brief Register transmit complete callback
 *
 * @param[in]  handle         Handle to the CAN bus device
 * @param[in]  rx_cb          Callback to be called when the TX complete event
 * occurs.
 * @param[in]  rx_ctx         Parameter to be passed when the callback is
 * called.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_register_tx_complete_callback(yaa_can_handle_t handle, void (*tx_complete_cb)(void *context),
                                                  void *tx_complete_ctx);

/**
 * @brief Register transmit abort callback
 *
 * @param[in]  handle         Handle to the CAN bus device
 * @param[in]  rx_cb          Callback to be called when the TX abort event
 * occurs.
 * @param[in]  rx_ctx         Parameter to be passed when the callback is
 * called.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_register_tx_abort_callback(yaa_can_handle_t handle, void (*tx_abort_cb)(void *context),
                                               void *tx_abort_ctx);

/**
 * @brief Register transmit abort callback
 *
 * @param[in]  handle         Handle to the CAN bus device
 * @param[in]  rx_cb          Callback to be called when the TX abort event
 * occurs.
 * @param[in]  rx_ctx         Parameter to be passed when the callback is
 * called.
 *
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_can_register_error_callback(yaa_can_handle_t handle, void (*error_cb)(void *context, uint32_t error),
                                            void *error_ctx);

/**
 * @brief Get hardware handle.
 *        See #CAN_HandleTypeDef.
 *
 * @param[in]  handle         Handle to the CAN bus device
 *
 * @return Pointer to CAN_HandleTypeDef
 */
void *yaa_can_hw_handle(yaa_can_handle_t handle);

/**
 * @brief Get CAN bus counters
 */
yaa_err_t yaa_can_get_status_info(yaa_can_handle_t handle, yaa_can_status_info_t *status_info);

#ifdef __cplusplus
}
#endif

#endif // YAA_CAN_H
