/**
 * @file    rdnx_socket_backend.h
 * @author  Software development team
 * @brief   Socket backend interface for the RDNX networking stack.
 *
 * This header defines the socket backend interface used by the
 * RDNX generic socket API (@ref rdnx_socket.c). A backend provides
 * the concrete implementation of socket operations for a specific
 * network stack or hardware device (e.g., W5500, lwIP, Linux,
 * TLS wrapper, etc.).
 *
 * All public socket API calls are dispatched to the registered
 * backend, which performs the actual low-level operations.
 *
 * @note Only one backend may be active at a time unless multi-backend
 *       support is explicitly implemented.
 *
 * @warning This interface is internal to the networking stack and
 *          should not be used directly by application code.
 */

#ifndef RDNX_SOCKET_BACKEND_H
#define RDNX_SOCKET_BACKEND_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

#include <network/rdnx_socket.h>

/* ============================================================================
 * Public Type Definitions
 * ==========================================================================*/

/**
 * @brief Table of socket backend operations.
 *
 * This structure contains function pointers implementing the full
 * set of socket operations required by the RDNX socket API.
 * Each backend must implement the operations it supports. Unsupported
 * operations should return an appropriate error code.
 */
typedef struct rdnx_socket_backend
{
    /**
     * @brief Create a new socket.
     *
     * @param[in] net      Network interface instance.
     * @param[in] domain   Communication domain (e.g., AF_INET).
     * @param[in] type     Socket type (e.g., SOCK_STREAM, SOCK_DGRAM).
     * @param[in] protocol Protocol identifier (e.g., TCP, UDP).
     *
     * @return Socket handle on success,
     *         @ref RDNX_SOCKET_INVALID_SOCKET on failure.
     */
    rdnx_socket_t (*socket)(void *net, int32_t domain, int32_t type, int32_t protocol);

    /**
     * @brief Bind a socket to a local address.
     *
     * @param[in] sock Socket handle.
     * @param[in] addr Pointer to local address structure.
     * @param[in] len  Length of the address structure.
     *
     * @return RDNX_ERR_OK on success, or an error code on failure.
     */
    rdnx_err_t (*bind)(rdnx_socket_t sock, const rdnx_sockaddr_t *addr, rdnx_socklen_t len);

    /**
     * @brief Mark a socket as passive to accept incoming connections.
     *
     * @param[in] sock    Socket handle.
     * @param[in] backlog Maximum number of queued connections.
     *
     * @return RDNX_ERR_OK on success, or an error code on failure.
     */
    rdnx_err_t (*listen)(rdnx_socket_t sock, uint16_t backlog);

    /**
     * @brief Accept an incoming connection.
     *
     * @param[in]  sock Socket handle in listening state.
     * @param[out] addr Optional pointer to store remote address.
     * @param[in]  len  Length of the address structure.
     *
     * @return New socket handle for the accepted connection,
     *         or @ref RDNX_SOCKET_INVALID_SOCKET on failure.
     */
    rdnx_socket_t (*accept)(rdnx_socket_t sock, rdnx_sockaddr_t *addr, rdnx_socklen_t len);

    /**
     * @brief Connect a socket to a remote peer.
     *
     * @param[in] sock Socket handle.
     * @param[in] addr Pointer to remote address.
     * @param[in] len  Length of the address structure.
     *
     * @return RDNX_ERR_OK on success, or an error code on failure.
     */
    rdnx_err_t (*connect)(rdnx_socket_t sock, const rdnx_sockaddr_t *addr, rdnx_socklen_t len);

    /**
     * @brief Receive data from a connected socket.
     *
     * @param[in]  sock  Socket handle.
     * @param[out] buf   Buffer to store received data.
     * @param[in]  len   Maximum number of bytes to receive.
     * @param[in]  flags Optional receive flags.
     *
     * @return Number of bytes received on success, negative error code on failure.
     */
    int32_t (*recv)(rdnx_socket_t sock, void *buf, size_t len, uint32_t flags);

    /**
     * @brief Send data through a connected socket.
     *
     * @param[in] sock  Socket handle.
     * @param[in] buf   Data buffer to send.
     * @param[in] len   Number of bytes to send.
     * @param[in] flags Optional send flags.
     *
     * @return Number of bytes transmitted on success, negative error code on failure.
     */
    int32_t (*send)(rdnx_socket_t sock, const void *buf, size_t len, uint32_t flags);

    /**
     * @brief Send a datagram to a specific address (UDP).
     *
     * @param[in] sock Socket handle.
     * @param[in] data Data buffer to send.
     * @param[in] size Length of data.
     * @param[in] addr Destination address.
     *
     * @return Number of bytes sent on success, negative error code on failure.
     */
    int32_t (*sendto)(rdnx_socket_t sock, const void *data, uint32_t size, const rdnx_sockaddr_t *to);

    /**
     * @brief Receive a datagram from a socket (UDP).
     *
     * @param[in]  sock Socket handle.
     * @param[out] data Buffer to store received data.
     * @param[in]  size Maximum number of bytes to receive.
     * @param[out] from Optional pointer to store source address.
     *
     * @return Number of bytes received on success, negative error code on failure.
     */
    int32_t (*recvfrom)(rdnx_socket_t sock, void *data, uint32_t size, rdnx_sockaddr_t *from);

    /**
     * @brief Disable further send and/or receive operations on a socket.
     *
     * @param[in] sock Socket handle.
     * @param[in] how  Shutdown mode (e.g., send, receive, or both).
     *
     * @return RDNX_ERR_OK on success, or an error code on failure.
     */
    rdnx_err_t (*shutdown)(rdnx_socket_t sock, uint32_t how);

    /**
     * @brief Close a socket and release all associated resources.
     *
     * @param[in] sock Socket handle.
     *
     * @return RDNX_ERR_OK on success, or an error code on failure.
     */
    rdnx_err_t (*close)(rdnx_socket_t sock);

    /**
     * @brief Set options on a socket.
     *
     * @param[in] sock    Socket handle.
     * @param[in] level   Option level (e.g., SOL_SOCKET).
     * @param[in] optname Option name.
     * @param[in] optval  Pointer to option value.
     * @param[in] optlen  Size of the option value.
     *
     * @return RDNX_ERR_OK on success, or an error code on failure.
     */
    rdnx_err_t (*setsockopt)(rdnx_socket_t sock,
                             int32_t level,
                             int32_t optname,
                             const void *optval,
                             rdnx_socklen_t optlen);

    /**
    * @brief Get options from a socket.
    *
    * Retrieves the current value of a socket option.
    *
    * @param[in]     sock    Socket handle.
    * @param[in]     level   Option level (e.g., SOL_SOCKET).
    * @param[in]     optname Option name.
    * @param[out]    optval  Pointer to buffer where the option value will be stored.
    * @param[in,out] optlen  Pointer to size of the buffer. On input it contains
    *                        the buffer size, on output it contains the actual
    *                        size of the returned value.
    *
    * @return RDNX_ERR_OK on success, or an error code on failure.
    */
    rdnx_err_t (*getsockopt)(rdnx_socket_t sock,
                             int32_t level,
                             int32_t optname,
                             void *optval,
                             rdnx_socklen_t *optlen);

} rdnx_socket_backend_t;

#endif /* RDNX_SOCKET_BACKEND_H */
