/**
 * @file rdnx_tftp.c
 * @author Software development team
 * @brief RDNX TFTP Server Implementation
 * @version 0.1
 * @date 2026-03-09
 */
/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Core includes. */
#include <ethernet/w5500.h>
#include <network/rdnx_socket.h>
#include <rdnx_macro.h>
#include <rdnx_sal.h>
#include <rdnx_types.h>

#include "network/proto/rdnx_tftp.h"

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macros for the W5500 driver. */
#if defined(DEBUG)
#define TFTP_DEB(fmt, ...) printf("[TFTP]:" fmt "\n\r", ##__VA_ARGS__)
#define TFTP_ERR(fmt, ...) printf("[TFTP](ERR)(%s:%d):" fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#else
#define TFTP_DEB(fmt, ...) ((void)0)
#define TFTP_ERR(fmt, ...) ((void)0)
#endif

#define TFTP_TIMEOUT    2000

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief TFTP protocol block and datagram sizes.
 *
 * According to the TFTP specification, the maximum DATA block size
 * is 512 bytes. A full DATA datagram consists of:
 *
 * - 2 bytes opcode
 * - 2 bytes block number
 * - up to 512 bytes data
 *
 * Therefore the maximum datagram size is 516 bytes.
 */
enum
{
    blockLength = 512,     /**< Maximum TFTP data block size in bytes. */
    datagramLength = 516   /**< Maximum TFTP UDP datagram size (header + data). */
};

/**
 * @brief TFTP operation codes.
 *
 * These values define the packet type used in the
 * TFTP protocol as described in RFC 1350.
 */
enum
{
    opRRQ = 1, /**< Read Request (client requests to download a file). */
    opWRQ,     /**< Write Request (client requests to upload a file). */
    opDATA,    /**< DATA packet containing file data. */
    opACK,     /**< Acknowledgement of received DATA block. */
    opERROR,   /**< Error packet indicating transfer failure. */
};

// "netascii"
// "octet"
// "mail"

/**
 * @brief TFTP Read Request (RRQ) or Write Request (WRQ) packet.
 *
 * Format (RFC 1350):
 * @verbatim
 *  2 bytes     string    1 byte    string    1 byte
 * --------------------------------------------------
 * | Opcode |  Filename  |   0  |    Mode    |   0  |
 * --------------------------------------------------
 * @endverbatim
 */
typedef struct
{
    uint16_t opcode;   /**< TFTP operation code (opRRQ or opWRQ). */
    char *filename;    /**< Pointer to null-terminated filename string. */
    char *mode;        /**< Pointer to null-terminated transfer mode string
                          (e.g., "octet" or "netascii"). */
} tftp_rrq_wrq_t;

/**
 * @brief TFTP DATA packet.
 *
 * Format (RFC 1350):
 * @verbatim
 *  2 bytes     2 bytes      n bytes
 * ----------------------------------
 * | Opcode |   Block #  |   Data     |
 * ----------------------------------
 * @endverbatim
 *
 * The length of Data is typically 0–512 bytes. The end of file is
 * indicated by a DATA packet with length < 512 bytes.
 */
typedef struct
{
    uint16_t opcode;    /**< TFTP operation code (opDATA). */
    uint16_t block;     /**< Block number of this data packet. */
    uint8_t *buffer;    /**< Pointer to allocated buffer (capacity >= length). */
    uint8_t *data;      /**< Pointer to the actual data within the buffer. */
    uint16_t length;    /**< Number of bytes currently in `data`. */
    uint16_t capacity;  /**< Total capacity of the buffer. */
} tftp_data_t;

/** Forward declaration */
struct rdnx_tftp_ctx;

/**
 * @brief Callback type invoked on TFTP RRQ (Read Request) or WRQ (Write Request).
 *
 * This callback is called when a client initiates a read or write transfer.
 *
 * @param ctx Pointer to the TFTP server context.
 * @param rrq_wrq Pointer to the RRQ/WRQ packet structure.
 *
 * @return
 * - true  Accept the request and continue transfer.
 * - false Reject the request and abort transfer.
 */
typedef bool (*on_tftp_rrq_wrq_cb)(struct rdnx_tftp_ctx *ctx, tftp_rrq_wrq_t *rrq_wrq);

