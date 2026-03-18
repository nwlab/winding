/**
 * @file    w5500_socket_backend.c
 * @author  Software development team
 * @brief   W5500 socket backend implementation for rdnx socket abstraction.
 *
 * This file provides an implementation of the rdnx_socket_backend_t
 * interface using the W5500 Ethernet controller.
 *
 * It maps generic socket operations (create, bind, listen, connect,
 * send, recv, close, etc.) to low-level W5500 hardware socket
 * operations.
 *
 * Each logical rdnx socket corresponds to one hardware socket
 * inside the W5500 device. The backend is responsible for:
 *  - Allocating and releasing hardware sockets
 *  - Translating generic protocol identifiers
 *  - Converting network address structures
 *  - Managing socket lifecycle
 *
 * @note The W5500 supports a limited number of hardware sockets
 *       (typically up to 8). Resource allocation may fail if
 *       all hardware sockets are in use.
 *
 * @warning This backend is intended for embedded systems and
 *          assumes exclusive access to the W5500 device instance.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <string.h>
#include <stdio.h>

/* Core includes. */
#include "rdnx_macro.h"
#include "rdnx_types.h"
#include "rdnx_sal.h"
#include <network/rdnx_socket.h>
#include "../rdnx_socket_backend.h"
#include "w5500_internal.h"

/* ============================================================================
 * Private Type Declaration
 * ==========================================================================*/

/**
 * @brief W5500 socket backend context.
 *
 * This structure represents a socket instance bound to a specific
 * W5500 hardware socket. It stores the device reference, socket ID,
 * and a pointer to the internal socket descriptor.
 */
struct w5500_socket_backend_ctx
{
    eth_w5500_ctx_t *dev;
    uint32_t sock_id;
    struct w5500_socket *socket;
    uint16_t local_port;
    uint16_t remote_port;
    uint8_t remote_ip[4];
};

/* ============================================================================
 * Private Function Definitions
 * ==========================================================================*/

static rdnx_socket_t w5500_socket_backend_create(void *net,
                                                 int32_t domain,
                                                 int32_t type,
                                                 int32_t protocol)
{
    RDNX_UNUSED(domain);
    RDNX_UNUSED(type);
    rdnx_err_t ret;
    eth_w5500_handle_t w5500_dev = RDNX_CAST(eth_w5500_handle_t, net);
	uint8_t sock_id;
    uint8_t w5500_protocol;

    if (w5500_dev == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_SOCKET_INVALID_SOCKET;
    }

	ret = w5500_net_find_free_socket(w5500_dev, &sock_id);
	if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("No free socket");
		return RDNX_SOCKET_INVALID_SOCKET;
    }

    struct w5500_socket_backend_ctx *ctx = rdnx_alloc(sizeof(struct w5500_socket_backend_ctx));
    if (ctx == NULL)
    {
        W5500_ERR("No free memory");
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    ctx->dev = w5500_dev;
    ctx->sock_id = sock_id;
	ctx->local_port = 0;
	ctx->remote_port = 0;
	memset(ctx->remote_ip, 0, 4);

    w5500_protocol = w5500_socket_translate_protocol(protocol);

    ret = w5500_socket_open(ctx->dev, ctx->sock_id, w5500_protocol, ctx->dev->buff_kb);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to open socket %d", (int)ret);
        rdnx_free(ctx);
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    ctx->socket = &ctx->dev->sockets[ctx->sock_id];
    ctx->socket->in_use = 1;

    return RDNX_CAST(rdnx_socket_t, ctx);
}

static rdnx_err_t w5500_socket_backend_bind(rdnx_socket_t socket, const rdnx_sockaddr_t *addr, rdnx_socklen_t addrlen)
{
    RDNX_UNUSED(addrlen);
	rdnx_err_t ret;
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

	ret = w5500_socket_bind(ctx->dev, ctx->sock_id, addr->port);
	if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to bind socket %d", (int)ret);
		return ret;
    }

    ctx->local_port = addr->port;

    W5500_DEB("Bind socket %d to %d", (int)ctx->sock_id, addr->port);

	return RDNX_ERR_OK;
}

static rdnx_err_t w5500_socket_backend_listen(rdnx_socket_t socket, uint16_t backlog)
{
    RDNX_UNUSED(backlog);
	rdnx_err_t ret;
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    ret = w5500_socket_listen(ctx->dev, ctx->sock_id);
	if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to listen socket %d", (int)ret);
		return ret;
    }

    return RDNX_ERR_OK;
}

