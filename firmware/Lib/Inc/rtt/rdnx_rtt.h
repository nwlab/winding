/**
 * @file rdnx_rtt.h
 * @author Software development team
 * @brief RTT (Real-Time Transfer) API for reading/writing debug or trace data
 * @version 1.0
 * @date 2026-01-30
 */
#ifndef RDNX_RTT_H
#define RDNX_RTT_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes */
#include <stdint.h>

/* Core includes */
#include <rdnx_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/**
 * @brief RTT buffer operating modes.
 *
 * Defines how write operations behave when the RTT buffer
 * does not have enough free space.
 */
typedef enum
{
    /**
     * @brief Non-blocking mode, skip output.
     *
     * The write operation does not block and no data is written
     * if the buffer is full. This is the default behavior.
     */
    RDNX_RTT_MODE_NO_BLOCK_SKIP = 0,

    /**
     * @brief Non-blocking mode, trim output.
     *
     * The write operation does not block and writes as much data
     * as fits into the buffer. Remaining data is discarded.
     */
    RDNX_RTT_MODE_NO_BLOCK_TRIM = 1,

    /**
     * @brief Blocking mode when buffer is full.
     *
     * The write operation blocks until enough space becomes
     * available in the RTT buffer.
     */
    RDNX_RTT_MODE_BLOCK_IF_FIFO_FULL = 2,

    /**
     * @brief Bit mask for RTT mode values.
     *
     * Can be used to extract or validate the RTT mode field
     * from a flags or configuration value.
     */
    RDNX_RTT_MODE_MASK = 0x03

} rdnx_rtt_mode_t;

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Handle to an initialized RTT device.
 *
 * This handle is obtained via @ref rdnx_rtt_init and is used for
 * subsequent read/write operations. The handle must be valid for
 * the lifetime of the RTT session.
 */
typedef struct rdnx_rtt_ctx *rdnx_rtt_handle_t;

/**
 * @brief RTT initialization parameters.
 *
 * This structure holds the configuration parameters required to initialize
 * an RTT instance.
 */
typedef struct rdnx_rtt_params
{
    /**
     * @brief Index of the RTT buffer to use.
     *
     * Some RTT implementations support multiple buffers. This index
     * selects which buffer to read/write.
     */
    uint32_t BufferIndex;

    /**
     * @brief Pointer to the RTT buffer memory.
     *
     * Points to the memory region used for RTT data transfer.
     * The buffer must remain valid for the lifetime of the RTT session.
     */
    void* pBuffer;

    /**
     * @brief Size of the RTT buffer in bytes.
     *
     * Defines the total capacity of the RTT buffer pointed to by
     * @ref pBuffer.
     */
    uint32_t BufferSize;

    /**
     * @brief RTT buffer configuration flags.
     *
     * Flags control buffer behavior such as blocking mode,
     * overwrite policy, or transfer direction, depending on
     * the RTT implementation.
     */
    rdnx_rtt_mode_t Flags;
} rdnx_rtt_params_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Initialize an RTT instance.
 *
 * Sets up the RTT interface using the provided parameters and returns a handle
 * for subsequent read/write operations.
 *
 * @param[in] param Pointer to RTT initialization parameters.
 * @param[out] handle Pointer to a variable that will receive the RTT handle.
 *
 * @return rdnx_err_t Error code indicating success or failure.
 *         - 0: Success
 *         - Non-zero: Initialization failed
 *
 * @note The handle must be freed or invalidated according to the RTT
 *       implementation when no longer needed.
 */
rdnx_err_t rdnx_rtt_init(const rdnx_rtt_params_t* param, rdnx_rtt_handle_t *handle);

/**
 * @brief Read data from the RTT buffer.
 *
 * Reads up to @p BufferSize bytes from the RTT buffer into the provided memory.
 *
 * @param[in] handle RTT handle obtained from @ref rdnx_rtt_init.
 * @param[out] pBuffer Pointer to a buffer where read data will be stored.
 * @param[in] BufferSize Size of the buffer in bytes.
 *
 * @return uint32_t Number of bytes actually read from the RTT buffer.
 *
 * @note The function may return fewer bytes than requested if less data
 *       is available in the buffer.
 */
uint32_t rdnx_rtt_read(rdnx_rtt_handle_t handle, void *pBuffer, unsigned BufferSize);

/**
 * @brief Write data to the RTT buffer.
 *
 * Writes the specified number of bytes from the given buffer to the RTT
 * output buffer.
 *
 * @param[in] handle RTT handle obtained from @ref rdnx_rtt_init.
 * @param[in] pBuffer Pointer to the data to write.
 * @param[in] NumBytes Number of bytes to write from the buffer.
 *
 * @return uint32_t Number of bytes actually written to the RTT buffer.
 *
 * @note The function may write fewer bytes than requested if the buffer
 *       does not have enough space.
 */
uint32_t rdnx_rtt_write(rdnx_rtt_handle_t handle, const void *pBuffer, unsigned NumBytes);

#ifdef __cplusplus
}
#endif

#endif // RDNX_RTT_H
