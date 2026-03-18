/**
 * @file    rdnx_pca9547.c
 * @brief   Driver for PCA9547 I2C multiplexer.
 * @version 1.0
 * @date    2026-02-04
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Core includes. */
#include <drivers/i2c/rdnx_pca9547.h>
#include <hal/rdnx_i2c.h>
#include <rdnx_macro.h>
#include <rdnx_sal.h>
#include <rdnx_types.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macroses for the PCA9547 driver. */
#ifdef DEBUG
    #define PCA9547_DEB(fmt, ...) printf("[PCA9547](%s:%d):"fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
    #define PCA9547_ERR(fmt, ...) printf("[PCA9547](ERROR):" fmt "\n\r", ##__VA_ARGS__)
#else
    #define PCA9547_DEB(fmt, ...)   ((void)0)
    #define PCA9547_ERR(fmt, ...)   ((void)0)
#endif

/* Commands for the PCA9547 */
#define PCA9547_CMD_SELECT_CHANNEL 0x08 /* Select a channel */
#define PCA9547_CMD_DESELECT_ALL   0x00 /* Deselect all channels */

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Struct containing the data for linking an application
 *        to the PCA9547 I2C multiplexer.
 */
typedef struct pca9547_ctx
{
    /** @brief I2C handle */
    rdnx_i2c_handle_t i2c;

    /** @brief Reset pin GPIO handle */
    rdnx_gpio_handle_t rst_gpio;

    /** @brief Reset pin inverse */
    bool rst_inverse;

    /** @brief I2C address of the device */
    uint8_t address;
} pca9547_ctx_t;

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

/* ============================================================================
 * Private Function Declarations
 * ==========================================================================*/

static rdnx_err_t pca9547_write_command(pca9547_ctx_t *ctx, uint8_t command);
static rdnx_err_t pca9547_read_command(pca9547_ctx_t *ctx, uint8_t *status) __attribute__((unused));

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/**
 * @brief Initializes the PCA9547 multiplexer.
 *
 * @param param Parameters for initialization
 * @param handle Pointer to the handle for the initialized device
 * @return rdnx_err_t Returns error code (RDNX_ERR_OK on success)
 */
rdnx_err_t pca9547_init(const pca9547_params_t *param, pca9547_handle_t *handle)
{
    rdnx_err_t status = RDNX_ERR_OK;

    if (param == NULL || param->i2c == NULL)
    {
        return RDNX_ERR_BADARG;
    }

#if defined(RDNX_CONFIG_I2C) && RDNX_CONFIG_I2C
    status = rdnx_i2c_isready(param->i2c, param->address, 3u, 10u);
    if (status != RDNX_ERR_OK)
    {
        return status;
    }
#endif

    pca9547_ctx_t *ctx = (pca9547_ctx_t *)rdnx_alloc(sizeof(pca9547_ctx_t));
    if (ctx == NULL)
    {
        return RDNX_ERR_NOMEM;
    }

    ctx->rst_gpio = NULL;

    if (param->rst_port != RDNX_GPIO_PORT_NONE)
    {
        rdnx_gpio_params_t gpio_rst_param = {
            .port = param->rst_port,
            .pin = param->rst_pin,
            .pull = RDNX_GPIO_PULL_UP,
            .direction = RDNX_GPIO_DIRECTION_OUTPUT,
            .cb = NULL,
        };
        status = rdnx_gpio_init(&gpio_rst_param, &ctx->rst_gpio);
        if (status != RDNX_ERR_OK)
        {
            rdnx_free(ctx);
            return status;
        }

        ctx->rst_inverse = param->rst_inverse;

        (void)rdnx_gpio_set(ctx->rst_gpio, ctx->rst_inverse ? 0 : 1);
    }

    ctx->address = param->address;
    ctx->i2c = param->i2c;

#if defined(RDNX_CONFIG_I2C) && RDNX_CONFIG_I2C
    status = rdnx_i2c_set_timeout(ctx->i2c, PCA9547_TIMEOUT_I2C);
#endif
    *handle = ctx;

    return status;
}