static rdnx_socket_t w5500_socket_backend_accept(rdnx_socket_t socket, rdnx_sockaddr_t *addr, rdnx_socklen_t addrlen)
{
    RDNX_UNUSED(addr);
    RDNX_UNUSED(addrlen);
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    uint8_t status;
	uint8_t sock_id = ctx->sock_id;
    uint8_t new_sock_id;
    rdnx_err_t ret;

    ret = w5500_socket_read_status(ctx->dev, ctx->sock_id, &status);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to read status %d", (int)ret);
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    if (status != W5500_Sn_SR_ESTABLISHED)
    {
        W5500_ERR("Status %d", (int)status);
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    /* Find and configure a new socket to take over server role */
    ret = w5500_net_find_free_socket(ctx->dev, &new_sock_id);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("No free socket");
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    rdnx_socket_t new_socket = w5500_socket_backend_create(ctx->dev,
                                                           RDNX_SOCKET_AF_INET,
                                                           RDNX_SOCKET_SOCK_STREAM,
                                                           RDNX_SOCKET_IPPROTO_TCP);

    if (new_socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        W5500_ERR("Fail to create socket");
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    struct w5500_socket_backend_ctx *new_ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, new_socket);

    new_sock_id = new_ctx->sock_id;

    ret = w5500_socket_bind(new_ctx->dev, new_ctx->sock_id, ctx->local_port);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to bind socket %d", (int)ret);
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    ret = w5500_socket_listen(new_ctx->dev, new_ctx->sock_id);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to listen socket %d", (int)ret);
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    /* Configure new socket to take the place of the old server */
    new_ctx->sock_id = sock_id;
    new_ctx->socket = &new_ctx->dev->sockets[new_ctx->sock_id];
    new_ctx->local_port = ctx->local_port;

    /* Old server socket becomes new client socket */
    ctx->sock_id = new_sock_id;
    ctx->socket = &ctx->dev->sockets[ctx->sock_id];

    return RDNX_CAST(rdnx_socket_t, ctx);
}

static rdnx_err_t w5500_socket_backend_connect(rdnx_socket_t socket,
                                               const rdnx_sockaddr_t *addr,
                                               rdnx_socklen_t addrlen)
{
    RDNX_UNUSED(addrlen);
    rdnx_err_t ret;
    struct w5500_socket_address w5500_addr;
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    if (addr == NULL)
    {
        W5500_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    ret = w5500_net_addr_net_to_w5500(addr, &w5500_addr);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to convert net addr %d", (int)ret);
        return ret;
    }

    ret = w5500_socket_connect(ctx->dev, ctx->sock_id, &w5500_addr);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to connect %d", (int)ret);
        return ret;
    }

    return RDNX_ERR_OK;
}

static int32_t w5500_socket_backend_recv(rdnx_socket_t socket,
                                         void *buf,
                                         size_t len,
                                         uint32_t flags)
{
    RDNX_UNUSED(flags);
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);
    rdnx_err_t ret;
    uint16_t received = 0;

    if (ctx == NULL)
    {   W5500_ERR("Socket not initialized");
        return RDNX_SOCKET_ERR_NORESOURCE;
    }

	ret = w5500_socket_recv(ctx->dev, ctx->sock_id, buf, len, &received);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to recv %d", (int)ret);
        return RDNX_SOCKET_ERR_IO;
    }

    return received;
}

static int32_t w5500_socket_backend_send(rdnx_socket_t socket,
                                         const void *buf,
                                         size_t len,
                                         uint32_t flags)
{
    RDNX_UNUSED(flags);
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);
    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_SOCKET_ERR_NORESOURCE;
    }

    return w5500_socket_send(ctx->dev, ctx->sock_id, buf, len);
}

static int32_t w5500_socket_backend_sendto(rdnx_socket_t socket, const void *data, uint32_t size,
                                           const rdnx_sockaddr_t *to)
{
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);
    rdnx_err_t ret;
    struct w5500_socket_address w5500_addr;

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_SOCKET_ERR_NORESOURCE;
    }

    if (data == NULL || to == NULL)
    {
        W5500_ERR("Bad arguments");
        return RDNX_SOCKET_ERR_BADARG;
    }

    ret = w5500_net_addr_net_to_w5500(to, &w5500_addr);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to convert net addr %d", (int)ret);
        return RDNX_SOCKET_ERR_FAIL;
    }

    ret = w5500_socket_sendto(ctx->dev, ctx->sock_id, data, size, &w5500_addr);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("w5500_socket_sendto %d", ret);
        return RDNX_SOCKET_ERR_IO;
    }

    return size;
}

