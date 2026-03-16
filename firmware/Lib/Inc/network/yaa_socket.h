/**
 * @file yaa_socket.h
 * @author Software development team
 * @brief Socket API
 * @version 1.0
 * @date 2025-01-29
 */
#ifndef YAA_SOCKET_H
#define YAA_SOCKET_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

 /* Standard includes. */
#include <stddef.h>
#include <stdint.h>

/* Core includes. */
#include <yaa_types.h>
#include <network/yaa_ip.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

 /**
 * @brief Options for the domain parameter of yaa_socket_socket()
 * function.
 *
 * These select the protocol family to be used for communication.
 */
#define YAA_SOCKET_AF_INET     ( 2 )                /**< IPv4 Internet Protocols. */
#define YAA_SOCKET_PF_INET     YAA_SOCKET_AF_INET  /**< IPv4 Internet Protocol. */
#define YAA_SOCKET_AF_INET6    ( 10 )               /**< IPv6 Internet Protocols. \
                                                          This option is currently not supported. */

/**
 * @brief Options for the type parameter of yaa_socket_socket()
 * function.
 *
 * These specify the communication semantics.
 */
#define YAA_SOCKET_SOCK_DGRAM     ( 2 )    /**< Datagram. */
#define YAA_SOCKET_SOCK_STREAM    ( 1 )    /**< Byte-stream. */

/**
 * @brief Options for the protocol parameter of yaa_socket_socket() function.
 */
#define YAA_SOCKET_IPPROTO_IP     ( 0 )    /**< IP. */
#define YAA_SOCKET_IPPROTO_UDP    ( 17 )   /**< UDP. This option is currently not supported. */
#define YAA_SOCKET_IPPROTO_TCP    ( 6 )    /**< TCP. */

/**
 * @brief Options for option_name in yaa_socket_setsockopt().
 */
#define YAA_SOCKET_SO_RCVTIMEO                      ( 0 )  /**< Set the receive timeout. */
#define YAA_SOCKET_SO_SNDTIMEO                      ( 1 )  /**< Set the send timeout. */
#define YAA_SOCKET_SO_SNDBUF                        ( 4 )  /**< Set the size of the send buffer (TCP only). */
#define YAA_SOCKET_SO_RCVBUF                        ( 5 )  /**< Set the size of the receive buffer (TCP only). */
#define YAA_SOCKET_SO_REQUIRE_TLS                   ( 6 )  /**< Toggle client enforcement of TLS. */
#define YAA_SOCKET_SO_SERVER_NAME_INDICATION        ( 7 )  /**< Toggle client use of TLS SNI. */
#define YAA_SOCKET_SO_TRUSTED_SERVER_CERTIFICATE    ( 8 )  /**< Set TLS server certificate trust. */
#define YAA_SOCKET_SO_CLIENT_CERTIFICATE            ( 9 )  /**< Set TLS client certificate trust. */
#define YAA_SOCKET_SO_CLIENT_KEY                    ( 10 ) /**< Set TLS client shared key. */
#define YAA_SOCKET_SO_NONBLOCK                      ( 11 ) /**< Socket is nonblocking. */
#define YAA_SOCKET_SO_ALPN_PROTOCOLS                ( 12 ) /**< Application protocol list to be included in TLS ClientHello. */
#define YAA_SOCKET_SO_TCPKEEPALIVE                  ( 13 ) /**< Enable or Disable TCP keep-alive functionality. */
#define YAA_SOCKET_SO_TCPKEEPALIVE_INTERVAL         ( 14 ) /**< Set the time in seconds between individual TCP keep-alive probes. */
#define YAA_SOCKET_SO_TCPKEEPALIVE_COUNT            ( 15 ) /**< Set the maximum number of keep-alive probes TCP should send before dropping the connection. */
#define YAA_SOCKET_SO_TCPKEEPALIVE_IDLE_TIME        ( 16 ) /**< Set the time in seconds for which the connection needs to remain idle before TCP starts sending keep-alive probes. */
#define YAA_SOCKET_SO_TCPNODELAY                    ( 17 ) /**< Don't delay send to coalesce packets */
#define YAA_SOCKET_SO_STATE                         ( 18 )
#define YAA_SOCKET_SO_CONNECTED                     ( 19 )
#define YAA_SOCKET_SO_TX_BUFFER_SIZE                ( 20 )
#define YAA_SOCKET_SO_RX_BUFFER_SIZE                ( 21 )

