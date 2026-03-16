/**
 * @file yaa_socket.c
 * @author Software development team
 * @brief Sockets Interface implementation.
 * @version 0.1
 * @date 2025-01-29
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdio.h>

/* Core includes. */
#include <yaa_types.h>
#include <yaa_macro.h>
#include <network/yaa_socket.h>
#include "yaa_socket_backend.h"

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macros for the W5500 driver. */
#if defined(DEBUG)
    #define SOCK_DEB(fmt, ...) printf("[SOCK]:" fmt "\n\r", ##__VA_ARGS__)
    #define SOCK_ERR(fmt, ...) printf("[SOCK](ERROR)(%s:%d):" fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#else
    #define SOCK_DEB(fmt, ...)   ((void)0)
    #define SOCK_ERR(fmt, ...)   ((void)0)
#endif

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

extern const yaa_socket_backend_t w5500_socket_backend;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/*
 * Creates a TCP/UDP socket.
 */
yaa_socket_t yaa_sockets_socket(void *net,
                                  int32_t domain,
                                  int32_t type,
                                  int32_t protocol)
{
    if ((domain != YAA_SOCKET_AF_INET) ||
        (type != YAA_SOCKET_SOCK_STREAM &&
         type != YAA_SOCKET_SOCK_DGRAM) ||
        ((protocol != YAA_SOCKET_IPPROTO_TCP) &&
         (protocol != YAA_SOCKET_IPPROTO_IP) &&
         (protocol != YAA_SOCKET_IPPROTO_UDP)))
    {
        SOCK_ERR("Bad socket type");
        return YAA_SOCKET_INVALID_SOCKET;
    }

    return w5500_socket_backend.socket(net, domain, type, protocol);
}

yaa_err_t yaa_sockets_bind(yaa_socket_t socket,
                             const yaa_sockaddr_t *addr,
                             yaa_socklen_t addrlen)
{
    if (socket == YAA_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    return w5500_socket_backend.bind(socket, addr, addrlen);
}

yaa_err_t yaa_sockets_listen(yaa_socket_t socket, uint16_t backlog)
{
    if (socket == YAA_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    return w5500_socket_backend.listen(socket, backlog);
}

yaa_socket_t yaa_sockets_accept(yaa_socket_t socket,
                                  yaa_sockaddr_t *addr,
                                  yaa_socklen_t addrlen)
{
    if (socket == YAA_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_SOCKET_INVALID_SOCKET;
    }

    return w5500_socket_backend.accept(socket, addr, addrlen);
}

/*
 * Connects the socket to the specified IP address and port.
 */
yaa_err_t yaa_sockets_connect(yaa_socket_t socket,
                                const yaa_sockaddr_t *addr,
                                yaa_socklen_t addrlen)
{
    if (socket == YAA_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    if ((addr == NULL) || (addrlen == 0))
    {
        SOCK_ERR("Bad arguments");
        return YAA_ERR_BADARG;
    }

    /* support only SOCKETS_AF_INET for now */
    YAA_ASSERT(addr->socket_domain == YAA_SOCKET_AF_INET);

    return w5500_socket_backend.connect(socket, addr, addrlen);
}

/*
 * Receive data from a TCP socket.
 */
int32_t yaa_sockets_recv(yaa_socket_t socket,
                          void *buf,
                          size_t len,
                          uint32_t flags)
{
    if (socket == YAA_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_SOCKET_ERR_NORESOURCE;
    }

    if ((buf == NULL) || (len == 0))
    {
        SOCK_ERR("Bad arguments");
        return YAA_SOCKET_ERR_BADARG;
    }

    return w5500_socket_backend.recv(socket, buf, len, flags);
}

/*
 * Transmit data to the remote socket.
 */
int32_t yaa_sockets_send(yaa_socket_t socket,
                          const void *buf,
                          size_t len,
                          uint32_t flags)
{
    if (socket == YAA_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_SOCKET_ERR_NORESOURCE;
    }

    if ((buf == NULL) || (len == 0))
    {
        SOCK_ERR("Bad arguments");
        return YAA_SOCKET_ERR_BADARG;
    }

    return w5500_socket_backend.send(socket, buf, len, flags);
}

int32_t yaa_sockets_recvfrom(yaa_socket_t socket,
                             void *buf,
                             uint32_t len,
                             yaa_sockaddr_t *from)
{
    if (socket == YAA_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_SOCKET_ERR_NORESOURCE;
    }

    if ((buf == NULL) || (len == 0))
    {
        SOCK_ERR("Bad arguments");
        return YAA_SOCKET_ERR_BADARG;
    }

    return w5500_socket_backend.recvfrom(socket, buf, len, from);
}

int32_t yaa_sockets_sendto(yaa_socket_t socket,
                           const void *buf,
                           uint32_t len,
                           const yaa_sockaddr_t *to)
{
    if (socket == YAA_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_SOCKET_ERR_NORESOURCE;
    }

    if ((buf == NULL) || (len == 0))
    {
        SOCK_ERR("Bad arguments");
        return YAA_SOCKET_ERR_BADARG;
    }

    return w5500_socket_backend.sendto(socket, buf, len, to);
}

yaa_err_t yaa_sockets_shutdown(yaa_socket_t socket,
                                 uint32_t how)
{
    if ( YAA_SOCKET_INVALID_SOCKET == socket)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    return w5500_socket_backend.shutdown(socket, how);
}

yaa_err_t yaa_sockets_close(yaa_socket_t socket)
{
    if ( YAA_SOCKET_INVALID_SOCKET == socket)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    return w5500_socket_backend.close(socket);
}

yaa_err_t yaa_sockets_setsockopt(yaa_socket_t socket,
                                   int32_t level,
                                   int32_t optname,
                                   const void *optval,
                                   yaa_socklen_t optlen)
{
    if ( YAA_SOCKET_INVALID_SOCKET == socket)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    return w5500_socket_backend.setsockopt(socket, level, optname, optval, optlen);
}

yaa_err_t yaa_sockets_getsockopt(yaa_socket_t socket,
                                   int32_t level,
                                   int32_t optname,
                                   void *optval,
                                   yaa_socklen_t *optlen)
{
    if ( YAA_SOCKET_INVALID_SOCKET == socket)
    {
        SOCK_ERR("Socket not initialized");
        return YAA_ERR_NORESOURCE;
    }

    return w5500_socket_backend.getsockopt(socket, level, optname, optval, optlen);
}