/**
 * @brief Callback type invoked on TFTP DATA packet.
 *
 * Used during file transfers to handle incoming or outgoing DATA blocks.
 *
 * @param ctx Pointer to the TFTP server context.
 * @param data Pointer to the DATA packet structure.
 *
 * @return
 * - true  DATA handled successfully.
 * - false Abort the transfer.
 */
typedef bool (*on_tftp_data_cb)(struct rdnx_tftp_ctx *ctx, tftp_data_t *data);

/**
 * @brief TFTP server state machine.
 *
 * Represents the current state of a TFTP session.
 */
typedef enum tftp_state
{
    stNoReady = -100, /**< Context not ready / uninitialized. */
    stIDLE    = 0,    /**< Idle state, waiting for request. */
    stSendData,       /**< Sending DATA packets to client (RRQ). */
    stRecvData,       /**< Receiving DATA packets from client (WRQ). */
    stWaitAck,        /**< Waiting for ACK from client after sending DATA. */
} tftp_state_t;

/* ============================================================================
 * Internal context structure (hidden from user)
 * ==========================================================================*/

/**
 * @brief TFTP server context.
 *
 * This structure represents the internal state of a TFTP server session.
 * It holds the user parameters, callbacks, socket, buffer, current block,
 * and state machine information. The structure is opaque to the user when
 * using the API.
 */
typedef struct rdnx_tftp_ctx
{
    rdnx_tftp_param_t param;   /**< User-provided parameters and callbacks. */

    rdnx_socket_t socket;      /**< UDP socket used for TFTP communication. */
    uint16_t port;             /**< Local UDP port number for the server. */
    uint8_t buffer[datagramLength]; /**< Temporary buffer for sending/receiving packets. */

    uint16_t block;            /**< Current block number in the transfer. */
    uint32_t timestamp;        /**< Timestamp of last activity (used for timeouts). */
    int prev_size;             /**< Size of the previous DATA block (used to detect EOF). */

    rdnx_sockaddr_t dest_host; /**< Remote host address (client). */
    tftp_state_t state;        /**< Current state of the TFTP session. */

    tftp_rrq_wrq_t rrq_wrq;    /**< RRQ/WRQ packet currently being processed. */
    tftp_data_t data;          /**< DATA packet currently being processed. */
} rdnx_tftp_ctx_t;

/* ============================================================================
 * Static Variables
 * ==========================================================================*/

static uint32_t delta = 0;

/* ============================================================================
 * Private Function Declarations (prototypes)
 * ==========================================================================*/

static bool tftp_server_init(struct rdnx_tftp_ctx *ctx, eth_w5500_handle_t handle, uint16_t port);
static void tftp_server_loop(struct rdnx_tftp_ctx *ctx);

static int recv_data(struct rdnx_tftp_ctx *ctx);
static int send_ack(struct rdnx_tftp_ctx *ctx, uint16_t block);
static int send_err_message(struct rdnx_tftp_ctx *ctx, uint16_t block, const char *err_msg);
static int send_data(struct rdnx_tftp_ctx *ctx);
static bool parse_rrq_wrq(struct rdnx_tftp_ctx *ctx);

/* ============================================================================
 * Internal callback wrappers
 * ==========================================================================*/

static inline bool on_tftp_rrq(struct rdnx_tftp_ctx *ctx, tftp_rrq_wrq_t *rrq)
{
    if (ctx->param.rrq)
    {
        return ctx->param.rrq(rrq->filename, rrq->mode);
    }
    return false;
}

static inline bool on_tftp_wrq(struct rdnx_tftp_ctx *ctx, tftp_rrq_wrq_t *wrq)
{
    if (ctx->param.wrq)
    {
        return ctx->param.wrq(wrq->filename, wrq->mode);
    }
    return false;
}

static inline bool on_tftp_data_in(struct rdnx_tftp_ctx *ctx, tftp_data_t *data)
{
    if (ctx->param.data_in)
    {
        return ctx->param.data_in(data->data, data->length);
    }
    return false;
}

static inline bool on_tftp_data_out(struct rdnx_tftp_ctx *ctx, tftp_data_t *data)
{
    if (ctx->param.data_out)
    {
        uint16_t len = data->capacity;
        if (ctx->param.data_out(data->buffer, &len))
        {
            data->length = len;
            data->data = data->buffer;
            return true;
        }
    }
    return false;
}

