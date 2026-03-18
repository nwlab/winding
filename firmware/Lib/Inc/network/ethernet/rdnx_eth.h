/**
 * @file    rdnx_eth.h
 * @author  Software development team
 * @brief   Ethernet support functions
 * @version 1.0
 * @date    2026-01-28
 */

#ifndef RDNX_ETH_H
#define RDNX_ETH_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h> // for size_t
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

#define RDNX_NET_ETH_ADDR_LEN       (6U) /**< Ethernet MAC address length */

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Ethernet link state
 */
typedef enum
{
    RDNX_ETH_LINK_DOWN = 0, /**< Link is down */
    RDNX_ETH_LINK_UP   = 1  /**< Link is up   */
} rdnx_eth_link_state_t;

 /** Ethernet address */
typedef struct rdnx_net_eth_addr {
	uint8_t addr[RDNX_NET_ETH_ADDR_LEN]; /**< Buffer storing the address */
} rdnx_net_eth_addr_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

#ifdef __cplusplus
}
#endif

#endif // RDNX_ETH_H