/**
 * @brief Options for the how parameter in yaa_socket_shutdown().
 */
#define YAA_SOCKET_SHUT_RD      ( 0 )  /**< No further receives. */
#define YAA_SOCKET_SHUT_WR      ( 1 )  /**< No further sends. */
#define YAA_SOCKET_SHUT_RDWR    ( 2 )  /**< No further send or receive. */

/* Flags we can use with yaa_socket_send and yaa_socket_recv. */
#define YAA_SOCKET_MSG_PEEK      ( 0x01 )   /**< Peeks at an incoming message */
#define YAA_SOCKET_MSG_WAITALL   ( 0x02 )   /**< Unimplemented: Requests that the function block until the full amount of data requested can be returned */
#define YAA_SOCKET_MSG_OOB       ( 0x04 )   /**< Unimplemented: Requests out-of-band data. The significance and semantics of out-of-band data are protocol-specific */
#define YAA_SOCKET_MSG_DONTWAIT  ( 0x08 )   /**< Nonblocking i/o for this operation only */
#define YAA_SOCKET_MSG_MORE      ( 0x10 )   /**< Sender will send more */
#define YAA_SOCKET_MSG_NOSIGNAL  ( 0x20 )   /**< Uninmplemented: Requests not to send the SIGPIPE signal if an attempt to send is made on a stream-oriented socket that is no longer connected. */

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief The socket handle data type.
 *
 * Data contained by the yaa_socket_handle_t type is port specific.
 */
typedef struct yaa_socket *yaa_socket_t;

/**
 * @brief Assigned to an yaa_socket_t variable when the socket is not valid.
 */
#define YAA_SOCKET_INVALID_SOCKET    ( ( yaa_socket_t ) ~0U )

/**
 * @brief The "size_t" of the socket.
 *
 * This type is used for compatibility with the expected Berkeley sockets
 * naming.
 */
typedef uint32_t yaa_socklen_t;

/**
 * @brief Socket address.
 */
typedef struct yaa_sockaddr
{
    uint8_t  socket_domain; /**< Only YAA_SOCKET_AF_INET is supported. */
    uint16_t port;          /**< Port number. Convention is to call this sin_port. */
    yaa_ip_address_t addr;    /**< IP Address. Convention is to call this sin_addr. */
} yaa_sockaddr_t;

/**
 * @brief Socket API error codes.
 *
 * These codes are returned by YAA socket operations
 * to indicate success or specific failure conditions.
 */