/* ============================================================================
 * Public functions
 * ==========================================================================*/
rdnx_err_t rdnx_tftp_init(const rdnx_tftp_param_t *param, rdnx_tftp_handle_t *handle)
{
    if (!param || !handle)
    {
        return RDNX_ERR_BADARG;
    }

    struct rdnx_tftp_ctx *ctx = (struct rdnx_tftp_ctx *)rdnx_alloc(sizeof(struct rdnx_tftp_ctx));
    if (ctx == NULL)
    {
        return RDNX_ERR_NOMEM;
    }

    memset(ctx, 0, sizeof(*ctx));
    memcpy(&ctx->param, param, sizeof(*param));

    // Initialize original TFTP server
    if (!tftp_server_init(ctx, param->eth_handle, param->port))
    {
        rdnx_free(ctx);
        return RDNX_ERR_FAIL;
    }

    *handle = ctx;

    return RDNX_ERR_OK;
}

void rdnx_tftp_loop(rdnx_tftp_handle_t handle)
{
    if (handle == NULL)
    {
        return;
    }
    tftp_server_loop(handle);
}

void rdnx_tftp_deinit(rdnx_tftp_handle_t handle)
{
    if (handle == NULL)
    {
        return;
    }
    (void)rdnx_sockets_close(handle->socket);
    // Cleanup TFTP server if needed (nothing in current tftp_server API)
    rdnx_free(handle);
}

static bool tftp_server_init(struct rdnx_tftp_ctx *ctx, eth_w5500_handle_t handle, uint16_t port)
{
    ctx->socket = rdnx_sockets_socket(handle, RDNX_SOCKET_AF_INET, RDNX_SOCKET_SOCK_DGRAM, RDNX_SOCKET_IPPROTO_UDP);
    rdnx_sockaddr_t addr = { .port = port };
    rdnx_err_t err = rdnx_sockets_bind(ctx->socket, &addr, sizeof(addr));
    if (err != RDNX_ERR_OK)
    {
        TFTP_ERR("Failed to create socket");
        return false;
    }

    ctx->port = port;
    ctx->block = 0;
    memset(ctx->buffer, 0, datagramLength);
    ctx->dest_host.port = 0;
    ctx->dest_host.addr.ip.v4.value = 0;
    ctx->state = stIDLE;
    ctx->data.buffer = ctx->buffer;
    ctx->data.data = &ctx->buffer[4];
    ctx->data.capacity = datagramLength - 4;
    ctx->data.length = 0;
    ctx->data.opcode = 0;
    ctx->data.block = 0;

    // send_ack(ctx,256);

    return true;
}

