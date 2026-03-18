/**
 * @file rdnx_tfpt.h
 * @author Software development team
 * @brief TFTP Server
 * @version 0.1
 * @date 2026-03-09
 */
#ifndef _RDNX_TFTP_H
#define _RDNX_TFTP_H

/*
 Update command

 tftp -m binary 10.5.0.105 -c put io_monitor_sign.bin

*/

/* ============================================================================
 * Include Files (for references in this header file)
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h>
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
 * @brief Opaque handle to a TFTP server context.
 *
 * This handle represents an internal TFTP server instance. The actual
 * structure is hidden from the user to keep the API stable.
 */
typedef struct rdnx_tftp_ctx *rdnx_tftp_handle_t;

/**
 * @brief Callback invoked on TFTP RRQ (Read Request).
 *
 * Called when a client requests to read a file from the server.
 *
 * @param filename Requested file name.
 * @param mode Transfer mode (typically "octet" or "netascii").
 *
 * @return
 * - true  Allow the transfer.
 * - false Reject the request.
 */
typedef bool (*rdnx_tftp_rrq_cb)(const char *filename, const char *mode);

/**
 * @brief Callback invoked on TFTP WRQ (Write Request).
 *
 * Called when a client requests to upload a file to the server.
 *
 * @param filename Target file name.
 * @param mode Transfer mode (typically "octet" or "netascii").
 *
 * @return
 * - true  Allow the transfer.
 * - false Reject the request.
 */
typedef bool (*rdnx_tftp_wrq_cb)(const char *filename, const char *mode);

/**
 * @brief Callback invoked when a DATA block is received from the client.
 *
 * Used during WRQ transfers when the client uploads file data.
 *
 * @param data Pointer to received data buffer.
 * @param length Length of received data in bytes (0–512).
 *
 * @return
 * - true  Data accepted.
 * - false Abort transfer.
 */
typedef bool (*rdnx_tftp_data_in_cb)(const uint8_t *data, uint16_t length);

/**
 * @brief Callback invoked when the server needs to send a DATA block.
 *
 * Used during RRQ transfers to obtain the next block of data to send.
 *
 * @param buffer Output buffer where data should be written.
 * @param length Pointer to variable that must be filled with the
 *               number of bytes written to the buffer (0–512).
 *
 * @return
 * - true  Data prepared successfully.
 * - false Abort transfer.
 */
typedef bool (*rdnx_tftp_data_out_cb)(uint8_t *buffer, uint16_t *length);

/**
 * @brief TFTP server configuration parameters.
 *
 * This structure is used to initialize a TFTP server instance and
 * provide callbacks for file transfer operations.
 */
typedef struct
{
    void *eth_handle;              /**< Ethernet driver handle. */
    uint16_t port;                 /**< UDP port used by the TFTP server. */

    rdnx_tftp_rrq_cb rrq;          /**< Read request callback. */
    rdnx_tftp_wrq_cb wrq;          /**< Write request callback. */

    rdnx_tftp_data_in_cb data_in;  /**< Incoming data handler (WRQ). */
    rdnx_tftp_data_out_cb data_out;/**< Outgoing data provider (RRQ). */

} rdnx_tftp_param_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Initialize the TFTP server.
 *
 * This function sets up a TFTP server instance with the user-provided
 * parameters and callbacks. It prepares the UDP socket, initializes
 * the state machine, and clears internal buffers.
 *
 * @param param Pointer to a TFTP parameter structure containing:
 *   - Ethernet handle
 *   - UDP port
 *   - User callbacks for RRQ, WRQ, data in/out
 * @param handle Pointer to a variable that will receive the opaque
 * TFTP context handle on successful initialization.
 *
 * @return
 * - RDNX_ERR_OK on success
 * - RDNX_ERR_BADARG if any pointer is NULL or parameters are invalid
 * - Other negative error codes on failure to allocate resources or
 *   initialize the socket
 *
 * @note The returned handle must be used in subsequent calls to
 * rdnx_tftp_process() and related TFTP API functions.
 * The context should be released with rdnx_tftp_deinit() when done.
 */
rdnx_err_t rdnx_tftp_init(const rdnx_tftp_param_t *param, rdnx_tftp_handle_t *handle);

/**
 * @brief Run the TFTP server loop (blocking or non-blocking)
 * @param handle Pointer to TFTP context
 */
void rdnx_tftp_loop(rdnx_tftp_handle_t handle);

/**
 * @brief Deinitialize / free TFTP server context
 * @param handle Pointer to TFTP context
 */
void rdnx_tftp_deinit(rdnx_tftp_handle_t handle);

#ifdef __cplusplus
}
#endif

#endif // _RDNX_TFTP_H
