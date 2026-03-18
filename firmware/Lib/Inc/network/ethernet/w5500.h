/**
 * @file    w5500.h
 * @author  Software development team
 * @brief   Driver interface for the W5500 Ethernet controller.
 *
 * This module provides initialization, configuration, and link status
 * management for the WIZnet W5500 Ethernet IC. It abstracts low-level SPI
 * communication and GPIO control and integrates with the RDNX Ethernet
 * framework.
 *
 * Supported features:
 * - Static IP configuration
 * - DHCP-based dynamic configuration
 * - Manual chip select, reset, and interrupt handling
 *
 * @version 1.0
 * @date    2026-01-27
 */

#ifndef ETH_W5500_H
#define ETH_W5500_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Core includes. */
#include <rdnx_macro.h>
#include <rdnx_types.h>
#include <rdnx_util.h>
#include <hal/rdnx_spi.h>
#include <network/ethernet/rdnx_eth.h>
#include <network/ethernet/w5500_regs.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/** Maximum SPI frequency supported by W5500 */
#define W5500_SPI_MAX_FREQ_HZ        (18000000U)

#define W5500_CHIP_VERSION   0x04

/* Byte manipulation helpers */
#define W5500_BYTE_HIGH(value)       (((value) >> 8) & 0xFF)
#define W5500_BYTE_LOW(value)        ((value) & 0xFF)

/* SPI Frame Structure Masks */
#define W5500_BSB_MASK               RDNX_GENMASK(7, 3)   /* Block Select Bits */
#define W5500_RWB_MASK               RDNX_BIT(2)          /* Read/Write Bit */
#define W5500_OM_MASK                RDNX_GENMASK(1, 0)   /* Operation Mode Bits */

/* Control Byte Construction Helpers */
#define W5500_BSB(block)             rdnx_field_prep(W5500_BSB_MASK, block)
#define W5500_RWB_READ               0x00
#define W5500_RWB_WRITE              RDNX_BIT(2)
#define W5500_OM_VDM                 0x00    /* Variable Data Length Mode */
#define W5500_OM_FDM_1               0x01    /* Fixed Data Length Mode 1 byte */
#define W5500_OM_FDM_2               0x02    /* Fixed Data Length Mode 2 bytes */
#define W5500_OM_FDM_4               0x03    /* Fixed Data Length Mode 4 bytes */

/* Socket Interrupt Register (Sn_IR) values */
#define W5500_Sn_IR_SEND_OK          RDNX_BIT(4)    /* Send operation completed */
#define W5500_Sn_IR_TIMEOUT          RDNX_BIT(3)    /* Timeout occurred */
#define W5500_Sn_IR_RECV             RDNX_BIT(2)    /* Data received */
#define W5500_Sn_IR_DISCON           RDNX_BIT(1)    /* Connection termination requested/completed */
#define W5500_Sn_IR_CON              RDNX_BIT(0)    /* Connection established */

/* Interrupt Register (IR) values */
#define W5500_IR_CONFLICT            RDNX_BIT(7)    /* IP address conflict */
#define W5500_IR_UNREACH             RDNX_BIT(6)    /* Destination unreachable */
#define W5500_IR_PPPOE               RDNX_BIT(5)    /* PPPoE connection closed */
#define W5500_IR_MP                  RDNX_BIT(4)    /* Magic packet received */

/* PHY Configuration Register (PHYCFGR) values */
#define W5500_PHYCFGR_RST            RDNX_BIT(7)    /* PHY Reset */
#define W5500_PHYCFGR_OPMD           RDNX_BIT(6)    /* Operation Mode Select */
#define W5500_PHYCFGR_OPMDC          RDNX_GENMASK(5, 3)    /* Operation Mode Configuration */
#define W5500_PHYCFGR_DPX            RDNX_BIT(2)    /* Duplex Status (1=Full, 0=Half) */
#define W5500_PHYCFGR_SPD            RDNX_BIT(1)    /* Speed Status (1=100Mbps, 0=10Mbps) */
#define W5500_PHYCFGR_LNK            RDNX_BIT(0)    /* Link Status (1=Up, 0=Down) */