rdnx_err_t pca9547_destroy(pca9547_handle_t handle)
{
    pca9547_ctx_t *ctx = RDNX_CAST(pca9547_ctx_t *, handle);
    if (ctx == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    rdnx_free(ctx);

    return RDNX_ERR_OK;
}

/**
 * @brief Selects a specific channel on the PCA9547 multiplexer.
 *
 * @param handle Handle to the PCA9547 device
 * @param channel Channel to select (0-7)
 * @return rdnx_err_t Returns error code (RDNX_ERR_OK on success)
 */
rdnx_err_t pca9547_select_channel(pca9547_handle_t handle, uint8_t channel)
{
    if (channel > 7)
    {
        return RDNX_ERR_BADARG;
    }

    pca9547_ctx_t *ctx = RDNX_CAST(pca9547_ctx_t *, handle);
    if (ctx == NULL || ctx->i2c == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    uint8_t command = channel | PCA9547_CMD_SELECT_CHANNEL; // Set the corresponding bit for the channel
    return pca9547_write_command(ctx, command);
}

/**
 * @brief Deselects all channels on the PCA9547 multiplexer.
 *
 * @param handle Handle to the PCA9547 device
 * @return rdnx_err_t Returns error code (RDNX_ERR_OK on success)
 */
rdnx_err_t pca9547_deselect_all(pca9547_handle_t handle)
{
    pca9547_ctx_t *ctx = RDNX_CAST(pca9547_ctx_t *, handle);
    if (ctx == NULL || ctx->i2c == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    return pca9547_write_command(ctx, PCA9547_CMD_DESELECT_ALL);
}

/**
 * @brief Resets the PCA9547 multiplexer (deselects all channels).
 *
 * @param handle Handle to the PCA9547 device
 * @return rdnx_err_t Returns error code (RDNX_ERR_OK on success)
 */
rdnx_err_t pca9547_reset(pca9547_handle_t handle)
{
    pca9547_ctx_t *ctx = RDNX_CAST(pca9547_ctx_t *, handle);
    if (ctx == NULL || ctx->i2c == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    rdnx_err_t ret = pca9547_deselect_all(handle);

    if (ctx->rst_gpio)
    {
        (void)rdnx_gpio_set(ctx->rst_gpio, ctx->rst_inverse ? 1 : 0);
        rdnx_mdelay(1); // reset time 500ns
        (void)rdnx_gpio_set(ctx->rst_gpio, ctx->rst_inverse ? 0 : 1);
    }

    return ret;
}

/* ============================================================================
 * Private Function Definitions
 * ==========================================================================*/

/**
 * @brief Writes a command to the PCA9547 multiplexer via I2C.
 *
 * @param handle Handle to the PCA9547 device
 * @param command The command byte to send
 * @return rdnx_err_t Returns error code (RDNX_ERR_OK on success)
 */
static rdnx_err_t pca9547_write_command(pca9547_ctx_t *ctx, uint8_t command)
{
#if defined(RDNX_CONFIG_I2C) && RDNX_CONFIG_I2C
    uint8_t data[1] = { command };
    return rdnx_i2c_write(ctx->i2c, ctx->address, 0, RDNX_I2C_REGISTER_NONE, data, sizeof(data), true);
#else
    return RDNX_ERR_NOTSUP;
#endif
}

/**
 * @brief Read a status from the PCA9547 multiplexer via I2C.
 *
 * @param handle Handle to the PCA9547 device
 * @param command The status byte to read
 * @return rdnx_err_t Returns error code (RDNX_ERR_OK on success)
 */
static rdnx_err_t pca9547_read_command(pca9547_ctx_t *ctx, uint8_t *status)
{
#if defined(RDNX_CONFIG_I2C) && RDNX_CONFIG_I2C
    uint8_t data[1];

    rdnx_err_t ret = rdnx_i2c_read(ctx->i2c, ctx->address, 0, RDNX_I2C_REGISTER_NONE, data, sizeof(data), true);
    if (ret == RDNX_ERR_OK && status != NULL)
    {
        *status = data[0];
    }
    return ret;
#else
    return RDNX_ERR_NOTSUP;
#endif
}
