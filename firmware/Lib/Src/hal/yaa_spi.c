/**
 * @file    yaa_spi.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/* Library includes. */
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_spi.h> // for SPI API if disabled by default

/* Core includes. */
#include <hal/yaa_spi.h>
#include <yaa_macro.h>
#include <yaa_sal.h>
#include <yaa_slist.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

 /** @brief Debug and Error log macros for the SPI driver. */
#ifdef DEBUG
    #define SPI_DEB(fmt, ...) printf("[SPI](%s:%d):" fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
    #define SPI_ERR(fmt, ...) printf("[SPI](ERROR):" fmt "\n\r", ##__VA_ARGS__)
#else
    #define SPI_DEB(fmt, ...)   ((void)0)
    #define SPI_ERR(fmt, ...)   ((void)0)
#endif

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Struct containing the data for linking an application
 *        to a SPI Bus instance.
 */
typedef struct yaa_spibus_struct
{
    /* Node for linked list */
    sys_snode_t next;
    /* SPI instance pointer */
    SPI_HandleTypeDef *hspi;
    /* SPI instance */
    SPI_HandleTypeDef spi_handle;
    /* Define type of chip select line control.*/
    yaa_spi_cs_control_t cs_type;
    /* Count of allocated slave devices on the bus. */
    uint8_t ref_cnt;
    /* Active device */
    yaa_spi_handle_t active;
} yaa_spibus_struct_t;

/**
 * @brief Struct containing the data for linking an application
 *        to a SPI device instance.
 */
typedef struct yaa_spi_struct
{
    /** @brief Pointer to SPI bus structure */
    yaa_spibus_handle_t spi_bus;
    /** @brief The chip select GPIO handle */
    yaa_gpio_handle_t cs_handle;
    /** @brief Polarity of the chip select signal */
    yaa_spi_cs_polarity_t cs_pol;
    /** @brief Delay before transmission and releasing chip select */
    uint32_t cs_delay;
    /** @brief Enable hold-mode for a peripheral device's chip select line */
    bool cs_hold;
    /** Callback for async operation */
    void (*async_user_cb)(void *ctx);
    void *async_user_ctx;
} yaa_spi_struct_t;

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

static sys_slist_t spi_ctx_list = YAA_SLIST_STATIC_INIT();

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

static yaa_spibus_handle_t yaa_spi_get_handle(SPI_HandleTypeDef *hspi) YAA_UNUSED_FUNC;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

