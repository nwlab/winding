/**
 * @file yaa_can.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <string.h>

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Core includes. */
#include <hal/yaa_can.h>
#include <yaa_clock.h>
#include <yaa_macro.h>
#include <yaa_sal.h>
#include <yaa_slist.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief CAN bit rate configuration.
 */
typedef struct yaa_can_bit_timing_cfg
{
    /* Baud rate prescaler. Valid values: 1 - 1024. */
    uint32_t baud_rate_prescaler;
    /* Time segment 1 control. */
    uint32_t time_segment_1;
    /* Time segment 2 control. */
    uint32_t time_segment_2;
    /* Synchronization jump width. */
    uint32_t synchronization_jump_width;
} yaa_can_bit_timing_cfg_t;

/**
 * @brief Struct containing the data for linking an application to a CAN
 * instance.
 */
typedef struct yaa_can_ctx
{
    /* Node for linked list */
    sys_snode_t next;

    /* CAN handle for message, for legacy */
    CAN_HandleTypeDef *hcan;

    /* CAN handle */
    CAN_HandleTypeDef can_handle;

    /* Timeout parameter, in milliseconds. In addition to time intervals, the
     * following special values may be used:
     * #YAA_TIMO_NOWAIT.  Non-blocking.  The call will fail immediately if
     *                     no objects are available.
     * #YAA_TIMO_FOREVER.  Block indefinitely.  The call will block until
     *                      an object becomes available. */
    uint32_t timeout;

    /* Bit timing configuration. Filled with #yaa_can_calc_time_quanta. */
    yaa_can_bit_timing_cfg_t bit_timing;

    /* Temporary store for RX frame */
    yaa_can_frame_t rx_frame;

    /* Status info */
    yaa_can_status_info_t status_info;

    /* Callback to be called when the RX event occurs. */
    void (*rx_cb)(void *context, const yaa_can_frame_t *frame);
    /* Parameter to be passed when the callback is called */
    void *rx_ctx;

    /* Callback to be called when the TX complete event occurs. */
    void (*tx_complete_cb)(void *context);
    /* Parameter to be passed when the callback is called */
    void *tx_complete_ctx;

    /* Callback to be called when the TX abort event occurs. */
    void (*tx_abort_cb)(void *context);
    /* Parameter to be passed when the callback is called */
    void *tx_abort_ctx;

    /* Callback to be called when the TX abort event occurs. */
    void (*error_cb)(void *context, uint32_t error);
    /* Parameter to be passed when the callback is called */
    void *error_ctx;

} yaa_can_ctx_t;

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

static sys_slist_t can_ctx_list = YAA_SLIST_STATIC_INIT();

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

static yaa_err_t yaa_can_calc_time_quanta(yaa_can_ctx_t *ctx, uint32_t baudrate, uint32_t samplepoint);
static uint32_t yaa_can_get_clock(void);

static yaa_can_handle_t yaa_can_get_handle(CAN_HandleTypeDef *hcan);

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

