/**
 * @file yaa_ip.h
 * @author Software development team
 * @brief IP address types
 * @version 0.1
 * @date 2025-01-29
 */
#ifndef YAA_IP_H
#define YAA_IP_H

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

#define YAA_NET_IPV4_ADDR_LEN      (4U) /**< IP V4 address length */

/* ============================================================================
 * Public Type Definitions
 * ==========================================================================*/

/**
 * @brief Enumeration of IP version
 */
typedef enum yaa_ip_version
{
    YAA_IP_VERSION_V4 = 4,
    YAA_IP_VERSION_V6 = 6
} yaa_ip_version_t;

/**
 * @brief Enumeration of IP address type
 */
typedef enum yaa_ip_address_type
{
    YAA_IPADDR_TYPE_V4 = (1 << 2),                             /**< IPv4 */
    YAA_IPADDR_TYPE_V6 = (1 << 3),                             /**< IPv6. Unspecified sub-type */
    YAA_IPADDR_TYPE_ANY = (YAA_IPADDR_TYPE_V4 | YAA_IPADDR_TYPE_V6), /**< IPv4+IPv6 ("dual-stack") */
    YAA_IPADDR_TYPE_V6_LINK_LOCAL = YAA_IPADDR_TYPE_V6 + 1,   /**< IPv6. Link local address */
    YAA_IPADDR_TYPE_V6_SITE_LOCAL = YAA_IPADDR_TYPE_V6 + 2,   /**< IPv6. Site local address */
    YAA_IPADDR_TYPE_V6_GLOBAL = YAA_IPADDR_TYPE_V6 + 3,       /**< IPv6. Global address */
    YAA_IPADDR_INVALID = 0                                     /**< YAA_INVALID_IP */
} yaa_ip_address_type_t;

/**
 * @brief IPv4 address type
 */
typedef union
{
    uint32_t value;   /**< IPv4 address as a uint32_t */
    uint8_t bytes[4]; /**< IPv4 address as uint8_t[4] */
} yaa_ipv4_address_t;

/**
 * @brief IPv6 address type
 */
typedef union
{
    uint32_t value[4]; /**< IPv6 address as a uint32_t[4] */
    uint8_t bytes[16]; /**< IPv6 address as uint8_t[16] */
} yaa_ipv6_address_t;

/**
 * @brief Generic IP Address Structure. Supports both IPv4 and IPv6 addresses
 */
typedef struct yaa_ip_address
{
    union
    {
        yaa_ipv4_address_t v4;
        yaa_ipv6_address_t v6;
    } ip;

    yaa_ip_address_type_t type; /**< IP address type */
} yaa_ip_address_t;

/**
 * @brief Macro to assist initializing an IPv4 address
 */
#define YAA_IPV4_ADDRESS(a, b, c, d) \
    {                                 \
        .ip.v4.bytes = {a, b, c, d},  \
        .type = YAA_IPADDR_TYPE_V4   \
    }

#define YAA_IP_ADDRESS_CMP(addr1, addr2)                                                   \
    (((addr1)->type != (addr2)->type) ? 0 : (addr1)->type ==  YAA_IPADDR_TYPE_V6 ?         \
                                            memcmp((addr1)->ip.v6.value,                    \
                                                   (addr2)->ip.v6.value,                    \
                                                   sizeof((addr1)->ip.v6.value) == 0) :     \
                                            (addr1)->ip.v4.value == (addr2)->ip.v4.value)


#ifdef __cplusplus
}
#endif

#endif // YAA_IP_H