/* PHYCFGR Operation Mode Configuration values */
#define W5500_PHYCFGR_OPMDC_10BT_HD      rdnx_field_prep(W5500_PHYCFGR_OPMDC, 0)
/* 10BT Half-duplex, no auto-nego */
#define W5500_PHYCFGR_OPMDC_10BT_FD      rdnx_field_prep(W5500_PHYCFGR_OPMDC, 1)
/* 10BT Full-duplex, no auto-nego */
#define W5500_PHYCFGR_OPMDC_100BT_HD     rdnx_field_prep(W5500_PHYCFGR_OPMDC, 2)
/* 100BT Half-duplex, no auto-nego */
#define W5500_PHYCFGR_OPMDC_100BT_FD     rdnx_field_prep(W5500_PHYCFGR_OPMDC, 3)
/* 100BT Full-duplex, no auto-nego */
#define W5500_PHYCFGR_OPMDC_100BT_HD_AN  rdnx_field_prep(W5500_PHYCFGR_OPMDC, 4)
/* 100BT Half-duplex, with auto-nego */
#define W5500_PHYCFGR_OPMDC_POWER_DOWN   rdnx_field_prep(W5500_PHYCFGR_OPMDC, 6)
/* Power down mode */
#define W5500_PHYCFGR_OPMDC_ALL_AN       rdnx_field_prep(W5500_PHYCFGR_OPMDC, 7)
/* All capable, with auto-nego */

/* Socket Buffer Size values (2^n KB) */
#define W5500_SOCK_BUF_SIZE_0K       0x00    /* 0KB buffer size */
#define W5500_SOCK_BUF_SIZE_1K       0x01    /* 1KB buffer size */
#define W5500_SOCK_BUF_SIZE_2K       0x02    /* 2KB buffer size (default) */
#define W5500_SOCK_BUF_SIZE_4K       0x04    /* 4KB buffer size */
#define W5500_SOCK_BUF_SIZE_8K       0x08    /* 8KB buffer size */
#define W5500_SOCK_BUF_SIZE_16K      0x10    /* 16KB buffer size */

/* Maximum socket number */
#define W5500_MAX_SOCK_NUMBER        7       /* 8 sockets (0-7) */

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Ethernet IP configuration mode.
 */
typedef enum
{
   ETH_STATIC = 1,    /**< Static IP configuration (manual network settings). */
   ETH_DHCP            /**< Dynamic IP configuration using a DHCP server. */
} eth_dhcp_mode_t;

/**
 * @brief W5500 hardware and network configuration parameters.
 *
 * This structure defines all GPIO connections and network parameters
 * required to initialize and operate a W5500 device instance.
 *
 * All GPIO pins must be configured as outputs (CS, RST) or inputs (IRQ)
 * by the platform layer before calling eth_w5500_init().
 */