yaa_err_t yaa_can_init(const yaa_can_params_t *param, yaa_can_handle_t *handle)
{
    CAN_TypeDef *base = NULL;

    if (param == NULL || handle == NULL)
    {
        return YAA_ERR_BADARG;
    }

    switch (param->device_id)
    {
#if defined(CAN1)
    case YAA_CAN_1:
        base = CAN1;
        break;
#endif
#if defined(CAN2)
    case YAA_CAN_2:
        base = CAN2;
        break;
#endif
    default:
        return YAA_ERR_BADARG;
    }

    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)yaa_alloc(sizeof(yaa_can_ctx_t));
    if (ctx == NULL)
    {
        return YAA_ERR_NOMEM;
    }

    memset(&ctx->status_info, 0, sizeof(yaa_can_status_info_t));
    ctx->hcan = param->can_handle;
    ctx->timeout = param->timeout;
    ctx->rx_cb = NULL;
    ctx->rx_ctx = NULL;
    ctx->tx_complete_cb = NULL;
    ctx->tx_complete_ctx = NULL;
    ctx->tx_abort_cb = NULL;
    ctx->tx_abort_ctx = NULL;
    ctx->error_cb = NULL;
    ctx->error_ctx = NULL;

    (void)yaa_can_calc_time_quanta(ctx, param->baudrate, 857);

    if (ctx->hcan == NULL)
    {
        ctx->can_handle.Instance = base;
        ctx->can_handle.Init.Prescaler = ctx->bit_timing.baud_rate_prescaler;
        ctx->can_handle.Init.Mode = CAN_MODE_NORMAL;
        ctx->can_handle.Init.SyncJumpWidth = ctx->bit_timing.synchronization_jump_width;
        ctx->can_handle.Init.TimeSeg1 = ctx->bit_timing.time_segment_1;
        ctx->can_handle.Init.TimeSeg2 = ctx->bit_timing.time_segment_2;
        ctx->can_handle.Init.TimeTriggeredMode = DISABLE;
        ctx->can_handle.Init.AutoBusOff = ENABLE;
        ctx->can_handle.Init.AutoWakeUp = DISABLE;
        ctx->can_handle.Init.AutoRetransmission = ENABLE;
        ctx->can_handle.Init.ReceiveFifoLocked = DISABLE;
        ctx->can_handle.Init.TransmitFifoPriority = DISABLE;
        if (HAL_CAN_Init(&ctx->can_handle) != HAL_OK)
        {
            yaa_free(ctx);
            return YAA_ERR_FAIL;
        }
        ctx->hcan = &ctx->can_handle;
    }

    yaa_slist_append(&can_ctx_list, &ctx->next);

    *handle = ctx;
    return YAA_ERR_OK;
}

yaa_err_t yaa_can_free(yaa_can_handle_t handle)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    (void)HAL_CAN_DeInit(ctx->hcan);

    yaa_slist_remove(&can_ctx_list, NULL, &ctx->next);

    yaa_free(ctx);

    return YAA_ERR_OK;
}

yaa_err_t yaa_can_set_filter(yaa_can_handle_t handle, uint8_t bank_num, yaa_canid_t filter_id,
                               yaa_canid_t filter_mask, uint32_t filter_mode, uint32_t filter_scale, uint32_t fifo)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;
    if (ctx == NULL || ctx->hcan == NULL)
    {
        return YAA_ERR_BADARG;
    }

    CAN_FilterTypeDef sFilterConfig;

    sFilterConfig.FilterBank = bank_num;
    sFilterConfig.FilterMode = filter_mode;
    sFilterConfig.FilterScale = filter_scale;
    sFilterConfig.FilterFIFOAssignment = fifo;
    sFilterConfig.FilterActivation = ENABLE;

    if ((filter_id & YAA_CAN_EFF_FLAG) == YAA_CAN_EFF_FLAG)
    {
        filter_id &= YAA_CAN_EFF_MASK;
        filter_mask &= YAA_CAN_EFF_MASK;
        // Extended ID
        sFilterConfig.FilterIdLow = (uint16_t)(filter_id << 3);
        sFilterConfig.FilterIdLow |= CAN_ID_EXT;
        sFilterConfig.FilterIdHigh = (uint16_t)(filter_id >> 13);
        sFilterConfig.FilterMaskIdLow = (uint16_t)(filter_mask << 3);
        sFilterConfig.FilterMaskIdLow |= CAN_ID_EXT;
        sFilterConfig.FilterMaskIdHigh = (uint16_t)(filter_mask >> 13);
    }
    else
    {
        filter_id &= YAA_CAN_SFF_MASK;
        filter_mask &= YAA_CAN_SFF_MASK;
        // Standard ID can be only 11 bits long
        sFilterConfig.FilterIdHigh = (uint16_t)(filter_id << 5);
        sFilterConfig.FilterIdLow = 0;
        sFilterConfig.FilterMaskIdHigh = (uint16_t)(filter_mask << 5);
        sFilterConfig.FilterMaskIdLow = 0;
    }

#ifdef CAN2
    // Filter banks from 14 to 27 are for Can2, so first for Can2 is bank 14.
    // This is not relevant for devices with only one CAN
    if (ctx->hcan->Instance == CAN1)
    {
        sFilterConfig.SlaveStartFilterBank = 14;
    }
    if (ctx->hcan->Instance == CAN2)
    {
        sFilterConfig.FilterBank = 14;
    }
