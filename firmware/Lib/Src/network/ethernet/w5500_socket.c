/**
 * @file    w5500_socket.c
 * @author  Software development team
 * @brief   Low-level W5500 socket driver implementation.
 *
 * This file provides functions to manage sockets on the W5500
 * Ethernet controller. It implements operations such as:
 *  - Reading and writing socket registers
 *  - Opening, initializing, and closing sockets
 *  - Connecting, disconnecting, sending, and receiving data
 *  - UDP send/receive operations
 *  - TCP server functions: bind and listen
 *
 * Each logical socket maps to a W5500 hardware socket (0–7).
 * Functions handle translation between generic rdnx socket API
 * and W5500-specific registers and commands.
 *
 * @note The W5500 supports up to 8 hardware sockets. Attempting
 *       to use a socket beyond this limit will result in errors.
 *
 * @warning This driver assumes exclusive access to the W5500 device
 *          and is intended for embedded environments.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h> // for memcpy

/* Core includes. */
#include <hal/rdnx_gpio.h>
#include <hal/rdnx_spi.h>
#include <rdnx_macro.h>
#include <rdnx_sal.h>
#include <rdnx_types.h>

#include <network/ethernet/w5500.h>
#include <network/rdnx_socket.h>
#include "w5500_internal.h"

/******************************************************************************
 * @brief Write to a socket register
 *
 * @param dev        - The device descriptor
 * @param sock_id    - Socket number (0-7)
 * @param reg_offset - Register offset
 * @param data       - Data to write
 * @param len        - Data length
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_reg_write(eth_w5500_ctx_t *dev, uint8_t sock_id, uint16_t reg_offset,
                                         const uint8_t *data, uint16_t len)
{
    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    return w5500_reg_write(dev, W5500_SOCKET_REG_BLOCK(sock_id), reg_offset, data, len);
}

/******************************************************************************
 * @brief Read from a socket register
 *
 * @param dev        - The device descriptor
 * @param sock_id    - Socket number (0-7)
 * @param reg_offset - Register offset
 * @param data       - Buffer to store data
 * @param len        - Data length to read
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_reg_read(eth_w5500_ctx_t *dev, uint8_t sock_id, uint16_t reg_offset, uint8_t *data,
                                        uint16_t len)
{
    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    return w5500_reg_read(dev, W5500_SOCKET_REG_BLOCK(sock_id), reg_offset, data, len);
}

/******************************************************************************
 * @brief Read status of a socket
 *
 * @param dev     - The device descriptor
 * @param sock_id - Socket number (0-7)
 * @param status  - Variable to store status
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_read_status(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t *status)
{
    return w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, status, 1);
}

/******************************************************************************
 * @brief Write a command to a socket's command register
 *
 * @param dev     - The device descriptor
 * @param sock_id - Socket number (0-7)
 * @param val     - Command value to write
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_command_write(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t val)
{
    return w5500_socket_reg_write(dev, sock_id, W5500_Sn_CR, &val, 1);
}

/******************************************************************************
 * @brief Configure socket interrupt mask
 *
 * Enables specific interrupt events for a given socket. Only the enabled
 * events can set the corresponding bits in Sn_IR and propagate to SIR/SIMR.
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 * @param mask    - Interrupt mask to enable
 *
 * Mask bits:
 *   W5500_Sn_IMR_SEND_OK  - Send completed
 *   W5500_Sn_IMR_TIMEOUT  - Timeout occurred
 *   W5500_Sn_IMR_RECV     - Data received
 *   W5500_Sn_IMR_DISCON   - Socket disconnected
 *   W5500_Sn_IMR_CON      - Connection established
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_config_interrupt(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t mask)
{
    return w5500_socket_reg_write(dev, sock_id, W5500_Sn_IMR, &mask, 1);
}

/******************************************************************************
 * @brief Get socket interrupt flags
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 *
 * @return Current interrupt flags (Sn_IR)
 *******************************************************************************/
uint8_t w5500_socket_get_interrupt(eth_w5500_ctx_t *dev, uint8_t sock_id)
{
    uint8_t flags = 0;
    (void)w5500_socket_reg_read(dev, sock_id, W5500_Sn_IR, &flags, 1);
    return flags;
}

