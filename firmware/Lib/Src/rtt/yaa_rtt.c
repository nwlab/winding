/**
 * @file yaa_rtt.c
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
#include "rtt/yaa_rtt.h"
#include "yaa_types.h"
#include "yaa_macro.h"

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
typedef struct yaa_rtt_ctx
{
    /**
     * @brief Index of the RTT buffer.
     *
     * Used to identify which buffer is associated with this RTT instance.
     */
    uint32_t BufferIndex;
} yaa_rtt_ctx_t;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

yaa_err_t yaa_rtt_init(const yaa_rtt_params_t* param, yaa_rtt_handle_t *handle)
{
    if (!param || ! handle)
    {
        return YAA_ERR_BADARG;
    }

    SEGGER_RTT_Init();
    SEGGER_RTT_ConfigUpBuffer(param->BufferIndex, NULL, param->pBuffer, param->BufferSize, param->Flags);
    SEGGER_RTT_WriteString(param->BufferIndex, "SEGGER Real-Time-Terminal\r\n");

    *handle = YAA_CAST(yaa_rtt_handle_t, param->BufferIndex);

    return YAA_ERR_OK;
}

uint32_t yaa_rtt_read(yaa_rtt_handle_t handle, void *pBuffer, unsigned BufferSize)
{
    uint32_t BufferIndex = YAA_CAST(uint32_t, handle);
    return SEGGER_RTT_Read(BufferIndex, pBuffer, BufferSize);
}

uint32_t yaa_rtt_write(yaa_rtt_handle_t handle, const void *pBuffer, unsigned NumBytes)
{
    uint32_t BufferIndex = YAA_CAST(uint32_t, handle);
    return SEGGER_RTT_Write(BufferIndex, pBuffer, NumBytes);
}