yaa_err_t yaa_spibus_init(const yaa_spibus_params_t *param, yaa_spibus_handle_t *handle)
{
    YAA_UNUSED(param);
    YAA_UNUSED(handle);
    uint32_t prescaler = 0u;
    SPI_TypeDef *base = NULL;
    yaa_spibus_struct_t *ctx = NULL;

    if (param == NULL || handle == NULL)
    {
        return YAA_ERR_BADARG;
    }

    ctx = (yaa_spibus_struct_t *)yaa_alloc(sizeof(yaa_spibus_struct_t));
    if (ctx == NULL)
    {
        return YAA_ERR_NOMEM;
    }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

    /* automatically select prescaler based on max_speed_hz */
    if (param->frequency != 0u)
    {
        uint32_t input_clock = HAL_RCC_GetPCLK2Freq();
        uint32_t div = input_clock / param->frequency;

        switch (div)
        {
        case 0 ... 2:
            prescaler = SPI_BAUDRATEPRESCALER_2;
            div = 2;
            break;
        case 3 ... 4:
            prescaler = SPI_BAUDRATEPRESCALER_4;
            div = 4;
            break;
        case 5 ... 8:
            prescaler = SPI_BAUDRATEPRESCALER_8;
            div = 8;
            break;
        case 9 ... 16:
            prescaler = SPI_BAUDRATEPRESCALER_16;
            div = 16;
            break;
        case 17 ... 32:
            prescaler = SPI_BAUDRATEPRESCALER_32;
            div = 32;
            break;
        case 33 ... 64:
            prescaler = SPI_BAUDRATEPRESCALER_64;
            div = 64;
            break;
        case 65 ... 128:
            prescaler = SPI_BAUDRATEPRESCALER_128;
            div = 128;
            break;
        default:
            prescaler = SPI_BAUDRATEPRESCALER_256;
            div = 256;
            break;
        }

        SPI_DEB("SPI prescaler: 0x%02X, input_clock: %d, frequency: %d", (int)prescaler, (int)input_clock, (int)(input_clock/div));
    }
    else
        prescaler = SPI_BAUDRATEPRESCALER_64;

#pragma GCC diagnostic pop

    switch (param->device_id)
    {
#if defined(SPI1)
    case YAA_SPI_1:
        base = SPI1;
        /* SPI1 clock enable */
        __HAL_RCC_SPI1_CLK_ENABLE();
        break;
#endif
#if defined(SPI2)
    case YAA_SPI_2:
        base = SPI2;
        /* SPI2 clock enable */
        __HAL_RCC_SPI2_CLK_ENABLE();
        break;
#endif
#if defined(SPI3)
    case YAA_SPI_3:
        base = SPI3;
        /* SPI3 clock enable */
        __HAL_RCC_SPI3_CLK_ENABLE();
        break;
#endif
        break;
#if defined(SPI4)
    case YAA_SPI_4:
        base = SPI4;
        /* SPI4 clock enable */
        __HAL_RCC_SPI4_CLK_ENABLE();
        break;
#endif
#if defined(SPI5)
    case YAA_SPI_5:
        base = SPI5;
        /* SPI5 clock enable */
        __HAL_RCC_SPI5_CLK_ENABLE();
        break;
#endif
#if defined(SPI6)
    case YAA_SPI_6:
        base = SPI6;
        /* SPI6 clock enable */
        __HAL_RCC_SPI6_CLK_ENABLE();
        break;
#endif
    default:
        yaa_free(ctx);
        return YAA_ERR_NORESOURCE;
    };

    ctx->hspi = param->spi;
    ctx->active = NULL;
    ctx->cs_type = param->cs_type;
    ctx->ref_cnt = 0;

    if (ctx->hspi == NULL)
    {
        ctx->spi_handle.Instance = base;
        ctx->spi_handle.Init.Mode = (param->mode == YAA_SPI_MASTER ? SPI_MODE_MASTER : SPI_MODE_SLAVE);
        ctx->spi_handle.Init.Direction = SPI_DIRECTION_2LINES;
        ctx->spi_handle.Init.DataSize =
            param->data_frame_size == YAA_SPI_DATA_FRAME_SIZE_8 ? SPI_DATASIZE_8BIT : SPI_DATASIZE_16BIT;
        ctx->spi_handle.Init.CLKPolarity =
            param->cpol == YAA_SPI_CLK_POLARITY_CPOL ? SPI_POLARITY_HIGH : SPI_POLARITY_LOW;
        ctx->spi_handle.Init.CLKPhase = param->cpha == YAA_SPI_CLK_PHASE_CPHA ? SPI_PHASE_2EDGE : SPI_PHASE_1EDGE;
        ctx->spi_handle.Init.NSS =
            param->cs_type == YAA_SPI_CS_SW_CTRL ? SPI_NSS_SOFT : SPI_NSS_HARD_OUTPUT;
        ctx->spi_handle.Init.BaudRatePrescaler = prescaler;
        ctx->spi_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
        ctx->spi_handle.Init.TIMode = SPI_TIMODE_DISABLE;
        ctx->spi_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
        ctx->spi_handle.Init.CRCPolynomial = 10;
        if (HAL_SPI_Init(&ctx->spi_handle) != HAL_OK)
        {
            yaa_free(ctx);
            return YAA_ERR_IO;
        }
        ctx->hspi = &ctx->spi_handle;
    }

    yaa_slist_append(&spi_ctx_list, &ctx->next);

#if defined(SPI_SR_TXE) && defined(__arm__)
    __HAL_SPI_ENABLE(ctx->hspi);
#endif

    *handle = ctx;

    return YAA_ERR_OK;
}