/******************************************************************************
 * @brief Clear specific socket interrupt flags
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 * @param flags   - Interrupt flags to clear
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_clear_interrupt(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t flags)
{
    return w5500_socket_reg_write(dev, sock_id, W5500_Sn_IR, &flags, 1);
}

/******************************************************************************
 * @brief Initialize a socket
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_init(eth_w5500_ctx_t *dev, uint8_t sock_id)
{
    rdnx_err_t ret;
    uint8_t mss_buf[2];
    uint8_t port_buf[2];
    struct w5500_socket *sock = &dev->sockets[sock_id];

    mss_buf[0] = W5500_BYTE_HIGH(sock->mss);
    mss_buf[1] = W5500_BYTE_LOW(sock->mss);

    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_MR, &sock->protocol, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    if (sock->protocol == W5500_Sn_MR_TCP)
    {
        ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_MSSR, mss_buf, 2);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to write reg %d", ret);
            return ret;
        }
    }

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_RXBUF_SIZE, &sock->rx_buf_size, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_TXBUF_SIZE, &sock->tx_buf_size, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    port_buf[0] = W5500_BYTE_HIGH(dev->sock_any_port);
    port_buf[1] = W5500_BYTE_LOW(dev->sock_any_port);
    if(++dev->sock_any_port == 0xFFF0)
    {
        dev->sock_any_port = SOCK_ANY_PORT_NUM;
    }

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_PORT, port_buf, 2);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    rdnx_mdelay(10);   // VERY IMPORTANT

    ret = w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_OPEN);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write command %d", ret);
        return ret;
    }

    bool opened = false;
    uint8_t status = W5500_Sn_SR_CLOSED;
    int timeout = 1000;

    while (timeout-- > 0)
    {
        (void)w5500_socket_read_status(dev, sock_id, &status);
        if (w5500_socket_is_open(sock->protocol, status))
        {
            opened = true;
            break;
        }
        rdnx_mdelay(1);
    }

#if 0
    (void)w5500_socket_config_interrupt(dev, sock_id, W5500_Sn_IMR_CON | W5500_Sn_IMR_DISCON);
#endif

    W5500_DEB("Socket %d %s, status 0x%02X, timeout %d",
              sock_id,
              opened ? "opened" : "fail",
              status,
              timeout);

    return (opened ? RDNX_ERR_OK : RDNX_ERR_FAIL);
}

/******************************************************************************
 * @brief Open a socket with specified protocol
 *
 * @param dev       - The device descriptor
 * @param sock_id   - Socket ID to use (0-7)
 * @param protocol     - W5500 mode register protocol
 * @param buff_size - Buffer size (1, 2, 4, 8, 16 KB)
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_open(eth_w5500_ctx_t *dev, uint8_t sock_id, uint8_t protocol, uint8_t buff_size)
{
    RDNX_UNUSED(buff_size);
    struct w5500_socket *sock;
    rdnx_err_t ret;

    if (protocol == W5500_Sn_MR_MACRAW)
    {
        if (sock_id != 0)
        {
            W5500_ERR("Bad arguments");
            return RDNX_ERR_BADARG;
        }
    }
    else if (protocol != W5500_Sn_MR_UDP && protocol != W5500_Sn_MR_TCP)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    sock = &dev->sockets[sock_id];

    sock->protocol = protocol;
    sock->tx_buf_size = 2; // buff_size
    sock->rx_buf_size = 2; // buff_size

    ret = w5500_socket_init(dev, sock_id);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to init socket %d", (int)ret);
        sock->protocol = W5500_Sn_MR_CLOSE;
        return ret;
    }

    return RDNX_ERR_OK;
}

/******************************************************************************
 * @brief Close a socket
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_close(eth_w5500_ctx_t *dev, uint8_t sock_id)
{
    rdnx_err_t ret;

    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_CLOSE);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to close socket %d", (int)ret);
        return ret;
    }

    return RDNX_ERR_OK;
}

/******************************************************************************
 * @brief Connect to a remote host (TCP client mode)
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 * @param addr    - Socket address structure containing remote IP and port
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_connect(eth_w5500_ctx_t *dev, uint8_t sock_id, struct w5500_socket_address *addr)
{
    int ret;
    uint8_t status;

    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    if (!addr)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (status != W5500_Sn_SR_INIT)
    {
        W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_DIPR, addr->ip, 4);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_DPORT, addr->port, 2);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    ret = w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_CONNECT);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write command %d", ret);
        return ret;
    }

    return RDNX_ERR_OK;
}

/******************************************************************************
 * @brief Disconnect a TCP connection
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 *
 * @return 0 on success, -ENOTCONN if not connected, other negative error codes
 *******************************************************************************/