typedef enum yaa_socket_err
{
    YAA_SOCKET_ERR_OK              = 0,                        /**< Operation completed successfully. */
    YAA_SOCKET_ERR_BADARG          = (YAA_SOCKET_ERR_OK - 1), /**< Invalid argument provided. */
    YAA_SOCKET_ERR_NORESOURCE      = (YAA_SOCKET_ERR_OK - 2), /**< Socket or resource not available. */
    YAA_SOCKET_ERR_IO              = (YAA_SOCKET_ERR_OK - 3), /**< Input/output error or socket not in correct state. */
    YAA_SOCKET_ERR_TIMEOUT         = (YAA_SOCKET_ERR_OK - 4), /**< Operation timed out. */
    YAA_SOCKET_ERR_CONNREFUSED     = (YAA_SOCKET_ERR_OK - 5), /**< Connection was refused by the peer. */
    YAA_SOCKET_ERR_CONNRESET       = (YAA_SOCKET_ERR_OK - 6), /**< Connection was reset by the peer. */
    YAA_SOCKET_ERR_NOTCONN         = (YAA_SOCKET_ERR_OK - 7), /**< Socket is not connected. */
    YAA_SOCKET_ERR_ALREADY         = (YAA_SOCKET_ERR_OK - 8), /**< Socket is already in the requested state. */
    YAA_SOCKET_ERR_UNSUPPORTED     = (YAA_SOCKET_ERR_OK - 9), /**< Operation not supported by backend. */
    YAA_SOCKET_ERR_FAIL            = (YAA_SOCKET_ERR_OK - 10),/**< Generic failure, unspecified error. */
    YAA_SOCKET_ERR_SOCKNUM         = (YAA_SOCKET_ERR_OK - 11),/**< Invalid socket number. */
    YAA_SOCKET_ERR_SOCKOPT         = (YAA_SOCKET_ERR_OK - 12),/**< Invalid socket option. */
    YAA_SOCKET_ERR_SOCKINIT        = (YAA_SOCKET_ERR_OK - 13),/**< Socket is not initialized or SIPR is Zero IP address when Sn_MR_TCP. */
    YAA_SOCKET_ERR_SOCKCLOSED      = (YAA_SOCKET_ERR_OK - 14),/**< Socket unexpectedly closed. */
    YAA_SOCKET_ERR_SOCKMODE        = (YAA_SOCKET_ERR_OK - 15),/**< Invalid socket mode for socket operation. */
    YAA_SOCKET_ERR_SOCKFLAG        = (YAA_SOCKET_ERR_OK - 16),/**< Invalid socket flag. */
    YAA_SOCKET_ERR_SOCKSTATUS      = (YAA_SOCKET_ERR_OK - 17),/**< Invalid socket status for socket operation. */
    YAA_SOCKET_ERR_PORTZERO        = (YAA_SOCKET_ERR_OK - 18),/**< Port number is zero. */
    YAA_SOCKET_ERR_IPINVALID       = (YAA_SOCKET_ERR_OK - 19),/**< Invalid IP address. */
    YAA_SOCKET_ERR_DATALEN         = (YAA_SOCKET_ERR_OK - 20),/**< Data length is zero or greater than buffer max size. */
    YAA_SOCKET_ERR_BUFFER          = (YAA_SOCKET_ERR_OK - 21),/**< Socket buffer is not enough for data communication. */

} yaa_socket_err_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief               Creates a TCP socket.
 *
 *                      This call allocates memory and claims a socket resource.
 *
 * @param[in] net       Network interface instance.
 * @param[in] domain:   Must be set to YAA_SOCKET_AF_INET. See @ref SocketDomains.
 * @param[in] type:     Set to YAA_SOCKET_SOCK_STREAM to create a TCP socket.
 *                      No other value is valid.
 * @param[in] protocol: Set to YAA_SOCKET_IPPROTO_TCP to create a TCP socket.
 *                      No other value is valid. See @ref Protocols.
 *
 * @return
 *   If a socket is created successfully, then the socket handle is returned
 *   @ref YAA_SOCKET_INVALID_SOCKET is returned if an error occurred.
 */
yaa_socket_t yaa_sockets_socket(void *net,
                                  int32_t domain,
                                  int32_t type,
                                  int32_t protocol);

/**
 * @brief               Bind a TCP socket.
 *
 * @param[in] socket:   The handle of the socket to which specified address to be bound.
 * @param[in] addr:     A pointer to a yaa_sockaddr_t structure that contains
 *                      the address and port to be bound to the socket.
 * @param[in] addrlen:  Should be set to sizeof( @ref yaa_sockaddr_t ).
 *
 * @return
 *  If the bind was successful then @ref YAA_ERR_OK is returned.
 *  If an error occurred, a negative value is returned.
 */
yaa_err_t yaa_sockets_bind(yaa_socket_t socket,
                             const yaa_sockaddr_t *addr,
                             yaa_socklen_t addrlen);

/**
 * @brief               Prepare the endpoint to receive TCP messages.
 *
 * @param[in] socket:   The handle of the socket to which specified address to be bound.
 * @param[in] backlog:  Maximum depth of connection acceptance queue
 *
 * @return
 *   If the operation was successful, 0 is returned.
 *   If an error occurred, a negative value is returned. See socket errors.
 */
