/**
 * @file rdnx_byteorder.h
 * @author Software development team
 * @brief Byteorder APIs
 * @version 1.0
 * @date 2024-10-10
 */

#ifndef RDNX_BYTEORDER_H
#define RDNX_BYTEORDER_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Core includes. */
#include <rdnx_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

// clang-format off

/* Define a macro that unconditionally swaps */
#define RDNX_SWAP_32(x) \
    (((uint32_t)(x) << 24) | (((uint32_t)(x) & 0xff00) << 8) | \
    (((uint32_t)(x) & 0x00ff0000) >> 8) | ((uint32_t)(x) >> 24))

#define RDNX_SWAP_16(x) \
    ((((uint16_t)(x) & 0xff) << 8) | ((uint16_t)(x) >> 8))

// clang-format on

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

uint32_t htonl(uint32_t net)
{
    return __builtin_bswap32(net);
}

uint16_t htons(uint16_t net)
{
    return __builtin_bswap16(net);
}

uint64_t htonll(uint64_t net)
{
    return __builtin_bswap64(net);
}

#ifdef __cplusplus
}
#endif

#endif // RDNX_BYTEORDER_H