yaa_err_t yaa_spibus_free(yaa_spibus_handle_t handle)
{
    yaa_spibus_struct_t *ctx = handle;
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

#if defined(SPI_SR_TXE) && defined(__arm__)
    __HAL_SPI_DISABLE(ctx->hspi);
#endif
    (void)HAL_SPI_DeInit(ctx->hspi);

    yaa_slist_remove(&spi_ctx_list, NULL, &ctx->next);

    yaa_free(ctx);

    return YAA_ERR_OK;
}

yaa_err_t yaa_spi_init(const yaa_spi_params_t *spi_params, yaa_spibus_handle_t bus_handle,
                         yaa_spi_handle_t *spi_handle)
{
    yaa_spi_struct_t *spi = NULL;

    if ((spi_params == NULL) || (bus_handle == NULL) || (spi_handle == NULL))
    {
        return YAA_ERR_BADARG;
    }

    spi = (yaa_spi_struct_t *)yaa_alloc(sizeof(yaa_spi_struct_t));
    if (spi == NULL)
    {
        return YAA_ERR_NOMEM;
    }

    spi->spi_bus = bus_handle;
    spi->cs_handle = spi_params->cs_handle;
    spi->cs_pol = spi_params->cs_pol;
    spi->cs_delay = spi_params->cs_delay;
    spi->cs_hold = false;

    bus_handle->ref_cnt++;

    *spi_handle = spi;

    return YAA_ERR_OK;
}

yaa_err_t yaa_spi_transmitreceive(yaa_spi_handle_t handle, const uint8_t *tx_data, uint8_t *rx_data,
                                    size_t tx_data_size, size_t rx_data_size)
{
    int ret = 0;
    yaa_spi_struct_t *spi = handle;
    if (spi == NULL || spi->spi_bus == NULL)
    {
        return YAA_ERR_BADARG;
    }

    spi->spi_bus->active = spi;

    /* Assert CS */
    if (spi->spi_bus->cs_type == YAA_SPI_CS_SW_CTRL && spi->cs_handle != NULL)
    {
        yaa_gpio_set(spi->cs_handle, spi->cs_pol == YAA_SPI_CS_NCPOL ? 0 : 1);
    }

    if (spi->cs_delay)
    {
        yaa_udelay(spi->cs_delay);
    }

    if (tx_data && rx_data)
    {
        ret = HAL_SPI_TransmitReceive(spi->spi_bus->hspi, tx_data, rx_data, tx_data_size, HAL_MAX_DELAY);
    }
    else if (tx_data)
    {
        ret = HAL_SPI_Transmit(spi->spi_bus->hspi, tx_data, tx_data_size, HAL_MAX_DELAY);
    }
    else
    {
        ret = HAL_SPI_Receive(spi->spi_bus->hspi, rx_data, rx_data_size, HAL_MAX_DELAY);
    }

    // Wait until SPI is not busy (poll the BSY flag)
    while (__HAL_SPI_GET_FLAG(spi->spi_bus->hspi, SPI_FLAG_BSY))
    {
        // Optionally, add a small delay here if needed
        yaa_udelay(1); // Delay (in microseconds)
    }

    if (spi->cs_delay)
    {
        yaa_udelay(spi->cs_delay);
    }

    /* De-assert CS */
    if (!spi->cs_hold && spi->spi_bus->cs_type == YAA_SPI_CS_SW_CTRL && spi->cs_handle != NULL)
    {
        yaa_gpio_set(spi->cs_handle, spi->cs_pol == YAA_SPI_CS_NCPOL ? 1 : 0);
    }

    if (ret != HAL_OK)
    {
        if (ret == HAL_TIMEOUT)
        {
            return YAA_ERR_TIMEOUT;
        }
        else
        {
            return YAA_ERR_IO;
        }
    }

    return YAA_ERR_OK;
}