static void tftp_server_loop(struct rdnx_tftp_ctx *ctx)
{
    int32_t stts = recv_data(ctx);
    if (stts >= 4)
    {
        TFTP_DEB("recv %ld bytes", (long)stts);
        // minimum packet size is 4 bytes
        ctx->timestamp = rdnx_systemtime();
        switch (ctx->buffer[1])
        {
        case opRRQ:
        case opWRQ:
            ctx->prev_size = 0;
            if (ctx->buffer[1] == opRRQ)
            {
                TFTP_DEB("%s", "RRQ");
            }
            else
            {
                TFTP_DEB("%s", "WRQ");
            }
            if (ctx->state == stIDLE)
            {
                if (parse_rrq_wrq(ctx))
                {
                    ctx->timestamp = rdnx_systemtime();
                }
            }

            // else {
            //     send_err_message(ctx,-2,"invalid state");
            // }

            break;
        case opDATA:
            // incoming data
            // send ack
            // stts = recv_data(ctx);
            TFTP_DEB("%s", "DATA");
            if (ctx->state == stRecvData)
            {
                // ctx->data.data = ctx->buffer;
                ctx->data.opcode = (ctx->buffer[0] << 8) | ctx->buffer[1];
                ctx->data.block = (ctx->buffer[2] << 8) | ctx->buffer[3];
                ctx->data.length = stts - 4;

                if (ctx->data.length == 0)
                {
                    // last packet
                    ctx->state = stIDLE;
                    send_ack(ctx, ctx->data.block);
                }
                else
                {
                    if (on_tftp_data_in(ctx, &ctx->data))
                    {
                        // ctx->block = ctx->data.block;
                        TFTP_DEB("data block: %d", ctx->data.block);
                        send_ack(ctx, ctx->data.block);
                    }

                    if (ctx->data.length < blockLength)
                    {
                        // last packet
                        ctx->state = stIDLE;
                    }
                }
                ctx->timestamp = rdnx_systemtime();
            }
            else
            {
                send_err_message(ctx, -2, "invalid state");
            }
            break;
        case opACK:
            // incoming ack
            // send data
            if (ctx->state == stSendData)
            {
                uint16_t block = (ctx->buffer[2] << 8) | ctx->buffer[3];
                TFTP_DEB("ACK (%u)", block);
                if (block == ctx->data.block)
                {
                    ctx->data.block++;
                    ctx->data.length = 0;
                    if (on_tftp_data_out(ctx, &ctx->data))
                    {
                        ctx->prev_size = ctx->data.length;
                        send_data(ctx);
                    }
                    else if (ctx->prev_size == blockLength)
                    {
                        ctx->data.length = 0;
                        send_data(ctx);
                    }

                    if (ctx->data.length < blockLength)
                    {
                        // last packet
                        ctx->state = stIDLE;
                        TFTP_DEB("last packet");
                    }
                }
                ctx->timestamp = rdnx_systemtime();
            }

            //  else {
            //     send_err_message(ctx,-2,"invalid state 1");
            // }
            break;
        case opERROR:
            TFTP_DEB("%s", "ERROR");
            break;
        default:
            break;
        }
    }

    delta = rdnx_systemtime() - ctx->timestamp;

    if (delta > TFTP_TIMEOUT)
    {
        if (ctx->state != stIDLE)
        {
            ctx->state = stIDLE;
            TFTP_DEB("%s", "timeout");
            send_err_message(ctx, -5, "timeout");
        }
        ctx->timestamp = rdnx_systemtime();
    }
}

//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------
static bool parse_rrq_wrq(struct rdnx_tftp_ctx *ctx)
{
    tftp_rrq_wrq_t *rq = &ctx->rrq_wrq;

    rq->opcode = (ctx->buffer[0] << 8) | ctx->buffer[1];
    rq->filename = (char *)(&ctx->buffer[2]);

    // mode : netascii,octet,mail - max 8 char

    int len = strlen(rq->filename);

    // RRQ/WRQ packet
    //
    //  2 bytes     string    1 byte    string    1 byte
    // --------------------------------------------------
    // | Opcode |  Filename  |   0  |    Mode    |   0  |
    // --------------------------------------------------

    if ((2 + len + 1 + 8 + 1) >= datagramLength)
    {
        rq->filename = NULL;
        return false;
    }

    rq->mode = (char *)(&ctx->buffer[2 + len + 1]);

    len += strlen(rq->mode);

    if (len >= datagramLength)
    {
        rq->filename = NULL;
        rq->mode = NULL;
        return false;
    }

    if (rq->opcode == opRRQ)
    {
        // ctx->state = stSendData;
        TFTP_DEB("RRQ filename: %s , mode: %s", rq->filename, rq->mode);
        if (on_tftp_rrq(ctx, &ctx->rrq_wrq) && on_tftp_data_out(ctx, &ctx->data))
        {
            ctx->state = stSendData;
            ctx->data.block = 1;
            send_data(ctx);
            return true;
        }
        send_err_message(ctx, -1, "requested file not found");
    }
    else if (rq->opcode == opWRQ)
    {
        TFTP_DEB("WRQ filename: %s , mode: %s", rq->filename, rq->mode);
        if (on_tftp_wrq(ctx, &ctx->rrq_wrq))
        {
            send_ack(ctx, 0);
            ctx->state = stRecvData;
            return true;
        }
        send_err_message(ctx, -3, "upload not allowed");
    }

    return false;
}