#endif

    // Enable filter
    if (HAL_CAN_ConfigFilter(ctx->hcan, &sFilterConfig) != HAL_OK)
    {
        return YAA_ERR_FAIL;
    }
    else
    {
        return YAA_ERR_OK;
    }
}

yaa_err_t yaa_can_start(yaa_can_handle_t handle)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;
    if (ctx == NULL || ctx->hcan == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (HAL_CAN_Start(ctx->hcan) != HAL_OK)
    {
        if ((ctx->hcan->ErrorCode & HAL_CAN_ERROR_TIMEOUT) == HAL_CAN_ERROR_TIMEOUT)
        {
            return YAA_ERR_TIMEOUT;
        }
        else
        {
            return YAA_ERR_FAIL;
        }
    }
    else
    {
        return YAA_ERR_OK;
    }
}

yaa_err_t yaa_can_stop(yaa_can_handle_t handle)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;
    if (ctx == NULL || ctx->hcan == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (HAL_CAN_Stop(ctx->hcan) != HAL_OK)
    {
        return YAA_ERR_FAIL;
    }
    else
    {
        return YAA_ERR_OK;
    }
}

yaa_err_t yaa_can_register_rx_callback(yaa_can_handle_t handle,
                                         void (*rx_cb)(void *context, const yaa_can_frame_t *frame), void *rx_ctx)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    ctx->rx_cb = rx_cb;
    ctx->rx_ctx = rx_ctx;

    return YAA_ERR_OK;
}

yaa_err_t yaa_can_register_tx_complete_callback(yaa_can_handle_t handle, void (*tx_complete_cb)(void *context),
                                                  void *tx_complete_ctx)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    ctx->tx_complete_cb = tx_complete_cb;
    ctx->tx_complete_ctx = tx_complete_ctx;

    return YAA_ERR_OK;
}

yaa_err_t yaa_can_register_tx_abort_callback(yaa_can_handle_t handle, void (*tx_abort_cb)(void *context),
                                               void *tx_abort_ctx)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    ctx->tx_abort_cb = tx_abort_cb;
    ctx->tx_abort_ctx = tx_abort_ctx;

    return YAA_ERR_OK;
}

yaa_err_t yaa_can_register_error_callback(yaa_can_handle_t handle, void (*error_cb)(void *context, uint32_t error),
                                            void *error_ctx)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    ctx->error_cb = error_cb;
    ctx->error_ctx = error_ctx;

    return YAA_ERR_OK;
}

void *yaa_can_hw_handle(yaa_can_handle_t handle)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;

    if (ctx == NULL)
    {
        return NULL;
    }

    return ctx->hcan;
}

