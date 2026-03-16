/**
 * @file    w5500_socket_backend.c
 * @author  Software development team
 * @brief   W5500 socket backend implementation for yaa socket abstraction.
 *
 * This file provides an implementation of the yaa_socket_backend_t
 * interface using the W5500 Ethernet controller.
 *
 * It maps generic socket operations (create, bind, listen, connect,
 * send, recv, close, etc.) to low-level W5500 hardware socket
 * operations.
 *
 * Each logical yaa socket corresponds to one hardware socket
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
#include "yaa_macro.h"
#include "yaa_types.h"
#include "yaa_sal.h"
#include <network/yaa_socket.h>
#include "../yaa_socket_backend.h"
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

static yaa_socket_t w5500_socket_backend_create(void *net,
                                                 int32_t domain,
                                                 int32_t type,
                                                 int32_t protocol)
{
    YAA_UNUSED(domain);
    YAA_UNUSED(type);
    yaa_err_t ret;
    eth_w5500_handle_t w5500_dev = YAA_CAST(eth_w5500_handle_t, net);
	uint8_t sock_id;
    uint8_t w5500_protocol;

    if (w5500_dev == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_SOCKET_INVALID_SOCKET;
    }

	ret = w5500_net_find_free_socket(w5500_dev, &sock_id);
	if (ret != YAA_ERR_OK)
    {
        W5500_ERR("No free socket");
		return YAA_SOCKET_INVALID_SOCKET;
    }

    struct w5500_socket_backend_ctx *ctx = yaa_alloc(sizeof(struct w5500_socket_backend_ctx));
    if (ctx == NULL)
    {
        W5500_ERR("No free memory");
        return YAA_SOCKET_INVALID_SOCKET;
    }

    ctx->dev = w5500_dev;
    ctx->sock_id = sock_id;
	ctx->local_port = 0;
	ctx->remote_port = 0;
	memset(ctx->remote_ip, 0, 4);

    w5500_protocol = w5500_socket_translate_protocol(protocol);

    ret = w5500_socket_open(ctx->dev, ctx->sock_id, w5500_protocol, ctx->dev->buff_kb);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to open socket %d", (int)ret);
        yaa_free(ctx);
        return YAA_SOCKET_INVALID_SOCKET;
    }

    ctx->socket = &ctx->dev->sockets[ctx->sock_id];
    ctx->socket->in_use = 1;

    return YAA_CAST(yaa_socket_t, ctx);
}

static yaa_err_t w5500_socket_backend_bind(yaa_socket_t socket, const yaa_sockaddr_t *addr, yaa_socklen_t addrlen)
{
    YAA_UNUSED(addrlen);
	yaa_err_t ret;
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

	ret = w5500_socket_bind(ctx->dev, ctx->sock_id, addr->port);
	if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to bind socket %d", (int)ret);
		return ret;
    }

    ctx->local_port = addr->port;

    W5500_DEB("Bind socket %d to %d", (int)ctx->sock_id, addr->port);

	return YAA_ERR_OK;
}

static yaa_err_t w5500_socket_backend_listen(yaa_socket_t socket, uint16_t backlog)
{
    YAA_UNUSED(backlog);
	yaa_err_t ret;
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    ret = w5500_socket_listen(ctx->dev, ctx->sock_id);
	if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to listen socket %d", (int)ret);
		return ret;
    }

    return YAA_ERR_OK;
}

static yaa_socket_t w5500_socket_backend_accept(yaa_socket_t socket, yaa_sockaddr_t *addr, yaa_socklen_t addrlen)
{
    YAA_UNUSED(addr);
    YAA_UNUSED(addrlen);
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_SOCKET_INVALID_SOCKET;
    }

    uint8_t status;
	uint8_t sock_id = ctx->sock_id;
    uint8_t new_sock_id;
    yaa_err_t ret;

    ret = w5500_socket_read_status(ctx->dev, ctx->sock_id, &status);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to read status %d", (int)ret);
        return YAA_SOCKET_INVALID_SOCKET;
    }

    if (status != W5500_Sn_SR_ESTABLISHED)
    {
        W5500_ERR("Status %d", (int)status);
        return YAA_SOCKET_INVALID_SOCKET;
    }

    /* Find and configure a new socket to take over server role */
    ret = w5500_net_find_free_socket(ctx->dev, &new_sock_id);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("No free socket");
        return YAA_SOCKET_INVALID_SOCKET;
    }

    yaa_socket_t new_socket = w5500_socket_backend_create(ctx->dev,
                                                           YAA_SOCKET_AF_INET,
                                                           YAA_SOCKET_SOCK_STREAM,
                                                           YAA_SOCKET_IPPROTO_TCP);

    if (new_socket == YAA_SOCKET_INVALID_SOCKET)
    {
        W5500_ERR("Fail to create socket");
        return YAA_SOCKET_INVALID_SOCKET;
    }

    struct w5500_socket_backend_ctx *new_ctx = YAA_CAST(struct w5500_socket_backend_ctx *, new_socket);

    new_sock_id = new_ctx->sock_id;

    ret = w5500_socket_bind(new_ctx->dev, new_ctx->sock_id, ctx->local_port);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to bind socket %d", (int)ret);
        return YAA_SOCKET_INVALID_SOCKET;
    }

    ret = w5500_socket_listen(new_ctx->dev, new_ctx->sock_id);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to listen socket %d", (int)ret);
        return YAA_SOCKET_INVALID_SOCKET;
    }

    /* Configure new socket to take the place of the old server */
    new_ctx->sock_id = sock_id;
    new_ctx->socket = &new_ctx->dev->sockets[new_ctx->sock_id];
    new_ctx->local_port = ctx->local_port;

    /* Old server socket becomes new client socket */
    ctx->sock_id = new_sock_id;
    ctx->socket = &ctx->dev->sockets[ctx->sock_id];

    return YAA_CAST(yaa_socket_t, ctx);
}