static int32_t w5500_socket_backend_recvfrom(rdnx_socket_t socket, void *data, uint32_t size,
                                             rdnx_sockaddr_t *from)
{
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);
    rdnx_err_t ret;
    uint16_t received = 0;
    struct w5500_socket_address w5500_from = {{0,0,0,0},{0,0}};

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_SOCKET_ERR_NORESOURCE;
    }

    if (data == NULL)
    {
        W5500_ERR("Bad arguments");
        return RDNX_SOCKET_ERR_BADARG;
    }

    ret = w5500_socket_recvfrom(ctx->dev, ctx->sock_id, data, size, &w5500_from, &received);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to recv %d", (int)ret);
        return RDNX_SOCKET_ERR_IO;
    }

    if (from)
    {
        ret = w5500_net_addr_w5500_to_net(&w5500_from, from);
        if (ret != RDNX_ERR_OK)
        {
            W5500_ERR("Fail to convert net addr %d", (int)ret);
            return RDNX_SOCKET_ERR_FAIL;
        }
    }

    return received;
}

static rdnx_err_t w5500_socket_backend_shutdown(rdnx_socket_t socket,
                                                uint32_t how)
{
    RDNX_UNUSED(how);
    rdnx_err_t ret;
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    ret = w5500_socket_disconnect(ctx->dev, ctx->sock_id);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to disconnect %d", (int)ret);
        return ret;
    }

    return RDNX_ERR_OK;
}

static rdnx_err_t w5500_socket_backend_close(rdnx_socket_t socket)
{
    rdnx_err_t ret;
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    ret = w5500_socket_close(ctx->dev, ctx->sock_id);
    if (ret != RDNX_ERR_OK)
    {
        W5500_ERR("Fail to close %d", (int)ret);
        return ret;
    }

    ctx->socket->in_use = 0;

    rdnx_free(ctx);

    return RDNX_ERR_OK;
}

static rdnx_err_t w5500_socket_backend_setsockopt(rdnx_socket_t socket,
                                                  int32_t level,
                                                  int32_t optname,
                                                  const void *optval,
                                                  rdnx_socklen_t optlen)
{
    RDNX_UNUSED(level);
    RDNX_UNUSED(optname);
    RDNX_UNUSED(optval);
    RDNX_UNUSED(optlen);

    rdnx_err_t ret = RDNX_ERR_OK;
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    switch (optname)
    {
        case RDNX_SOCKET_SO_RCVTIMEO:
        case RDNX_SOCKET_SO_SNDTIMEO:
            break;
        case RDNX_SOCKET_SO_NONBLOCK:
            break;
        case RDNX_SOCKET_SO_REQUIRE_TLS:
            break;
        case RDNX_SOCKET_SO_TRUSTED_SERVER_CERTIFICATE:
            break;
        case RDNX_SOCKET_SO_CLIENT_CERTIFICATE:
            break;
        case RDNX_SOCKET_SO_CLIENT_KEY:
            break;
        case RDNX_SOCKET_SO_SERVER_NAME_INDICATION:
            break;
        case RDNX_SOCKET_SO_ALPN_PROTOCOLS:
            break;
        case RDNX_SOCKET_SO_TCPKEEPALIVE:
            break;
        case RDNX_SOCKET_SO_TCPKEEPALIVE_INTERVAL:
            break;
        case RDNX_SOCKET_SO_TCPKEEPALIVE_COUNT:
            break;
        case RDNX_SOCKET_SO_TCPKEEPALIVE_IDLE_TIME:
            break;
        default:
            return RDNX_ERR_BADARG;
    }

    if (ret != RDNX_ERR_OK)
    {
        return RDNX_ERR_FAIL;
    }

    return RDNX_ERR_NORESOURCE;
}

static rdnx_err_t copy_opt(void *dst,
                           rdnx_socklen_t *dst_len,
                           const void *src,
                           rdnx_socklen_t src_len)
{
    if (!dst || !dst_len)
        return RDNX_ERR_BADARG;

    if (*dst_len < src_len)
        return RDNX_ERR_NOMEM;

    memcpy(dst, src, src_len);
    *dst_len = src_len;

    return RDNX_ERR_OK;
}