typedef struct eth_w5500_params
{
    /**
     * @brief SPI bus handle.
     *
     * SPI interface used for communication with the W5500 device.
     * Must be initialized prior to driver initialization.
     */
    rdnx_spi_handle_t spi;

    /** @brief GPIO port for SPI chip select (CS). */
    rdnx_gpio_port_t cs_port;

    /** @brief GPIO pin for SPI chip select (CS). */
    rdnx_gpio_pin_t  cs_pin;

    /** @brief GPIO port for hardware reset (RST). */
    rdnx_gpio_port_t rst_port;

    /** @brief GPIO pin for hardware reset (RST). */
    rdnx_gpio_pin_t  rst_pin;

    /** @brief GPIO port for interrupt signal (IRQ). */
    rdnx_gpio_port_t irq_port;

    /** @brief GPIO pin for interrupt signal (IRQ). */
    rdnx_gpio_pin_t  irq_pin;

    /** @brief Device MAC address (6 bytes). */
    uint8_t mac[RDNX_NET_ETH_ADDR_LEN];

    /** @brief Static IP address (used when DHCP is disabled). */
    uint8_t ip[4];

    /** @brief Subnet mask. */
    uint8_t subnet[4];

    /** @brief Gateway IP address. */
    uint8_t gw[4];

    /** @brief DNS server IP address. */
    uint8_t dns[4];

    /**
     * @brief IP configuration mode.
     *
     * - ETH_STATIC: use manually configured IP parameters
     * - ETH_DHCP:   obtain network configuration from a DHCP server
     */
    eth_dhcp_mode_t dhcp;

    /** @brief TCP retransmission timeout (RTR register) */
    uint16_t retry_time;

    /** @brief TCP retransmission count (RCR register) */
    uint8_t  retry_count;

	/**
	 *  @brief Max buffer size for incoming data.
	 *  If set to 0, default value will be used:
	 *  RDNX_DEFAULT_CONNECTION_BUFFER_SIZE
	 */
	uint32_t max_buff_size;
} eth_w5500_params_t;

/**
 * @brief Opaque handle to a W5500 device instance.
 *
 * The handle is created by eth_w5500_init() and must be destroyed
 * using eth_w5500_destroy() when no longer needed.
 */
typedef struct eth_w5500_ctx *eth_w5500_handle_t;

/* ============================================================================
 * Global Function Declarations
 * ==========================================================================*/

/**
 * @brief Initialize a W5500 Ethernet device.
 *
 * This function performs hardware reset, configures SPI callbacks,
 * initializes the W5500 chip, and applies network settings according
 * to the provided parameters.
 *
 * @param[in]  param       Pointer to W5500 configuration parameters.
 * @param[out] handle      Pointer to receive the created W5500 device handle.
 *
 * @retval RDNX_ERR_OK             Initialization successful.
 * @retval RDNX_ERR_BADARG         Invalid parameter.
 * @retval RDNX_ERR_NOMEM          Memory allocation failed.
 * @retval RDNX_ERR_FAIL           Hardware or communication error.
 */
rdnx_err_t eth_w5500_init(const eth_w5500_params_t *param,
                          eth_w5500_handle_t *handle);

/**
 * @brief Update and retrieve Ethernet link status.
 *
 * Queries the W5500 PHY status and updates the internal link state.
 *
 * @param[in]  handle  W5500 device handle.
 * @param[out] up      Optional pointer to receive link state:
 *                     - 1: link is up
 *                     - 0: link is down
 *                     Can be NULL if the status value is not required.
 *
 * @retval RDNX_ERR_OK     Operation successful.
 * @retval RDNX_ERR_BADARG Invalid handle.
 */
rdnx_err_t eth_w5500_update_link_status(eth_w5500_handle_t handle, rdnx_eth_link_state_t *up);

/**
 * @brief Deinitialize and release a W5500 device instance.
 *
 * Frees all allocated resources and invalidates the device handle.
 * After this call, the handle must not be used again.
 *
 * @param[in] handle W5500 device handle.
 *
 * @retval RDNX_ERR_OK          Destruction successful.
 * @retval RDNX_ERR_BADARG      Invalid handle.
 */
rdnx_err_t eth_w5500_destroy(eth_w5500_handle_t handle);

/**
 * @brief Set the MAC address of the W5500 device.
 *
 * Configures the source hardware (MAC) address used by the W5500.
 * The address must be a 6-byte array in standard Ethernet format.
 *
 * @param[in] handle W5500 device handle.
 * @param[in] mac    Pointer to 6-byte MAC address array.
 *
 * @retval RDNX_ERR_OK      MAC address successfully configured.
 * @retval RDNX_ERR_BADARG  Invalid handle or NULL pointer.
 */
rdnx_err_t eth_w5500_set_mac(eth_w5500_handle_t handle, const uint8_t mac[6]);