static bool ll_can_send_data(rdnx_socket_t sock)
{
    uint8_t status = W5500_Sn_SR_CLOSED;
    rdnx_socklen_t len = sizeof(status);

    (void)rdnx_sockets_getsockopt(sock,
                                    0,
                                    RDNX_SOCKET_SO_STATE,
                                    &status,
                                    &len);
    if ((status != W5500_Sn_SR_UDP))
    {
        TFTP_DEB("%s", "socket is not UDP");
        return false;
    }

    uint16_t free_size = 0;
    len = sizeof(free_size);

    (void)rdnx_sockets_getsockopt(sock,
                                    0,
                                    RDNX_SOCKET_SO_TX_BUFFER_SIZE,
                                    &free_size,
                                    &len);

    // If there's room in the TX buffer, we can send data
    if (free_size > 0)
    {
        return true; // Ready to send
    }

    return false;
}

static int ll_socket_sendto(rdnx_socket_t sock, uint8_t *buf, uint16_t len, const rdnx_sockaddr_t *to)
{
    int cnt = 0;
    while (ll_can_send_data(sock) == false)
    {
        rdnx_mdelay(1);
        if (cnt++ > 10)
        {
            return -1;
        }
    }
    return rdnx_sockets_sendto(sock, buf, len, to);
}

static int send_data(struct rdnx_tftp_ctx *ctx)
{
    tftp_data_t *data = &ctx->data;
    data->buffer[0] = 0;
    data->buffer[1] = opDATA;
    data->buffer[2] = (ctx->data.block >> 8) & 0xFF;
    data->buffer[3] = ctx->data.block & 0xFF;
    // memcpy(&ctx->buffer[4], ctx->data.data, ctx->data.length);

    TFTP_DEB("send to host %d.%d.%d.%d : %d",
              ctx->dest_host.addr.ip.v4.bytes[3],
              ctx->dest_host.addr.ip.v4.bytes[2],
              ctx->dest_host.addr.ip.v4.bytes[1],
              ctx->dest_host.addr.ip.v4.bytes[0],
              ctx->dest_host.port);

    return ll_socket_sendto(ctx->socket, data->buffer, data->length + 4, &ctx->dest_host);
}

static int send_err_message(struct rdnx_tftp_ctx *ctx, uint16_t block, const char *err_msg)
{
    ctx->buffer[0] = (opERROR >> 8) & 0xFF;
    ctx->buffer[1] = opERROR & 0xFF;
    ctx->buffer[2] = (block >> 8) & 0xFF;
    ctx->buffer[3] = block & 0xFF;
    int len = strlen(err_msg);
    memcpy(&ctx->buffer[4], err_msg, len);
    len += (4);
    ctx->buffer[len] = 0;

    TFTP_DEB("send error %d.%d.%d.%d : %d  (%s)",
              ctx->dest_host.addr.ip.v4.bytes[3],
              ctx->dest_host.addr.ip.v4.bytes[2],
              ctx->dest_host.addr.ip.v4.bytes[1],
              ctx->dest_host.addr.ip.v4.bytes[0],
              ctx->dest_host.port,
              err_msg);

    ctx->state = stIDLE;

    return ll_socket_sendto(ctx->socket, ctx->buffer, len, &ctx->dest_host);
}

static int send_ack(struct rdnx_tftp_ctx *ctx, uint16_t block)
{
    uint8_t out[4] = { 0, opACK, block >> 8, block & 0xFF };

    TFTP_DEB("send ack (%d) to host %d.%d.%d.%d : %d", block,
             ctx->dest_host.addr.ip.v4.bytes[3],
             ctx->dest_host.addr.ip.v4.bytes[2],
             ctx->dest_host.addr.ip.v4.bytes[1],
             ctx->dest_host.addr.ip.v4.bytes[0],
             ctx->dest_host.port);

    return ll_socket_sendto(ctx->socket, (uint8_t *)(&out[0]), sizeof(out), &ctx->dest_host);
}

static int recv_data(struct rdnx_tftp_ctx *ctx)
{
    int32_t stts = rdnx_sockets_recvfrom(ctx->socket, ctx->buffer, datagramLength, &ctx->dest_host);
    if (stts > 0)
    {
        TFTP_DEB("recv from host  %d.%d.%d.%d : %d (%ld)",
            ctx->dest_host.addr.ip.v4.bytes[3],
            ctx->dest_host.addr.ip.v4.bytes[2],
            ctx->dest_host.addr.ip.v4.bytes[1],
            ctx->dest_host.addr.ip.v4.bytes[0],
            ctx->dest_host.port,
            (long)stts);
    }
    return stts;
}