static rdnx_err_t w5500_socket_backend_getsockopt(rdnx_socket_t socket,
                                                  int32_t level,
                                                  int32_t optname,
                                                  void *optval,
                                                  rdnx_socklen_t *optlen)
{
    RDNX_UNUSED(level);
    RDNX_UNUSED(optname);
    RDNX_UNUSED(optval);
    RDNX_UNUSED(optlen);

    rdnx_err_t ret = RDNX_ERR_OK;
    struct w5500_socket_backend_ctx *ctx = RDNX_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    switch (optname)
    {
        case RDNX_SOCKET_SO_RCVTIMEO:
        case RDNX_SOCKET_SO_SNDTIMEO:
            break;
        case RDNX_SOCKET_SO_NONBLOCK:
            break;
        case RDNX_SOCKET_SO_REQUIRE_TLS:
            break;
        case RDNX_SOCKET_SO_TRUSTED_SERVER_CERTIFICATE:
            break;
        case RDNX_SOCKET_SO_CLIENT_CERTIFICATE:
            break;
        case RDNX_SOCKET_SO_CLIENT_KEY:
            break;
        case RDNX_SOCKET_SO_SERVER_NAME_INDICATION:
            break;
        case RDNX_SOCKET_SO_ALPN_PROTOCOLS:
            break;
        case RDNX_SOCKET_SO_TCPKEEPALIVE:
            break;
        case RDNX_SOCKET_SO_TCPKEEPALIVE_INTERVAL:
            break;
        case RDNX_SOCKET_SO_TCPKEEPALIVE_COUNT:
            break;
        case RDNX_SOCKET_SO_TCPKEEPALIVE_IDLE_TIME:
            break;
        case RDNX_SOCKET_SO_CONNECTED:
        {
            uint8_t state = 0;
            ret = w5500_socket_read_status(ctx->dev, ctx->sock_id, &state);
            if (ret == RDNX_ERR_OK)
            {
                uint32_t connected = (state == W5500_Sn_SR_ESTABLISHED);
                return copy_opt(optval,
                                optlen,
                                &connected,
                                sizeof(connected));
            }
            break;
        }

        case RDNX_SOCKET_SO_STATE:
        {
            uint8_t state = 0;
            ret = w5500_socket_read_status(ctx->dev, ctx->sock_id, &state);
            if (ret == RDNX_ERR_OK)
            {
                return copy_opt(optval,
                                optlen,
                                &state,
                                sizeof(state));
            }
            break;
        }
        case RDNX_SOCKET_SO_TX_BUFFER_SIZE:
        {
            uint16_t free_size = 0;
            ret = w5500_read_16bit_reg(ctx->dev, W5500_SOCKET_REG_BLOCK(ctx->sock_id), W5500_Sn_TX_FSR, &free_size);
            if (ret == RDNX_ERR_OK)
            {
                return copy_opt(optval,
                                optlen,
                                &free_size,
                                sizeof(free_size));
            }
            break;
        }
        case RDNX_SOCKET_SO_RX_BUFFER_SIZE:
        {
            uint16_t free_size = 0;
            ret = w5500_read_16bit_reg(ctx->dev, W5500_SOCKET_REG_BLOCK(ctx->sock_id), W5500_Sn_RX_RSR, &free_size);
            if (ret == RDNX_ERR_OK)
            {
                return copy_opt(optval,
                                optlen,
                                &free_size,
                                sizeof(free_size));
            }
            break;
        }

        default:
            return RDNX_ERR_BADARG;
    }

    if (ret != RDNX_ERR_OK)
    {
        return ret;
    }

    return RDNX_ERR_NORESOURCE;
}

/**
 * @brief W5500 socket backend implementation.
 *
 * Provides an implementation of rdnx_socket_backend_t
 * using the W5500 Ethernet controller.
 */
const rdnx_socket_backend_t w5500_socket_backend =
{
    .socket   = w5500_socket_backend_create,
    .bind     = w5500_socket_backend_bind,
    .listen   = w5500_socket_backend_listen,
    .accept   = w5500_socket_backend_accept,
    .connect  = w5500_socket_backend_connect,
    .recv     = w5500_socket_backend_recv,
    .send     = w5500_socket_backend_send,
    .recvfrom = w5500_socket_backend_recvfrom,
    .sendto   = w5500_socket_backend_sendto,
    .shutdown = w5500_socket_backend_shutdown,
    .close    = w5500_socket_backend_close,
    .setsockopt = w5500_socket_backend_setsockopt,
    .getsockopt = w5500_socket_backend_getsockopt
};