rdnx_err_t w5500_socket_disconnect(eth_w5500_ctx_t *dev, uint8_t sock_id)
{
    rdnx_err_t ret;
    uint8_t status;

    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (status != W5500_Sn_SR_ESTABLISHED && status != W5500_Sn_SR_CLOSE_WAIT)
    {
        W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
        return RDNX_ERR_IO;
    }

    return w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_DISCON);
}

/******************************************************************************
 * @brief Send data through a socket
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 * @param buf     - Data buffer to send
 * @param len     - Data length
 *
 *******************************************************************************/
rdnx_err_t w5500_socket_send(eth_w5500_ctx_t *dev, uint8_t sock_id, const void *buf, uint16_t len)
{
    rdnx_err_t ret;
    uint16_t free_size = 0;
    uint16_t wr_ptr;
    uint8_t wr_ptr_buf[2];
    uint8_t status;
    uint16_t total_sent = 0;
    uint16_t chunk_size;
    const uint8_t *data = (const uint8_t *)buf;

    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (status != W5500_Sn_SR_ESTABLISHED && status != W5500_Sn_SR_UDP && status != W5500_Sn_SR_MACRAW)
    {
        W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
        return RDNX_ERR_IO;
    }

    while (total_sent < len)
    {
        ret = w5500_read_16bit_reg(dev, W5500_SOCKET_REG_BLOCK(sock_id), W5500_Sn_TX_FSR, &free_size);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to read reg %d", ret);
            return ret;
        }

        if (free_size == 0)
        {
            ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
            if (ret != RDNX_ERR_OK)
            {
                W5500_ERR("Fail to read reg %d", ret);
                return ret;
            }

            if (status != W5500_Sn_SR_ESTABLISHED && status != W5500_Sn_SR_UDP && status != W5500_Sn_SR_MACRAW)
            {
                W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
                return RDNX_ERR_IO;
            }

            rdnx_mdelay(1);
            continue;
        }

        chunk_size = (len - total_sent) < free_size ? (len - total_sent) : free_size;

        ret = w5500_read_16bit_reg(dev, W5500_SOCKET_REG_BLOCK(sock_id), W5500_Sn_TX_WR, &wr_ptr);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to read reg %d", ret);
            return ret;
        }

        ret = w5500_reg_write(dev, W5500_SOCKET_TX_BUF_BLOCK(sock_id), wr_ptr, data + total_sent, chunk_size);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to write reg %d", ret);
            return ret;
        }

        wr_ptr += chunk_size;

        wr_ptr_buf[0] = W5500_BYTE_HIGH(wr_ptr);
        wr_ptr_buf[1] = W5500_BYTE_LOW(wr_ptr);

        ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_TX_WR, wr_ptr_buf, 2);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to write reg %d", ret);
            return ret;
        }

        ret = w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_SEND);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to write command %d", ret);
            return ret;
        }

        total_sent += chunk_size;
    }

    return RDNX_ERR_OK;
}