yaa_err_t yaa_can_write(yaa_can_handle_t handle, yaa_canid_t canid, const uint8_t *buffer, size_t size)
{
    uint32_t tx_mailbox;
    uint32_t wait_tx;
    CAN_TxHeaderTypeDef tx;
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;

    if (ctx == NULL || ctx->hcan == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (ctx->timeout != YAA_TIMO_NOWAIT)
    {
        wait_tx = yaa_systemtime();
        /* Get the number of free Tx mailboxes */
        /* Wait previous transmission complete.
         * Use only one Mailbox, otherwise ISOTP messages can be sent with
         * different sequence number if an error is happened. */
        while (HAL_CAN_GetTxMailboxesFreeLevel(ctx->hcan) != 3)
        {
            if (yaa_istimespent(wait_tx, ctx->timeout))
            {
                return YAA_ERR_TIMEOUT;
            }
        };
    }

    if ((canid & YAA_CAN_EFF_FLAG) == YAA_CAN_EFF_FLAG)
    {
        tx.IDE = CAN_ID_EXT;
        tx.ExtId = canid & YAA_CAN_EFF_MASK;
    }
    else
    {
        tx.IDE = CAN_ID_STD;
        tx.StdId = canid & YAA_CAN_SFF_MASK;
    }
    if ((canid & YAA_CAN_RTR_FLAG) == YAA_CAN_RTR_FLAG)
    {
        tx.RTR = CAN_RTR_REMOTE;
    }
    else
    {
        tx.RTR = CAN_RTR_DATA;
    }

    tx.DLC = size;

    HAL_CAN_ActivateNotification(ctx->hcan, CAN_IT_TX_MAILBOX_EMPTY);
    if (HAL_CAN_AddTxMessage(ctx->hcan, &tx, buffer, &tx_mailbox) != HAL_OK)
    {
        ctx->status_info.tx_error_counter++;
        return YAA_ERR_IO;
    }

    ctx->status_info.msgs_to_tx++;

    return YAA_ERR_OK;
}

yaa_err_t yaa_can_read(yaa_can_handle_t handle, yaa_canid_t canid, uint8_t *buffer, size_t *size)
{
    YAA_UNUSED(canid);
    CAN_RxHeaderTypeDef rx;
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;

    if (ctx == NULL || buffer == NULL)
    {
        return YAA_ERR_BADARG;
    }

    uint8_t fifo = CAN_RX_FIFO0;

    if (ctx->hcan->Instance->RF0R & CAN_RF0R_FMP0)
    {
        fifo = CAN_RX_FIFO0;
    }
    else if (ctx->hcan->Instance->RF1R & CAN_RF1R_FMP1)
    {
        fifo = CAN_RX_FIFO1;
    }
    else
    {
        return YAA_ERR_NOTFOUND;
    }

    if (ctx->hcan->Instance->sFIFOMailBox[fifo].RIR & CAN_RI0R_IDE)
    {
        // extended id we do not support
        // release fifo
        ctx->hcan->Instance->RF0R = CAN_RF0R_RFOM0;
        return YAA_ERR_NOTFOUND;
    }

    if (HAL_CAN_GetRxMessage(ctx->hcan, fifo, &rx, buffer) != HAL_OK)
    {
        return YAA_ERR_FAIL;
    }

    if (size != NULL)
    {
        *size = rx.DLC;
    }

    return YAA_ERR_OK;
}

yaa_err_t yaa_can_get_status_info(yaa_can_handle_t handle, yaa_can_status_info_t *status_info)
{
    yaa_can_ctx_t *ctx = (yaa_can_ctx_t *)handle;

    if (ctx == NULL || status_info == NULL)
    {
        return YAA_ERR_BADARG;
    }

    memcpy(status_info, &ctx->status_info, sizeof(yaa_can_status_info_t));

    return YAA_ERR_OK;
}

static yaa_can_handle_t yaa_can_get_handle(CAN_HandleTypeDef *hcan)
{
    if (hcan != NULL)
    {
        yaa_can_ctx_t *ctx = NULL;
        YAA_SLIST_FOR_EACH_CONTAINER(&can_ctx_list, yaa_can_ctx_t, ctx, next)
        {
            if (ctx->hcan->Instance == hcan->Instance)
            {
                return ctx;
            }
        }
    }

    return NULL;
}

/**
 * CAN Bit Timing
 *
 * Example for CAN bit timing:
 *   CLK on APB1 = 36 MHz
 *   BaudRate = 125 kBPs = 1 / NominalBitTime
 *   NominalBitTime = 8uS = tq + tBS1 + tBS2
 * with:
 *   tBS1 = tq * (TS1[3:0] + 1) = 12 * tq
 *   tBS2 = tq * (TS2[2:0] + 1) = 5 * tq
 *   tq = (BRP[9:0] + 1) * tPCLK
 * where tq refers to the Time quantum
 *   tPCLK = time period of the APB clock = 1 / 36 MHz
 *
 * STM32F1xx   tPCLK = 1 / 36 MHz
 * STM32F20x   tPCLK = 1 / 30 MHz
 * STM32F40x   tPCLK = 1 / 42 MHz
 *
 */
static yaa_err_t yaa_can_calc_time_quanta(yaa_can_ctx_t *ctx, uint32_t baudrate, uint32_t samplepoint)
{
    uint32_t frequency = yaa_can_get_clock();

    if (frequency % baudrate != 0)
    {
        // Configuration failed
        return YAA_ERR_BADARG;
    }

    /*
        Some equations:
        Tpclock = 1 / pclock
        Tq      = brp * Tpclock
        Tbs1    = Tq * TS1
        Tbs2    = Tq * TS2
        NominalBitTime = Tq + Tbs1 + Tbs2
        BaudRate = 1/NominalBitTime

        Bit value sample point is after Tq+Tbs1. Ideal sample point
        is at 87.5% of NominalBitTime

        Use the lowest brp where ts1 and ts2 are in valid range
     */

    uint32_t bit_clocks = frequency / baudrate; // clock ticks per bit
    uint32_t qs;                                // Number of time quanta
    // Find number of time quantas that gives us the exact wanted bit time
    for (qs = 18; qs > 9; qs--)
    {
        // check that bit_clocks / quantas is an integer
        uint32_t brp_rem = bit_clocks % qs;
        if (brp_rem == 0)
            break;
    }
    uint32_t brp = bit_clocks / qs;
    uint32_t time_seg2 = (1 + qs) / 7; // sample at ~87.5%
    uint32_t time_seg1 = qs - (1 + time_seg2);

    const uint32_t sample_cur = 1000U * (1U + time_seg1) / (1U + time_seg1 + time_seg2);
    if (sample_cur < samplepoint)
    {
        return YAA_ERR_FAIL;
    }
    ctx->bit_timing.synchronization_jump_width = 1;
    ctx->bit_timing.baud_rate_prescaler = brp;
    ctx->bit_timing.time_segment_1 = time_seg1;
    ctx->bit_timing.time_segment_2 = time_seg2;

    return YAA_ERR_OK;
}

static uint32_t yaa_can_get_clock(void)
{
    RCC_ClkInitTypeDef clkInit = { 0 };
    uint32_t flashLatency = 0;
    HAL_RCC_GetClockConfig(&clkInit, &flashLatency);

    uint32_t hclkClock = yaa_clock_getspeed(YAA_CLOCK_HCLK);
    uint8_t clockDivider = 1;
    if (clkInit.APB1CLKDivider == RCC_HCLK_DIV1)
        clockDivider = 1;
    if (clkInit.APB1CLKDivider == RCC_HCLK_DIV2)
        clockDivider = 2;
    if (clkInit.APB1CLKDivider == RCC_HCLK_DIV4)
        clockDivider = 4;
    if (clkInit.APB1CLKDivider == RCC_HCLK_DIV8)
        clockDivider = 8;
    if (clkInit.APB1CLKDivider == RCC_HCLK_DIV16)
        clockDivider = 16;

    uint32_t apb1Clock = hclkClock / clockDivider;

    return apb1Clock;
}

/* ============================================================================
 *  Interrupt callback to manage Can Rx message ready to read
 * ==========================================================================*/

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
    yaa_can_ctx_t *ctx = yaa_can_get_handle(hcan);
    if (ctx != NULL && ctx->hcan != NULL)
    {
        CAN_RxHeaderTypeDef rx;
        uint8_t fifo = CAN_RX_FIFO0;
#ifdef __arm__
        if (ctx->hcan->Instance->RF1R & CAN_RF1R_FMP1)
        {
            fifo = CAN_RX_FIFO1;
        }
        else if (ctx->hcan->Instance->RF0R & CAN_RF0R_FMP0)
        {
            fifo = CAN_RX_FIFO0;
        }
        else
        {
            return;
        }

        if (ctx->hcan->Instance->sFIFOMailBox[fifo].RIR & CAN_RI0R_IDE)
        {
            // extended id we do not support
            // release fifo
            ctx->hcan->Instance->RF0R = CAN_RF0R_RFOM0;
            ctx->status_info.rx_error_counter++;
            return;
        }
#endif // __arm__
        if (HAL_CAN_GetRxMessage(ctx->hcan, fifo, &rx, ctx->rx_frame.data) != HAL_OK)
        {
            ctx->status_info.rx_error_counter++;
            if (ctx->error_cb)
            {
                ctx->error_cb(ctx->rx_ctx, HAL_CAN_GetError(ctx->hcan));
            }
            return;
        }
        ctx->status_info.msgs_to_rx++;

        if (ctx->rx_cb)
        {
            ctx->rx_frame.can_dlc = rx.DLC;
            if (rx.IDE == CAN_ID_STD)
            {
                ctx->rx_frame.can_id = rx.StdId;
            }
            else
            {
                ctx->rx_frame.can_id = rx.ExtId | YAA_CAN_EFF_FLAG;
            }
            if (rx.RTR == CAN_RTR_REMOTE)
            {
                ctx->rx_frame.can_id |= YAA_CAN_RTR_FLAG;
            }
            ctx->rx_cb(ctx->rx_ctx, &ctx->rx_frame);
        }
    }
    __DSB(); // prevent erroneous recall of this handler due to delayed memory
             // write
}