yaa_err_t yaa_sockets_listen(yaa_socket_t socket, uint16_t backlog);

/**
 * @brief               Accept a new connection on a socket.
 *                      The purpose of the @ref yaa_socket_accept function is
 *                      to block until a client attempts to connect to
 *                      the server socket, once a connection request is received.
 *
 * @param[in] socket:   The handle of the socket.
 * @param[out] addr:    A pointer to a yaa_sockaddr_t structure that contains
 *                      the accepted client socket address.
 * @param[in,out] addrlen:  Should be set to sizeof( @ref yaa_sockaddr_t ).
 *
 * @return
 *  If the bind was successful then @ref YAA_ERR_OK is returned.
 *  If an error occurred, a negative value is returned.
 */
yaa_socket_t yaa_sockets_accept(yaa_socket_t socket,
                                  yaa_sockaddr_t *addr,
                                  yaa_socklen_t addrlen);

/**
 * @brief              Connects the socket to the specified IP address and port.
 *
 *                     The socket must first have been successfully created by
 *                     a call to yaa_socket_socket().
 *
 *                     To create a secure socket, yaa_socket_setsockopt() should be called
 *                     with the YAA_SOCKET_SO_REQUIRE_TLS option before
 *                     yaa_socket_connect() is called.
 *
 *                     If this function returns an error the socket is considered invalid.
 *
 * @param[in] socket:  The handle of the socket to be connected.
 * @param[in] addr:    A pointer to a yaa_sockaddr_t structure that contains the
 *                     the address to connect the socket to.
 * @param[in] addrlen: Should be set to sizeof( @ref yaa_sockaddr_t ).
 *
 * @return
 *   @ref YAA_ERR_OK if a connection is established.
 *   If an error occurred, a negative value is returned.
 */
yaa_err_t yaa_sockets_connect(yaa_socket_t socket,
                                const yaa_sockaddr_t *addr,
                                yaa_socklen_t addrlen);

/**
 * @brief             Receive data from a TCP socket.
 *
 *                    The socket must have already been created using a call
 *                    to yaa_socket_socket() and connected to a remote socket
 *                    using yaa_socket_connect().
 *
 * @param[in] socket: The handle of the socket from which data is being received.
 * @param[out] buf:   The buffer into which the received data will be placed.
 * @param[in] len:    The maximum number of bytes which can be received.
 *                    buf must be at least len bytes long.
 * @param[in] flags:  Not currently used. Should be set to 0.
 *
 * @return
 *   If the receive was successful then the number of bytes received
 *   (placed in the buffer pointed to by buf) is returned.
 *   If a timeout occurred before data could be received then 0 is returned.
 *   If an error occurred, a negative value is returned.
 */
int32_t yaa_sockets_recv(yaa_socket_t socket,
                          void *buf,
                          size_t len,
                          uint32_t flags);

/**
 * @brief             Transmit data to the remote socket.
 *
 *                    The socket must have already been created using a call
 *                    to yaa_socket_socket() and connected to a remote socket
 *                    using yaa_socket_connect().
 *
 * @param[in] socket: The handle of the sending socket.
 * @param[in] buf:    The buffer containing the data to be sent.
 * @param[in] len:    The length of the data to be sent.
 * @param[in] flags:  Not currently used. Should be set to 0.
 *
 * @return
 *   On success, the number of bytes actually sent is returned.
 *   If an error occurred, a negative value is returned.
 */
int32_t yaa_sockets_send(yaa_socket_t socket,
                          const void *buf,
                          size_t len,
                          uint32_t flags);

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
int32_t yaa_sockets_recvfrom(yaa_socket_t socket, void *data, uint32_t size, yaa_sockaddr_t *from);

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
int32_t yaa_sockets_sendto(yaa_socket_t socket,
                            const void *data,
                            uint32_t size,
                            const yaa_sockaddr_t *to);

