/**
 * @file    w5500_internal.h
 * @author  Software development team
 * @brief   Internal definitions and private interfaces for the W5500 driver.
 *
 * This header contains private macros, internal data structures,
 * and static function declarations used exclusively by the
 * W5500 Ethernet driver implementation.
 *
 * It defines:
 *  - Debug and error logging macros
 *  - Internal socket configuration structures
 *  - Driver context structure
 *  - Low-level register access function prototypes
 *
 * @note This file is intended for internal driver use only and must
 *       not be included by application-level code.
 *
 * @warning The contents of this file may change without notice.
 *          It is not part of the public driver API.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/* Core includes. */
#include <yaa_macro.h>
#include <yaa_types.h>

#include <network/ethernet/w5500.h>
#include <network/yaa_socket.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macros for the W5500 driver. */
#if defined(DEBUG) && 0
    #define W5500_DEB(fmt, ...) printf("[W5500]:" fmt "\n\r", ##__VA_ARGS__)
    #define W5500_ERR(fmt, ...) printf("[W5500](ERROR)(%s:%d):" fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#else
    #define W5500_DEB(fmt, ...)   ((void)0)
    #define W5500_ERR(fmt, ...)   ((void)0)
#endif

/**
 * @brief The port number assigned to any available socket.
 *
 * This constant defines the port number `0xC000` that can be used by sockets
 * when they are configured to dynamically bind to any available port. It is
 * typically used in applications where specific port binding is not necessary
 * and the system assigns an available port automatically.
 *
 * @note This value is often used in UDP socket communication to allow the system
 *       to choose an unused port.
 */
#define SOCK_ANY_PORT_NUM  0xC000

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Internal socket address representation.
 */
struct w5500_socket_address
{
    uint8_t ip[4];     /**< IPv4 address */
    uint8_t port[2];   /**< TCP/UDP port (big-endian) */
};

/**
 * @brief Internal socket configuration descriptor.
 */
struct w5500_socket
{
    uint8_t  in_use;
    uint8_t  protocol;      /**< Socket protocol (TCP/UDP) */
    uint16_t mss;           /**< Maximum Segment Size */
    uint8_t  tx_buf_size;   /**< TX buffer size (kB units) */
    uint8_t  rx_buf_size;   /**< RX buffer size (kB units) */
};

/**
 * @brief Ethernet W5500 driver context.
 *
 * This structure binds the application layer to a specific W5500 Ethernet
 * controller instance. It contains all hardware handles and network
 * configuration required to initialize, control, and operate the W5500
 * device over SPI.
 *
 * One instance of this structure should be created per physical W5500
 * device.
 */
typedef struct eth_w5500_ctx
{
    /**
     * @brief SPI bus handle.
     *
     * SPI interface used for communication with the W5500 controller.
     * Must be initialized before the W5500 driver is started.
     */
    yaa_spi_handle_t spi;

    /**
     * @brief Chip Select (CS) GPIO handle.
     *
     * Controls the SPI chip select line of the W5500.
     * Must be driven low during SPI transactions.
     */
    yaa_gpio_handle_t gpio_cs;

    /**
     * @brief Reset (RST) GPIO handle.
     *
     * Used to perform a hardware reset of the W5500 device.
     * If not required, this pin may be left unconnected and ignored
     * by the driver.
     */
    yaa_gpio_handle_t gpio_reset;

    /**
     * @brief Interrupt (IRQ) GPIO handle.
     *
     * Optional interrupt line from the W5500 for event notification
     * (socket events, link status changes, etc.).
     */
    yaa_gpio_handle_t gpio_int;

    /**
     * @brief Source Mac Address
     */
    uint8_t mac_addr[YAA_NET_ETH_ADDR_LEN];

    /**
     * @brief TCP retransmission timeout (RTR register)
     */
    uint16_t retry_time;

    /**
     * @brief TCP retransmission count (RCR register)
     */
    uint8_t  retry_count;

    /**
     * @brief Internal socket descriptors.
     *
     * Indexed by socket number (0..W5500_MAX_SOCK_NUMBER).
     */
    struct w5500_socket sockets[W5500_MAX_SOCK_NUMBER + 1];

    /**
     *  Buffer size in kb for incoming data.
     */
    uint32_t buff_kb;

    /**
    * @brief The port number used for any available socket.
    *
    * This variable holds the port number used by the socket when it is
    * configured to use any available port for communication. Typically used
    * in cases where the socket binding does not require a specific port,
    * and the system will dynamically assign an available one.
    *
    * @note This is typically used for UDP sockets or when the application
    *       does not require a specific port to bind to.
    */
    uint16_t sock_any_port;
} eth_w5500_ctx_t;

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

/** @defgroup W5500_Internal W5500 Internal Driver Interface
 *  @brief Private driver internals (not part of public API).
 *  @{
 */