/**
 * @brief Set the IP address of the W5500 device.
 *
 * Configures the IPv4 address of the device.
 *
 * @param[in] handle W5500 device handle.
 * @param[in] ip     Pointer to 4-byte IPv4 address array.
 *
 * @retval RDNX_ERR_OK      IP address successfully configured.
 * @retval RDNX_ERR_BADARG  Invalid handle or NULL pointer.
 */
rdnx_err_t eth_w5500_set_ip(eth_w5500_handle_t handle, const uint8_t ip[4]);

/**
 * @brief Set the subnet mask of the W5500 device.
 *
 * Configures the IPv4 subnet mask used by the device.
 *
 * @param[in] handle W5500 device handle.
 * @param[in] subnet Pointer to 4-byte subnet mask array.
 *
 * @retval RDNX_ERR_OK      Subnet mask successfully configured.
 * @retval RDNX_ERR_BADARG  Invalid handle or NULL pointer.
 */
rdnx_err_t eth_w5500_set_subnet(eth_w5500_handle_t handle, const uint8_t subnet[4]);

/**
 * @brief Set the gateway address of the W5500 device.
 *
 * Configures the IPv4 gateway address used for outgoing traffic.
 *
 * @param[in] handle  W5500 device handle.
 * @param[in] gateway Pointer to 4-byte gateway address array.
 *
 * @retval RDNX_ERR_OK      Gateway address successfully configured.
 * @retval RDNX_ERR_BADARG  Invalid handle or NULL pointer.
 */
rdnx_err_t eth_w5500_set_gateway(eth_w5500_handle_t handle, const uint8_t gateway[4]);

/**
 * @brief Get the MAC address of the W5500 device.
 *
 * Reads the currently configured MAC address from the device.
 *
 * @param[in]  handle W5500 device handle.
 * @param[out] mac    Pointer to 6-byte buffer to store MAC address.
 *
 * @retval RDNX_ERR_OK      MAC address successfully retrieved.
 * @retval RDNX_ERR_BADARG  Invalid handle or NULL pointer.
 */
rdnx_err_t eth_w5500_get_mac(eth_w5500_handle_t handle, uint8_t mac[6]);

/**
 * @brief Get the IP address of the W5500 device.
 *
 * Reads the currently configured IPv4 address from the device.
 *
 * @param[in]  handle W5500 device handle.
 * @param[out] ip     Pointer to 4-byte buffer to store IPv4 address.
 *
 * @retval RDNX_ERR_OK      IP address successfully retrieved.
 * @retval RDNX_ERR_BADARG  Invalid handle or NULL pointer.
 */
rdnx_err_t eth_w5500_get_ip(eth_w5500_handle_t handle, uint8_t ip[4]);

/**
 * @brief Get the subnet mask of the W5500 device.
 *
 * Reads the currently configured subnet mask from the device.
 *
 * @param[in]  handle W5500 device handle.
 * @param[out] subnet Pointer to 4-byte buffer to store subnet mask.
 *
 * @retval RDNX_ERR_OK      Subnet mask successfully retrieved.
 * @retval RDNX_ERR_BADARG  Invalid handle or NULL pointer.
 */
rdnx_err_t eth_w5500_get_subnet(eth_w5500_handle_t handle, uint8_t subnet[4]);

/**
 * @brief Get the gateway address of the W5500 device.
 *
 * Reads the currently configured gateway address from the device.
 *
 * @param[in]  handle  W5500 device handle.
 * @param[out] gateway Pointer to 4-byte buffer to store gateway address.
 *
 * @retval RDNX_ERR_OK      Gateway address successfully retrieved.
 * @retval RDNX_ERR_BADARG  Invalid handle or NULL pointer.
 */
rdnx_err_t eth_w5500_get_gateway(eth_w5500_handle_t handle, uint8_t gateway[4]);


#ifdef __cplusplus
}
#endif

#endif /* ETH_W5500_H */