/**
 * @brief             Closes all or part of a full-duplex connection on the socket.
 *
 *                    Disable reads and writes on a connected TCP socket.
 *                    A connected TCP socket must be gracefully shut down before it can be closed.
 *
 * @param[in] socket: The handle of the socket to shutdown.
 * @param[in] how:    YAA_SOCKET_SHUT_RD, YAA_SOCKET_SHUT_WR or YAA_SOCKET_SHUT_RDWR.
 *
 * @return
 *   If the operation was successful, 0 is returned.
 *   If an error occurred, a negative value is returned. See socket errors.
 */
yaa_err_t yaa_sockets_shutdown(yaa_socket_t socket,
                                 uint32_t how);

/**
 * @brief             Closes the socket and frees the related resources.
 *
 *                    A socket should be shutdown gracefully before it is closed,
 *                    and cannot be used after it has been closed.
 *
 * @param[in] socket: The handle of the socket to close.
 *
 * @return
 *   On success, 0 is returned.
 *   If an error occurred, a negative value is returned. See socket errors.
 */
yaa_err_t yaa_sockets_close(yaa_socket_t socket);

/**
 * @brief              Manipulates the options for the socket.
 *
 * @param[in] socket:  The handle of the socket to set the option for.
 * @param[in] level:   Not currently used. Should be set to 0.
 * @param[in] optname: An option name.
 * @param[in] optval:  A buffer containing the value of the option to set.
 * @param[in] optlen:  The length of the buffer pointed to by pvOptionValue.
 *
 *  - Berkeley Socket Options
 *    - @ref YAA_ERR_TIMEOUT
 *      - Sets the receive timeout
 *      - optval (uint32_t) is the number of milliseconds that the
 *      receive function should wait before timing out.
 *      - Setting optval = 0 causes receive to wait forever.
 *    - @ref YAA_SOCKET_SO_SNDTIMEO
 *      - Sets the send timeout
 *      - optval (uint32_t) is the number of milliseconds that the
 *      send function should wait before timing out.
 *      - Setting optval = 0 causes send to wait forever.
 *  - Non-Standard Options
 *    - @ref YAA_SOCKET_SO_NONBLOCK
 *      - Makes a socket non-blocking.
 *      - Non-blocking connect is not supported - socket option should be
 *        called after connect.
 *      - optval is ignored for this option.
 *  - Security Sockets Options
 *    - @ref YAA_SOCKET_SO_REQUIRE_TLS
 *      - Use TLS for all connect, send, and receive on this socket.
 *      - This socket options MUST be set for TLS to be used, even
 *        if other secure socket options are set.
 *      - This socket option should be set before yaa_socket_connect() is
 *        called.
 *      - optval is ignored for this option.
 *    - @ref YAA_SOCKET_SO_TRUSTED_SERVER_CERTIFICATE
 *      - Set the root of trust server certificate for the socket.
 *      - This socket option only takes effect if @ref YAA_SOCKET_SO_REQUIRE_TLS
 *        is also set.  If @ref YAA_SOCKET_SO_REQUIRE_TLS is not set,
 *        this option will be ignored.
 *      - optval is a pointer to the formatted server certificate.
 *      - optlen (uint32_t) is the length of the certificate
 *        in bytes.
 *    - @ref YAA_SOCKET_SO_SERVER_NAME_INDICATION
 *      - Use Server Name Indication (SNI)
 *      - This socket option only takes effect if @ref SOCKETS_SO_REQUIRE_TLS
 *        is also set.  If @ref YAA_SOCKET_SO_REQUIRE_TLS is not set,
 *        this option will be ignored.
 *      - optval is a pointer to a string containing the hostname
 *      - optlen is the length of the hostname string in bytes.
 *    - @ref YAA_SOCKET_SO_ALPN_PROTOCOLS
 *      - Negotiate an application protocol along with TLS.
 *      - The ALPN list is expressed as an array of NULL-terminated ANSI
 *        strings.
 *      - optlen is the number of items in the array.
 *    - @ref YAA_SOCKET_SO_TCPKEEPALIVE
 *      - Enable or disable the TCP keep-alive functionality.
 *      - optval is the value to enable or disable Keepalive.
 *    - @ref YAA_SOCKET_SO_TCPKEEPALIVE_INTERVAL
 *      - Set the time in seconds between individual TCP keep-alive probes.
 *      - optval is the time in seconds.
 *    - @ref YAA_SOCKET_SO_TCPKEEPALIVE_COUNT
 *      - Set the maximum number of keep-alive probes TCP should send before
 *        dropping the connection.
 *      - optval is the maximum number of keep-alive probes.
 *    - @ref YAA_SOCKET_SO_TCPKEEPALIVE_IDLE_TIME
 *      - Set the time in seconds for which the connection needs to remain idle
 *        before TCP starts sending keep-alive probes.
 *      - optval is the time in seconds.
 *
 * @return
 *   On success, 0 is returned.
 *   If an error occurred, a negative value is returned. See socket errors
 */