static yaa_err_t w5500_socket_backend_connect(yaa_socket_t socket,
                                               const yaa_sockaddr_t *addr,
                                               yaa_socklen_t addrlen)
{
    YAA_UNUSED(addrlen);
    yaa_err_t ret;
    struct w5500_socket_address w5500_addr;
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    if (addr == NULL)
    {
        W5500_ERR("Bad arguments");
        return YAA_ERR_BADARG;
    }

    ret = w5500_net_addr_net_to_w5500(addr, &w5500_addr);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to convert net addr %d", (int)ret);
        return ret;
    }

    ret = w5500_socket_connect(ctx->dev, ctx->sock_id, &w5500_addr);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to connect %d", (int)ret);
        return ret;
    }

    return YAA_ERR_OK;
}

static int32_t w5500_socket_backend_recv(yaa_socket_t socket,
                                         void *buf,
                                         size_t len,
                                         uint32_t flags)
{
    YAA_UNUSED(flags);
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);
    yaa_err_t ret;
    uint16_t received = 0;

    if (ctx == NULL)
    {   W5500_ERR("Socket not initialized");
        return YAA_SOCKET_ERR_NORESOURCE;
    }

	ret = w5500_socket_recv(ctx->dev, ctx->sock_id, buf, len, &received);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to recv %d", (int)ret);
        return YAA_SOCKET_ERR_IO;
    }

    return received;
}

static int32_t w5500_socket_backend_send(yaa_socket_t socket,
                                         const void *buf,
                                         size_t len,
                                         uint32_t flags)
{
    YAA_UNUSED(flags);
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);
    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_SOCKET_ERR_NORESOURCE;
    }

    return w5500_socket_send(ctx->dev, ctx->sock_id, buf, len);
}

static int32_t w5500_socket_backend_sendto(yaa_socket_t socket, const void *data, uint32_t size,
                                           const yaa_sockaddr_t *to)
{
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);
    yaa_err_t ret;
    struct w5500_socket_address w5500_addr;

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_SOCKET_ERR_NORESOURCE;
    }

    if (data == NULL || to == NULL)
    {
        W5500_ERR("Bad arguments");
        return YAA_SOCKET_ERR_BADARG;
    }

    ret = w5500_net_addr_net_to_w5500(to, &w5500_addr);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to convert net addr %d", (int)ret);
        return YAA_SOCKET_ERR_FAIL;
    }

    ret = w5500_socket_sendto(ctx->dev, ctx->sock_id, data, size, &w5500_addr);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("w5500_socket_sendto %d", ret);
        return YAA_SOCKET_ERR_IO;
    }

    return size;
}