yaa_err_t w5500_reg_write(eth_w5500_ctx_t *ctx, uint8_t block, uint16_t addr, const uint8_t *data, uint16_t len);
yaa_err_t w5500_reg_read(eth_w5500_ctx_t *ctx, uint8_t block, uint16_t addr, uint8_t *data, uint16_t len);
yaa_err_t w5500_read_16bit_reg(eth_w5500_ctx_t *dev, uint8_t block, uint16_t addr, uint16_t *value);
void w5500_sockets_init(eth_w5500_ctx_t *ctx);
yaa_err_t w5500_setup(eth_w5500_ctx_t *ctx);
yaa_err_t w5500_reset(eth_w5500_ctx_t *ctx);
yaa_err_t w5500_check_link_status(eth_w5500_ctx_t *dev);
yaa_err_t w5500_tcp_config(eth_w5500_ctx_t *dev, uint16_t retry_time, uint8_t retry_count);
yaa_err_t w5500_set_mac(eth_w5500_ctx_t *ctx, const uint8_t mac[6]);
yaa_err_t w5500_set_ip(eth_w5500_ctx_t *ctx, const uint8_t ip[4]);
yaa_err_t w5500_set_subnet(eth_w5500_ctx_t *ctx, const uint8_t subnet[4]);
yaa_err_t w5500_set_gateway(eth_w5500_ctx_t *ctx, const uint8_t gateway[4]);
yaa_err_t w5500_get_mac(eth_w5500_ctx_t *ctx, uint8_t mac[6]);
yaa_err_t w5500_get_ip(eth_w5500_ctx_t *ctx, uint8_t ip[4]);
yaa_err_t w5500_get_subnet(eth_w5500_ctx_t *ctx, uint8_t subnet[4]);
yaa_err_t w5500_get_gateway(eth_w5500_ctx_t *ctx, uint8_t gateway[4]);
yaa_err_t w5500_socket_reg_write(eth_w5500_ctx_t *dev, uint8_t sock_id, uint16_t reg_offset,
                                  const uint8_t *data, uint16_t len);
yaa_err_t w5500_socket_reg_read(eth_w5500_ctx_t *dev, uint8_t sock_id, uint16_t reg_offset, uint8_t *data,
                                 uint16_t len);
yaa_err_t w5500_socket_read_status(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t *status);
yaa_err_t w5500_socket_command_write(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t val);
yaa_err_t w5500_socket_config_interrupt(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t mask);
uint8_t w5500_socket_get_interrupt(eth_w5500_ctx_t *dev, uint8_t sock_id);
yaa_err_t w5500_socket_clear_interrupt(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t flags);
yaa_err_t w5500_socket_init(eth_w5500_ctx_t *dev, uint8_t sock_id);
yaa_err_t w5500_socket_open(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t protocol, uint8_t buff_size);
yaa_err_t w5500_socket_close(eth_w5500_ctx_t *dev, uint8_t sock_id);
yaa_err_t w5500_socket_connect(eth_w5500_ctx_t *dev, uint8_t sock_id, struct w5500_socket_address *addr);
yaa_err_t w5500_socket_disconnect(eth_w5500_ctx_t *dev, uint8_t sock_id);
yaa_err_t w5500_socket_send(eth_w5500_ctx_t *dev, uint8_t sock_id, const void *buf, uint16_t len);
yaa_err_t w5500_socket_recv(eth_w5500_ctx_t *dev, uint8_t sock_id, void *buf, uint16_t len, uint16_t *received);
yaa_err_t w5500_socket_sendto(eth_w5500_ctx_t *dev, uint8_t sock_id, const void *buf, uint16_t len,
                            struct w5500_socket_address *to);
yaa_err_t w5500_socket_recvfrom(eth_w5500_ctx_t *dev, uint8_t sock_id, void *buf, uint16_t len,
                                 struct w5500_socket_address *from, uint16_t *received);
yaa_err_t w5500_socket_bind(eth_w5500_ctx_t *dev, uint8_t sock_id, uint16_t port);
yaa_err_t w5500_socket_listen(eth_w5500_ctx_t *dev, uint8_t sock_id);
uint8_t w5500_socket_translate_protocol(uint32_t proto);
bool w5500_socket_is_open(uint8_t protocol, uint8_t status);
yaa_err_t w5500_net_find_free_socket(eth_w5500_ctx_t *dev, uint8_t *sock_id);
yaa_err_t w5500_net_addr_net_to_w5500(const yaa_sockaddr_t *src, struct w5500_socket_address *dst);
yaa_err_t w5500_net_addr_w5500_to_net(struct w5500_socket_address *src, yaa_sockaddr_t *dst);

yaa_err_t w5500_global_irq_config(eth_w5500_ctx_t *dev, uint8_t imr);
yaa_err_t w5500_global_irq_clear(eth_w5500_ctx_t *dev, uint8_t imr);
uint8_t w5500_global_irq_get(eth_w5500_ctx_t *dev);
yaa_err_t w5500_socket_irq_config(eth_w5500_ctx_t *dev, uint8_t mask);
yaa_err_t w5500_socket_irq_clear(eth_w5500_ctx_t *dev, uint8_t mask);
uint8_t w5500_socket_irq_get(eth_w5500_ctx_t *dev);

void w5500_dump_reg(eth_w5500_ctx_t *ctx);

/** @} */