/******************************************************************************
 * @brief Receive data from a socket
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 * @param buf     - Buffer to store received data
 * @param len     - Maximum length to receive
 *
 * @return Number of bytes received on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_recv(eth_w5500_ctx_t *dev, uint8_t sock_id, void *buf, uint16_t len, uint16_t *received)
{
    rdnx_err_t ret;
    uint16_t rx_size = 0;
    uint16_t rd_ptr;
    uint8_t rd_ptr_buf[2];
    uint8_t status;

    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (status != W5500_Sn_SR_ESTABLISHED && status != W5500_Sn_SR_UDP && status != W5500_Sn_SR_MACRAW)
    {
        W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
        return RDNX_ERR_IO;
    }

    while (1)
    {
        ret = w5500_read_16bit_reg(dev, W5500_SOCKET_REG_BLOCK(sock_id), W5500_Sn_RX_RSR, &rx_size);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to read reg %d", ret);
            return ret;
        }

        if (rx_size > 0)
        {
            break;
        }

        ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to read reg %d", ret);
            return ret;
        }

        if (status != W5500_Sn_SR_ESTABLISHED && status != W5500_Sn_SR_UDP && status != W5500_Sn_SR_MACRAW)
        {
            W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
            return RDNX_ERR_IO;
        }

        rdnx_mdelay(1);
    }

    if (len > rx_size)
    {
        len = rx_size;
    }

    ret = w5500_read_16bit_reg(dev, W5500_SOCKET_REG_BLOCK(sock_id), W5500_Sn_RX_RD, &rd_ptr);
    if (ret != RDNX_ERR_OK)
    {
        return ret;
    }

    ret = w5500_reg_read(dev, W5500_SOCKET_RX_BUF_BLOCK(sock_id), rd_ptr, buf, len);
    if (ret != RDNX_ERR_OK)
    {
        return ret;
    }

    rd_ptr += len;

    rd_ptr_buf[0] = W5500_BYTE_HIGH(rd_ptr);
    rd_ptr_buf[1] = W5500_BYTE_LOW(rd_ptr);

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_RX_RD, rd_ptr_buf, 2);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    ret = w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_RECV);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write command %d", ret);
        return ret;
    }

    ret = w5500_socket_clear_interrupt(dev, sock_id, W5500_Sn_IR_RECV);
    if (ret != RDNX_ERR_OK)
    {
        return ret;
    }

    if (received != NULL)
    {
        *received = len;
    }

    return RDNX_ERR_OK;
}

/******************************************************************************
 * @brief Send data to a specific destination (UDP)
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 * @param buf     - Data buffer to send
 * @param len     - Data length
 * @param to      - Destination address (IP and port)
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_sendto(eth_w5500_ctx_t *dev, uint8_t sock_id, const void *buf, uint16_t len,
                               struct w5500_socket_address *to)
{
    rdnx_err_t ret;
    uint8_t status;
    uint8_t mr;
    uint16_t free_size = 0;
    uint16_t wr_ptr;
    uint8_t wr_ptr_buf[2];

    if (sock_id > W5500_MAX_SOCK_NUMBER || !buf || !to)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_MR, &mr, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    switch (mr & 0x0F)
    {
    case W5500_Sn_MR_UDP:
    case W5500_Sn_MR_MACRAW:
        break;
    default:
        W5500_ERR("Fail mode reg 0x%02X", mr);
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (status != W5500_Sn_SR_UDP && status != W5500_Sn_SR_MACRAW)
    {
        W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
        return RDNX_ERR_IO;
    }

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_DIPR, to->ip, 4);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_DPORT, to->port, 2);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    while (1)
    {
        ret = w5500_read_16bit_reg(dev, W5500_SOCKET_REG_BLOCK(sock_id), W5500_Sn_TX_FSR, &free_size);
        if (ret != RDNX_ERR_OK)
        {
            printf("%d: w5500_socket_sendto %d\n\r", __LINE__, ret);
            return ret;
        }

        if (free_size >= len)
        {
            break;
        }

        ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
        if (ret != RDNX_ERR_OK)
        {
            printf("%d: w5500_socket_sendto %d\n\r", __LINE__, ret);
            return ret;
        }

        if (status != W5500_Sn_SR_UDP)
        {
            W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
            return RDNX_ERR_BADARG;
        }

        rdnx_mdelay(1);
    }

    ret = w5500_read_16bit_reg(dev, W5500_SOCKET_REG_BLOCK(sock_id), W5500_Sn_TX_WR, &wr_ptr);
    if (ret != RDNX_ERR_OK)
    {
        printf("%d: w5500_socket_sendto %d\n\r", __LINE__, ret);
        return ret;
    }

    ret = w5500_reg_write(dev, W5500_SOCKET_TX_BUF_BLOCK(sock_id), wr_ptr, buf, len);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    wr_ptr += len;

    wr_ptr_buf[0] = W5500_BYTE_HIGH(wr_ptr);
    wr_ptr_buf[1] = W5500_BYTE_LOW(wr_ptr);

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_TX_WR, wr_ptr_buf, 2);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    ret = w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_SEND);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write command %d", ret);
        return ret;
    }

    return RDNX_ERR_OK;
}

/******************************************************************************
 * @brief Receive data from a socket with source address information (UDP)
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 * @param buf     - Buffer to store received data
 * @param len     - Maximum length to receive
 * @param from    - Structure to store source address information
 *
 * @return Number of bytes received on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_recvfrom(eth_w5500_ctx_t *dev, uint8_t sock_id, void *buf, uint16_t len,
                                 struct w5500_socket_address *from, uint16_t *received)
{
    rdnx_err_t ret;
    uint16_t rx_size = 0;
    uint16_t rd_ptr;
    uint8_t rd_ptr_buf[2];
    uint8_t status;
    uint8_t header[8] = {0};
    uint16_t data_len;

    if (sock_id > W5500_MAX_SOCK_NUMBER || !buf)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (status != W5500_Sn_SR_UDP)
    {
        W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
        return RDNX_ERR_BADARG;
    }

    ret = w5500_read_16bit_reg(dev, W5500_SOCKET_REG_BLOCK(sock_id), W5500_Sn_RX_RSR, &rx_size);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (rx_size < 8)
    {
        if (received != NULL)
        {
            *received = 0;
        }
        return RDNX_ERR_OK;
    }

    ret = w5500_read_16bit_reg(dev, W5500_SOCKET_REG_BLOCK(sock_id), W5500_Sn_RX_RD, &rd_ptr);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    ret = w5500_reg_read(dev, W5500_SOCKET_RX_BUF_BLOCK(sock_id), rd_ptr, header, 8);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    data_len = (header[6] << 8) | header[7];

    if (len > data_len)
    {
        len = data_len;
    }

    ret = w5500_reg_read(dev, W5500_SOCKET_RX_BUF_BLOCK(sock_id), rd_ptr + 8, buf, len);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (from)
    {
        from->ip[0] = header[0];
        from->ip[1] = header[1];
        from->ip[2] = header[2];
        from->ip[3] = header[3];
        from->port[0] = header[4];
        from->port[1] = header[5];
    }

    rd_ptr += data_len + 8;

    rd_ptr_buf[0] = W5500_BYTE_HIGH(rd_ptr);
    rd_ptr_buf[1] = W5500_BYTE_LOW(rd_ptr);

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_RX_RD, rd_ptr_buf, 2);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    ret = w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_RECV);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write command %d", ret);
        return ret;
    }

    if (received != NULL)
    {
        *received = len;
    }
    return RDNX_ERR_OK;
}

/******************************************************************************
 * @brief Bind a socket to a specific port (server mode)
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 * @param port    - Local port number
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_bind(eth_w5500_ctx_t *dev, uint8_t sock_id, uint16_t port)
{
    rdnx_err_t ret;
    uint8_t port_buf[2];

    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    port_buf[0] = W5500_BYTE_HIGH(port);
    port_buf[1] = W5500_BYTE_LOW(port);

    ret = w5500_socket_reg_write(dev, sock_id, W5500_Sn_PORT, port_buf, 2);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write reg %d", ret);
        return ret;
    }

    return RDNX_ERR_OK;
}

/******************************************************************************
 * @brief Set socket to listen mode (TCP server)
 *
 * @param dev     - Device descriptor
 * @param sock_id - Socket number (0-7)
 *
 * @return 0 on success, negative error code otherwise
 *******************************************************************************/
