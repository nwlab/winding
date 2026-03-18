/**
 * @file rdnx_rtt.c
 * @brief Implementation of RTT (Real-Time Transfer) API using SEGGER RTT.
 * @author Software development team
 * @version 1.0
 * @date 2026-01-30
 *
 * This file provides the implementation of RTT initialization, read, and
 * write functions for use in real-time debugging and trace output.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes */
#include <stddef.h>
#include <string.h>

/* Core includes */
#include "rtt/rdnx_rtt.h"
#include "rdnx_types.h"
#include "rdnx_macro.h"

#include "SEGGER_RTT.h"

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Context structure for an RTT instance.
 *
 * This structure stores information required to manage a single RTT
 * buffer instance. It is used internally and is opaque to the user.
 */
typedef struct rdnx_rtt_ctx
{
    /**
     * @brief Index of the RTT buffer.
     *
     * Used to identify which buffer is associated with this RTT instance.
     */
    uint32_t BufferIndex;
} rdnx_rtt_ctx_t;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

rdnx_err_t rdnx_rtt_init(const rdnx_rtt_params_t* param, rdnx_rtt_handle_t *handle)
{
    if (!param || ! handle)
    {
        return RDNX_ERR_BADARG;
    }

    SEGGER_RTT_Init();
    SEGGER_RTT_ConfigUpBuffer(param->BufferIndex, NULL, param->pBuffer, param->BufferSize, param->Flags);
    SEGGER_RTT_WriteString(param->BufferIndex, "SEGGER Real-Time-Terminal\r\n");

    *handle = RDNX_CAST(rdnx_rtt_handle_t, param->BufferIndex);

    return RDNX_ERR_OK;
}

uint32_t rdnx_rtt_read(rdnx_rtt_handle_t handle, void *pBuffer, unsigned BufferSize)
{
    uint32_t BufferIndex = RDNX_CAST(uint32_t, handle);
    return SEGGER_RTT_Read(BufferIndex, pBuffer, BufferSize);
}

uint32_t rdnx_rtt_write(rdnx_rtt_handle_t handle, const void *pBuffer, unsigned NumBytes)
{
    uint32_t BufferIndex = RDNX_CAST(uint32_t, handle);
    return SEGGER_RTT_Write(BufferIndex, pBuffer, NumBytes);
}
