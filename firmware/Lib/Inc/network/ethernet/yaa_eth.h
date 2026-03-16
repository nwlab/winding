/**
 * @file    yaa_eth.h
 * @author  Software development team
 * @brief   Ethernet support functions
 * @version 1.0
 * @date    2026-01-28
 */

#ifndef YAA_ETH_H
#define YAA_ETH_H

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

#define YAA_NET_ETH_ADDR_LEN       (6U) /**< Ethernet MAC address length */

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Ethernet link state
 */
typedef enum
{
    YAA_ETH_LINK_DOWN = 0, /**< Link is down */
    YAA_ETH_LINK_UP   = 1  /**< Link is up   */
} yaa_eth_link_state_t;

 /** Ethernet address */
typedef struct yaa_net_eth_addr {
	uint8_t addr[YAA_NET_ETH_ADDR_LEN]; /**< Buffer storing the address */
} yaa_net_eth_addr_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

#ifdef __cplusplus
}
#endif

#endif // YAA_ETH_H