static int32_t w5500_socket_backend_recvfrom(yaa_socket_t socket, void *data, uint32_t size,
                                             yaa_sockaddr_t *from)
{
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);
    yaa_err_t ret;
    uint16_t received = 0;
    struct w5500_socket_address w5500_from = {{0,0,0,0},{0,0}};

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_SOCKET_ERR_NORESOURCE;
    }

    if (data == NULL)
    {
        W5500_ERR("Bad arguments");
        return YAA_SOCKET_ERR_BADARG;
    }

    ret = w5500_socket_recvfrom(ctx->dev, ctx->sock_id, data, size, &w5500_from, &received);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to recv %d", (int)ret);
        return YAA_SOCKET_ERR_IO;
    }

    if (from)
    {
        ret = w5500_net_addr_w5500_to_net(&w5500_from, from);
        if (ret != YAA_ERR_OK)
        {
            W5500_ERR("Fail to convert net addr %d", (int)ret);
            return YAA_SOCKET_ERR_FAIL;
        }
    }

    return received;
}

static yaa_err_t w5500_socket_backend_shutdown(yaa_socket_t socket,
                                                uint32_t how)
{
    YAA_UNUSED(how);
    yaa_err_t ret;
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    ret = w5500_socket_disconnect(ctx->dev, ctx->sock_id);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to disconnect %d", (int)ret);
        return ret;
    }

    return YAA_ERR_OK;
}

static yaa_err_t w5500_socket_backend_close(yaa_socket_t socket)
{
    yaa_err_t ret;
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    ret = w5500_socket_close(ctx->dev, ctx->sock_id);
    if (ret != YAA_ERR_OK)
    {
        W5500_ERR("Fail to close %d", (int)ret);
        return ret;
    }

    ctx->socket->in_use = 0;

    yaa_free(ctx);

    return YAA_ERR_OK;
}

static yaa_err_t w5500_socket_backend_setsockopt(yaa_socket_t socket,
                                                  int32_t level,
                                                  int32_t optname,
                                                  const void *optval,
                                                  yaa_socklen_t optlen)
{
    YAA_UNUSED(level);
    YAA_UNUSED(optname);
    YAA_UNUSED(optval);
    YAA_UNUSED(optlen);

    yaa_err_t ret = YAA_ERR_OK;
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    switch (optname)
    {
        case YAA_SOCKET_SO_RCVTIMEO:
        case YAA_SOCKET_SO_SNDTIMEO:
            break;
        case YAA_SOCKET_SO_NONBLOCK:
            break;
        case YAA_SOCKET_SO_REQUIRE_TLS:
            break;
        case YAA_SOCKET_SO_TRUSTED_SERVER_CERTIFICATE:
            break;
        case YAA_SOCKET_SO_CLIENT_CERTIFICATE:
            break;
        case YAA_SOCKET_SO_CLIENT_KEY:
            break;
        case YAA_SOCKET_SO_SERVER_NAME_INDICATION:
            break;
        case YAA_SOCKET_SO_ALPN_PROTOCOLS:
            break;
        case YAA_SOCKET_SO_TCPKEEPALIVE:
            break;
        case YAA_SOCKET_SO_TCPKEEPALIVE_INTERVAL:
            break;
        case YAA_SOCKET_SO_TCPKEEPALIVE_COUNT:
            break;
        case YAA_SOCKET_SO_TCPKEEPALIVE_IDLE_TIME:
            break;
        default:
            return YAA_ERR_BADARG;
    }

    if (ret != YAA_ERR_OK)
    {
        return YAA_ERR_FAIL;
    }

    return YAA_ERR_NORESOURCE;
}