yaa_err_t yaa_spi_transmitreceive_async(yaa_spi_handle_t handle, const uint8_t *tx_data, uint8_t *rx_data,
                                          size_t tx_data_size, size_t rx_data_size, void (*callback)(void *),
                                          void *ctx)
{
    YAA_UNUSED(callback);
    YAA_UNUSED(ctx);

    int ret = 0;
    yaa_spi_struct_t *spi = handle;
    if (spi == NULL || spi->spi_bus == NULL)
    {
        return YAA_ERR_BADARG;
    }

    spi->spi_bus->active = spi;
    spi->async_user_cb = callback;
    spi->async_user_ctx = ctx;

    /* Assert CS */
    if (spi->spi_bus->cs_type == YAA_SPI_CS_SW_CTRL && spi->cs_handle != NULL)
    {
        yaa_gpio_set(spi->cs_handle, spi->cs_pol == YAA_SPI_CS_NCPOL ? 0 : 1);
    }

    if (spi->cs_delay)
    {
        yaa_udelay(spi->cs_delay);
    }

    if (tx_data && rx_data)
    {
        ret = HAL_SPI_TransmitReceive_IT(spi->spi_bus->hspi, tx_data, rx_data, tx_data_size);
    }
    else if (tx_data)
    {
        ret = HAL_SPI_Transmit_IT(spi->spi_bus->hspi, tx_data, tx_data_size);
    }
    else
    {
        ret = HAL_SPI_Receive_IT(spi->spi_bus->hspi, rx_data, rx_data_size);
    }

    if (ret != HAL_OK)
    {
        if (ret == HAL_TIMEOUT)
        {
            return YAA_ERR_TIMEOUT;
        }
        else
        {
            return YAA_ERR_IO;
        }
    }

    return YAA_ERR_OK;
}

yaa_err_t yaa_spi_write_read(yaa_spi_handle_t handle, uint8_t *data, size_t data_size)
{
    return yaa_spi_transmitreceive(handle, data, data, data_size, data_size);
}

yaa_err_t yaa_spi_release(yaa_spi_handle_t handle)
{
    yaa_spi_struct_t *spi = handle;
    if (spi == NULL || spi->spi_bus == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (spi->spi_bus->ref_cnt > 0)
        spi->spi_bus->ref_cnt--;

    spi->spi_bus = NULL;

    yaa_free(spi);

    return YAA_ERR_OK;
}

yaa_err_t yaa_spi_cs_hold(yaa_spi_handle_t handle, bool hold)
{
    yaa_spi_struct_t *spi = handle;
    if (spi == NULL)
    {
        return YAA_ERR_BADARG;
    }

    spi->cs_hold = hold;

    return YAA_ERR_OK;
}

yaa_err_t yaa_spi_abort_transfer(yaa_spi_handle_t handle)
{
    yaa_spi_struct_t *spi = handle;
    if (spi == NULL || spi->spi_bus == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (HAL_SPI_Abort(spi->spi_bus->hspi) != HAL_OK)
    {
        return YAA_ERR_FAIL;
    }

    return YAA_ERR_OK;
}

static yaa_spibus_handle_t yaa_spi_get_handle(SPI_HandleTypeDef *hspi)
{
    if (hspi != NULL)
    {
        yaa_spibus_struct_t *ctx = NULL;
        YAA_SLIST_FOR_EACH_CONTAINER(&spi_ctx_list, yaa_spibus_struct_t, ctx, next)
        {
            if (ctx->hspi->Instance == hspi->Instance)
            {
                return ctx;
            }
        }
    }
    return NULL;
}

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)
{
    yaa_spibus_struct_t *ctx = yaa_spi_get_handle(hspi);
    if (ctx != NULL)
    {
        yaa_spi_struct_t *spi = ctx->active;
        if (spi && spi->async_user_cb)
        {
            if (!spi->cs_hold && ctx->cs_type == YAA_SPI_CS_SW_CTRL && spi->cs_handle != NULL)
            {
                yaa_gpio_set(spi->cs_handle, spi->cs_pol == YAA_SPI_CS_NCPOL ? 1 : 0);
            }
            spi->async_user_cb(spi->async_user_ctx);
        }
    }
}