void HAL_CAN_TxMailbox0CompleteCallback(CAN_HandleTypeDef *hcan)
{
    yaa_can_ctx_t *ctx = yaa_can_get_handle(hcan);
    if (ctx != NULL && ctx->hcan != NULL)
    {
        if (ctx->tx_complete_cb)
        {
            ctx->tx_complete_cb(ctx->tx_complete_ctx);
        }
    }
    __DSB(); // prevent erroneous recall of this handler due to delayed memory
             // write
}

void HAL_CAN_TxMailbox1CompleteCallback(CAN_HandleTypeDef *hcan)
{
    yaa_can_ctx_t *ctx = yaa_can_get_handle(hcan);
    if (ctx != NULL && ctx->hcan != NULL)
    {
        if (ctx->tx_complete_cb)
        {
            ctx->tx_complete_cb(ctx->tx_complete_ctx);
        }
    }
    __DSB(); // prevent erroneous recall of this handler due to delayed memory
             // write
}

void HAL_CAN_TxMailbox2CompleteCallback(CAN_HandleTypeDef *hcan)
{
    yaa_can_ctx_t *ctx = yaa_can_get_handle(hcan);
    if (ctx != NULL && ctx->hcan != NULL)
    {
        if (ctx->tx_complete_cb)
        {
            ctx->tx_complete_cb(ctx->tx_complete_ctx);
        }
    }
    __DSB(); // prevent erroneous recall of this handler due to delayed memory
             // write
}