yaa_err_t yaa_sockets_setsockopt(yaa_socket_t socket,
                                   int32_t level,
                                   int32_t optname,
                                   const void *optval,
                                   yaa_socklen_t optlen);

/**
 * @brief              Retrieves options for the socket.
 *
 * @param[in]  socket:  The handle of the socket to get the option from.
 * @param[in]  level:   Not currently used. Should be set to 0.
 * @param[in]  optname: An option name.
 * @param[out] optval:  A buffer where the option value will be stored.
 * @param[in,out] optlen:
 *                      On input, the length of the buffer pointed to by optval.
 *                      On output, the size of the returned option value.
 *
 *  - Berkeley Socket Options
 *    - @ref YAA_ERR_TIMEOUT
 *      - Gets the receive timeout.
 *      - optval (uint32_t) receives the number of milliseconds that the
 *        receive function waits before timing out.
 *      - A value of 0 indicates infinite wait.
 *
 *    - @ref YAA_SOCKET_SO_SNDTIMEO
 *      - Gets the send timeout.
 *      - optval (uint32_t) receives the number of milliseconds that the
 *        send function waits before timing out.
 *      - A value of 0 indicates infinite wait.
 *
 *  - Non-Standard Options
 *    - @ref YAA_SOCKET_SO_NONBLOCK
 *      - Gets the non-blocking mode state.
 *      - optval (uint32_t) receives non-zero if socket is non-blocking,
 *        otherwise zero.
 *
 *  - Security Socket Options
 *    - @ref YAA_SOCKET_SO_REQUIRE_TLS
 *      - Gets whether TLS is enabled for the socket.
 *      - optval (uint32_t) receives non-zero if TLS is required.
 *
 *    - @ref YAA_SOCKET_SO_TCPKEEPALIVE
 *      - Gets TCP keep-alive enable state.
 *      - optval (uint32_t) receives non-zero if keep-alive is enabled.
 *
 *    - @ref YAA_SOCKET_SO_TCPKEEPALIVE_INTERVAL
 *      - Gets the time in seconds between TCP keep-alive probes.
 *      - optval (uint32_t) receives the interval value.
 *
 *    - @ref YAA_SOCKET_SO_TCPKEEPALIVE_COUNT
 *      - Gets the maximum number of TCP keep-alive probes.
 *      - optval (uint32_t) receives the probe count.
 *
 *    - @ref YAA_SOCKET_SO_TCPKEEPALIVE_IDLE_TIME
 *      - Gets idle time before TCP keep-alive starts.
 *      - optval (uint32_t) receives the idle time in seconds.
 *
 * @return
 *   On success, 0 is returned.
 *   If an error occurred, a negative value is returned. See socket errors.
 */
yaa_err_t yaa_sockets_getsockopt(yaa_socket_t socket,
                                   int32_t level,
                                   int32_t optname,
                                   void *optval,
                                   yaa_socklen_t *optlen);

#ifdef __cplusplus
}
#endif

#endif /* YAA_SOCKET_H */