rdnx_err_t w5500_socket_listen(eth_w5500_ctx_t *dev, uint8_t sock_id)
{
    rdnx_err_t ret;
    uint8_t status;

    if (sock_id > W5500_MAX_SOCK_NUMBER)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_reg_read(dev, sock_id, W5500_Sn_SR, &status, 1);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read reg %d", ret);
        return ret;
    }

    if (status != W5500_Sn_SR_INIT)
    {
        W5500_ERR("Bad status 0x%02X, socket %d", (int)status, sock_id);
        return RDNX_ERR_BADARG;
    }

    ret = w5500_socket_command_write(dev, sock_id, W5500_Sn_CR_LISTEN);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to write command %d", ret);
    }

    return RDNX_ERR_OK;
}

/*****************************************************************************
 * @brief Convert network interface protocol to W5500 protocol
 *
 * @param proto - Network interface protocol
 *
 * @return W5500 protocol value
*******************************************************************************/
uint8_t w5500_socket_translate_protocol(uint32_t proto)
{
    switch (proto)
    {
    case RDNX_SOCKET_IPPROTO_TCP:
    {
        return W5500_Sn_MR_TCP;
    }
    case RDNX_SOCKET_IPPROTO_UDP:
    {
        return W5500_Sn_MR_UDP;
    }
    default:
    {
        return W5500_Sn_MR_CLOSE;
    }
    }
}

bool w5500_socket_is_open(uint8_t protocol, uint8_t status)
{
    switch (protocol)
    {
        case W5500_Sn_MR_TCP:
        {
            return status == W5500_Sn_SR_INIT;
        }
        case W5500_Sn_MR_UDP:
        {
            return status == W5500_Sn_SR_UDP;
        }
        case W5500_Sn_MR_MACRAW:
        {
            return status == W5500_Sn_SR_MACRAW;
        }
        default:
            return false;
    }
}
