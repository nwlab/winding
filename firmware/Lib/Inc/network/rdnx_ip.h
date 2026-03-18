/**
 * @file rdnx_ip.h
 * @author Software development team
 * @brief IP address types
 * @version 0.1
 * @date 2025-01-29
 */
#ifndef RDNX_IP_H
#define RDNX_IP_H

/* ============================================================================
 * Include Files (for references in this header file)
 * ==========================================================================*/

/* Standard includes. */
#include "stdint.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

#define RDNX_NET_IPV4_ADDR_LEN      (4U) /**< IP V4 address length */

/* ============================================================================
 * Public Type Definitions
 * ==========================================================================*/

/**
 * @brief Enumeration of IP version
 */
typedef enum rdnx_ip_version
{
    RDNX_IP_VERSION_V4 = 4,
    RDNX_IP_VERSION_V6 = 6
} rdnx_ip_version_t;

/**
 * @brief Enumeration of IP address type
 */
typedef enum rdnx_ip_address_type
{
    RDNX_IPADDR_TYPE_V4 = (1 << 2),                             /**< IPv4 */
    RDNX_IPADDR_TYPE_V6 = (1 << 3),                             /**< IPv6. Unspecified sub-type */
    RDNX_IPADDR_TYPE_ANY = (RDNX_IPADDR_TYPE_V4 | RDNX_IPADDR_TYPE_V6), /**< IPv4+IPv6 ("dual-stack") */
    RDNX_IPADDR_TYPE_V6_LINK_LOCAL = RDNX_IPADDR_TYPE_V6 + 1,   /**< IPv6. Link local address */
    RDNX_IPADDR_TYPE_V6_SITE_LOCAL = RDNX_IPADDR_TYPE_V6 + 2,   /**< IPv6. Site local address */
    RDNX_IPADDR_TYPE_V6_GLOBAL = RDNX_IPADDR_TYPE_V6 + 3,       /**< IPv6. Global address */
    RDNX_IPADDR_INVALID = 0                                     /**< RDNX_INVALID_IP */
} rdnx_ip_address_type_t;

/**
 * @brief IPv4 address type
 */
typedef union
{
    uint32_t value;   /**< IPv4 address as a uint32_t */
    uint8_t bytes[4]; /**< IPv4 address as uint8_t[4] */
} rdnx_ipv4_address_t;

/**
 * @brief IPv6 address type
 */
typedef union
{
    uint32_t value[4]; /**< IPv6 address as a uint32_t[4] */
    uint8_t bytes[16]; /**< IPv6 address as uint8_t[16] */
} rdnx_ipv6_address_t;

/**
 * @brief Generic IP Address Structure. Supports both IPv4 and IPv6 addresses
 */
typedef struct rdnx_ip_address
{
    union
    {
        rdnx_ipv4_address_t v4;
        rdnx_ipv6_address_t v6;
    } ip;

    rdnx_ip_address_type_t type; /**< IP address type */
} rdnx_ip_address_t;

/**
 * @brief Macro to assist initializing an IPv4 address
 */
#define RDNX_IPV4_ADDRESS(a, b, c, d) \
    {                                 \
        .ip.v4.bytes = {a, b, c, d},  \
        .type = RDNX_IPADDR_TYPE_V4   \
    }

#define RDNX_IP_ADDRESS_CMP(addr1, addr2)                                                   \
    (((addr1)->type != (addr2)->type) ? 0 : (addr1)->type ==  RDNX_IPADDR_TYPE_V6 ?         \
                                            memcmp((addr1)->ip.v6.value,                    \
                                                   (addr2)->ip.v6.value,                    \
                                                   sizeof((addr1)->ip.v6.value) == 0) :     \
                                            (addr1)->ip.v4.value == (addr2)->ip.v4.value)


#ifdef __cplusplus
}
#endif

#endif // RDNX_IP_H
