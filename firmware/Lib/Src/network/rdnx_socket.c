/**
 * @file rdnx_socket.c
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
#include <rdnx_types.h>
#include <rdnx_macro.h>
#include <network/rdnx_socket.h>
#include "rdnx_socket_backend.h"

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

extern const rdnx_socket_backend_t w5500_socket_backend;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/*
 * Creates a TCP/UDP socket.
 */
rdnx_socket_t rdnx_sockets_socket(void *net,
                                  int32_t domain,
                                  int32_t type,
                                  int32_t protocol)
{
    if ((domain != RDNX_SOCKET_AF_INET) ||
        (type != RDNX_SOCKET_SOCK_STREAM &&
         type != RDNX_SOCKET_SOCK_DGRAM) ||
        ((protocol != RDNX_SOCKET_IPPROTO_TCP) &&
         (protocol != RDNX_SOCKET_IPPROTO_IP) &&
         (protocol != RDNX_SOCKET_IPPROTO_UDP)))
    {
        SOCK_ERR("Bad socket type");
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    return w5500_socket_backend.socket(net, domain, type, protocol);
}

rdnx_err_t rdnx_sockets_bind(rdnx_socket_t socket,
                             const rdnx_sockaddr_t *addr,
                             rdnx_socklen_t addrlen)
{
    if (socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    return w5500_socket_backend.bind(socket, addr, addrlen);
}

rdnx_err_t rdnx_sockets_listen(rdnx_socket_t socket, uint16_t backlog)
{
    if (socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    return w5500_socket_backend.listen(socket, backlog);
}

rdnx_socket_t rdnx_sockets_accept(rdnx_socket_t socket,
                                  rdnx_sockaddr_t *addr,
                                  rdnx_socklen_t addrlen)
{
    if (socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_SOCKET_INVALID_SOCKET;
    }

    return w5500_socket_backend.accept(socket, addr, addrlen);
}

/*
 * Connects the socket to the specified IP address and port.
 */
rdnx_err_t rdnx_sockets_connect(rdnx_socket_t socket,
                                const rdnx_sockaddr_t *addr,
                                rdnx_socklen_t addrlen)
{
    if (socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    if ((addr == NULL) || (addrlen == 0))
    {
        SOCK_ERR("Bad arguments");
        return RDNX_ERR_BADARG;
    }

    /* support only SOCKETS_AF_INET for now */
    RDNX_ASSERT(addr->socket_domain == RDNX_SOCKET_AF_INET);

    return w5500_socket_backend.connect(socket, addr, addrlen);
}

/*
 * Receive data from a TCP socket.
 */
int32_t rdnx_sockets_recv(rdnx_socket_t socket,
                          void *buf,
                          size_t len,
                          uint32_t flags)
{
    if (socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_SOCKET_ERR_NORESOURCE;
    }

    if ((buf == NULL) || (len == 0))
    {
        SOCK_ERR("Bad arguments");
        return RDNX_SOCKET_ERR_BADARG;
    }

    return w5500_socket_backend.recv(socket, buf, len, flags);
}

/*
 * Transmit data to the remote socket.
 */
int32_t rdnx_sockets_send(rdnx_socket_t socket,
                          const void *buf,
                          size_t len,
                          uint32_t flags)
{
    if (socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_SOCKET_ERR_NORESOURCE;
    }

    if ((buf == NULL) || (len == 0))
    {
        SOCK_ERR("Bad arguments");
        return RDNX_SOCKET_ERR_BADARG;
    }

    return w5500_socket_backend.send(socket, buf, len, flags);
}

int32_t rdnx_sockets_recvfrom(rdnx_socket_t socket,
                             void *buf,
                             uint32_t len,
                             rdnx_sockaddr_t *from)
{
    if (socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_SOCKET_ERR_NORESOURCE;
    }

    if ((buf == NULL) || (len == 0))
    {
        SOCK_ERR("Bad arguments");
        return RDNX_SOCKET_ERR_BADARG;
    }

    return w5500_socket_backend.recvfrom(socket, buf, len, from);
}

int32_t rdnx_sockets_sendto(rdnx_socket_t socket,
                           const void *buf,
                           uint32_t len,
                           const rdnx_sockaddr_t *to)
{
    if (socket == RDNX_SOCKET_INVALID_SOCKET)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_SOCKET_ERR_NORESOURCE;
    }

    if ((buf == NULL) || (len == 0))
    {
        SOCK_ERR("Bad arguments");
        return RDNX_SOCKET_ERR_BADARG;
    }

    return w5500_socket_backend.sendto(socket, buf, len, to);
}

rdnx_err_t rdnx_sockets_shutdown(rdnx_socket_t socket,
                                 uint32_t how)
{
    if ( RDNX_SOCKET_INVALID_SOCKET == socket)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    return w5500_socket_backend.shutdown(socket, how);
}

rdnx_err_t rdnx_sockets_close(rdnx_socket_t socket)
{
    if ( RDNX_SOCKET_INVALID_SOCKET == socket)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    return w5500_socket_backend.close(socket);
}

rdnx_err_t rdnx_sockets_setsockopt(rdnx_socket_t socket,
                                   int32_t level,
                                   int32_t optname,
                                   const void *optval,
                                   rdnx_socklen_t optlen)
{
    if ( RDNX_SOCKET_INVALID_SOCKET == socket)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    return w5500_socket_backend.setsockopt(socket, level, optname, optval, optlen);
}

rdnx_err_t rdnx_sockets_getsockopt(rdnx_socket_t socket,
                                   int32_t level,
                                   int32_t optname,
                                   void *optval,
                                   rdnx_socklen_t *optlen)
{
    if ( RDNX_SOCKET_INVALID_SOCKET == socket)
    {
        SOCK_ERR("Socket not initialized");
        return RDNX_ERR_NORESOURCE;
    }

    return w5500_socket_backend.getsockopt(socket, level, optname, optval, optlen);
}