static yaa_err_t copy_opt(void *dst,
                           yaa_socklen_t *dst_len,
                           const void *src,
                           yaa_socklen_t src_len)
{
    if (!dst || !dst_len)
        return YAA_ERR_BADARG;

    if (*dst_len < src_len)
        return YAA_ERR_NOMEM;

    memcpy(dst, src, src_len);
    *dst_len = src_len;

    return YAA_ERR_OK;
}

static yaa_err_t w5500_socket_backend_getsockopt(yaa_socket_t socket,
                                                  int32_t level,
                                                  int32_t optname,
                                                  void *optval,
                                                  yaa_socklen_t *optlen)
{
    YAA_UNUSED(level);
    YAA_UNUSED(optname);
    YAA_UNUSED(optval);
    YAA_UNUSED(optlen);

    yaa_err_t ret = YAA_ERR_OK;
    struct w5500_socket_backend_ctx *ctx = YAA_CAST(struct w5500_socket_backend_ctx *, socket);

    if (ctx == NULL)
    {
        W5500_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    switch (optname)
    {
        case YAA_SOCKET_SO_RCVTIMEO:
        case YAA_SOCKET_SO_SNDTIMEO:
            break;
        case YAA_SOCKET_SO_NONBLOCK:
            break;
        case YAA_SOCKET_SO_REQUIRE_TLS:
            break;
        case YAA_SOCKET_SO_TRUSTED_SERVER_CERTIFICATE:
            break;
        case YAA_SOCKET_SO_CLIENT_CERTIFICATE:
            break;
        case YAA_SOCKET_SO_CLIENT_KEY:
            break;
        case YAA_SOCKET_SO_SERVER_NAME_INDICATION:
            break;
        case YAA_SOCKET_SO_ALPN_PROTOCOLS:
            break;
        case YAA_SOCKET_SO_TCPKEEPALIVE:
            break;
        case YAA_SOCKET_SO_TCPKEEPALIVE_INTERVAL:
            break;
        case YAA_SOCKET_SO_TCPKEEPALIVE_COUNT:
            break;
        case YAA_SOCKET_SO_TCPKEEPALIVE_IDLE_TIME:
            break;
        case YAA_SOCKET_SO_CONNECTED:
        {
            uint8_t state = 0;
            ret = w5500_socket_read_status(ctx->dev, ctx->sock_id, &state);
            if (ret == YAA_ERR_OK)
            {
                uint32_t connected = (state == W5500_Sn_SR_ESTABLISHED);
                return copy_opt(optval,
                                optlen,
                                &connected,
                                sizeof(connected));
            }
            break;
        }

        case YAA_SOCKET_SO_STATE:
        {
            uint8_t state = 0;
            ret = w5500_socket_read_status(ctx->dev, ctx->sock_id, &state);
            if (ret == YAA_ERR_OK)
            {
                return copy_opt(optval,
                                optlen,
                                &state,
                                sizeof(state));
            }
            break;
        }
        case YAA_SOCKET_SO_TX_BUFFER_SIZE:
        {
            uint16_t free_size = 0;
            ret = w5500_read_16bit_reg(ctx->dev, W5500_SOCKET_REG_BLOCK(ctx->sock_id), W5500_Sn_TX_FSR, &free_size);
            if (ret == YAA_ERR_OK)
            {
                return copy_opt(optval,
                                optlen,
                                &free_size,
                                sizeof(free_size));
            }
            break;
        }
        case YAA_SOCKET_SO_RX_BUFFER_SIZE:
        {
            uint16_t free_size = 0;
            ret = w5500_read_16bit_reg(ctx->dev, W5500_SOCKET_REG_BLOCK(ctx->sock_id), W5500_Sn_RX_RSR, &free_size);
            if (ret == YAA_ERR_OK)
            {
                return copy_opt(optval,
                                optlen,
                                &free_size,
                                sizeof(free_size));
            }
            break;
        }

        default:
            return YAA_ERR_BADARG;
    }

    if (ret != YAA_ERR_OK)
    {
        return ret;
    }

    return YAA_ERR_NORESOURCE;
}

/**
 * @brief W5500 socket backend implementation.
 *
 * Provides an implementation of yaa_socket_backend_t
 * using the W5500 Ethernet controller.
 */
const yaa_socket_backend_t w5500_socket_backend =
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