void HAL_CAN_TxMailbox0AbortCallback(CAN_HandleTypeDef *hcan)
{
    yaa_can_ctx_t *ctx = yaa_can_get_handle(hcan);
    if (ctx != NULL && ctx->hcan != NULL)
    {
        ctx->status_info.tx_failed_count++;
        if (ctx->tx_abort_cb)
        {
            ctx->tx_abort_cb(ctx->tx_abort_ctx);
        }
    }
    __DSB(); // prevent erroneous recall of this handler due to delayed memory
             // write
}

void HAL_CAN_TxMailbox1AbortCallback(CAN_HandleTypeDef *hcan)
{
    yaa_can_ctx_t *ctx = yaa_can_get_handle(hcan);
    if (ctx != NULL && ctx->hcan != NULL)
    {
        ctx->status_info.tx_failed_count++;
        if (ctx->tx_abort_cb)
        {
            ctx->tx_abort_cb(ctx->tx_abort_ctx);
        }
    }
    __DSB(); // prevent erroneous recall of this handler due to delayed memory
             // write
}
void HAL_CAN_TxMailbox2AbortCallback(CAN_HandleTypeDef *hcan)
{
    yaa_can_ctx_t *ctx = yaa_can_get_handle(hcan);
    if (ctx != NULL && ctx->hcan != NULL)
    {
        ctx->status_info.tx_failed_count++;
        if (ctx->tx_abort_cb)
        {
            ctx->tx_abort_cb(ctx->tx_abort_ctx);
        }
    }
    __DSB(); // prevent erroneous recall of this handler due to delayed memory
             // write
}

void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
    yaa_can_ctx_t *ctx = yaa_can_get_handle(hcan);
    if (ctx != NULL && ctx->hcan != NULL)
    {
        uint32_t error = HAL_CAN_GetError(hcan);
        if (error & (HAL_CAN_ERROR_TX_ALST0 | HAL_CAN_ERROR_TX_ALST1 | HAL_CAN_ERROR_TX_ALST2))
        {
            ctx->status_info.arb_lost_count++;
        }
        if (error & (HAL_CAN_ERROR_RX_FOV0 | HAL_CAN_ERROR_RX_FOV1))
        {
            ctx->status_info.rx_missed_count++;
        }
        if (error & HAL_CAN_ERROR_BOF)
        {
            ctx->status_info.bus_error_count++;
        }
        if (ctx->error_cb)
        {
            ctx->error_cb(ctx->error_ctx, error);
        }
        HAL_CAN_ResetError(hcan);
    }
    __DSB(); // prevent erroneous recall of this handler due to delayed memory
             // write
}
