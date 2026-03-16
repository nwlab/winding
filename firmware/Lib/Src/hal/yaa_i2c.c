/**
 * @file    yaa_i2c.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h> // for printf

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Core includes. */
#include <hal/yaa_i2c.h>
#include <yaa_macro.h>
#include <yaa_sal.h>
#include <yaa_slist.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macroses for the LTC2946 driver. */
#ifdef DEBUG
    #define I2C_DEB(fmt, ...) printf("[I2C](%s:%d):"fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
    #define I2C_ERR(fmt, ...) printf("[I2C](ERROR):" fmt "\n\r", ##__VA_ARGS__)
#else
    #define I2C_DEB(fmt, ...)   ((void)0)
    #define I2C_ERR(fmt, ...)   ((void)0)
#endif

#define I2C_ADDR(dev_address) ((dev_address) << 1u)

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Struct containing the data for linking an application to a I2C
 * instance.
 */
typedef struct yaa_i2c_ctx
{
    /* Node for linked list */
    sys_snode_t next;
    /** @brief I2C handle pointer*/
    I2C_HandleTypeDef *hi2c;
    /** @brief I2C handle*/
    I2C_HandleTypeDef i2c_handle;
    /** @brief RX/TX timeout */
    uint32_t timeout;
} yaa_i2c_ctx_t;

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

static sys_slist_t i2c_ctx_list = YAA_SLIST_STATIC_INIT();

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

static yaa_i2c_handle_t yaa_i2c_get_handle(I2C_HandleTypeDef *hi2c) YAA_UNUSED_FUNC;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

yaa_err_t yaa_i2c_init(const yaa_i2c_params_t *param, yaa_i2c_handle_t *handle)
{
    I2C_TypeDef *base = NULL;

    if (param == NULL || handle == NULL)
    {
        return YAA_ERR_BADARG;
    }

    switch (param->device_id)
    {
#if defined(I2C1)
    case YAA_I2C_1:
        base = I2C1;
        break;
#endif
#if defined(I2C2)
    case YAA_I2C_2:
        base = I2C2;
        break;
#endif
#if defined(I2C3)
    case YAA_I2C_3:
        base = I2C3;
        break;
#endif
    default:
        return YAA_ERR_BADARG;
    }

    yaa_i2c_ctx_t *ctx = (yaa_i2c_ctx_t *)yaa_alloc(sizeof(yaa_i2c_ctx_t));
    if (ctx == NULL)
    {
        return YAA_ERR_NOMEM;
    }

    ctx->hi2c = param->i2c;
    ctx->timeout = param->timeout;

    if (ctx->hi2c == NULL)
    {
        // Peripheral is not yet Initialized
        ctx->i2c_handle.Instance = base;
        ctx->i2c_handle.Init.ClockSpeed = (param->speed == YAA_I2C_SPEED_NORMAL ? 100000 : 400000);
        ctx->i2c_handle.Init.DutyCycle = I2C_DUTYCYCLE_2;
        ctx->i2c_handle.Init.OwnAddress1 = 0;
        ctx->i2c_handle.Init.AddressingMode =
            (param->addr == YAA_I2C_ADDR_7BIT ? I2C_ADDRESSINGMODE_7BIT : I2C_ADDRESSINGMODE_10BIT);
        ctx->i2c_handle.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
        ctx->i2c_handle.Init.OwnAddress2 = 0;
        ctx->i2c_handle.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
        ctx->i2c_handle.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
        if (HAL_I2C_Init(&ctx->i2c_handle) != HAL_OK)
        {
            yaa_free(ctx);
            return YAA_ERR_FAIL;
        }
        ctx->hi2c = &ctx->i2c_handle;
    }

    yaa_slist_append(&i2c_ctx_list, &ctx->next);

    *handle = ctx;
    return YAA_ERR_OK;
}

yaa_err_t yaa_i2c_free(yaa_i2c_handle_t handle)
{
    yaa_i2c_ctx_t *ctx = handle;
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    (void)HAL_I2C_DeInit(ctx->hi2c);

    yaa_slist_remove(&i2c_ctx_list, NULL, &ctx->next);

    yaa_free(ctx);

    return YAA_ERR_OK;
}

yaa_err_t yaa_i2c_isready(yaa_i2c_handle_t handle, uint16_t dev_address, uint32_t trials, uint32_t timeout)
{
    yaa_i2c_ctx_t *ctx = handle;
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (HAL_OK == HAL_I2C_IsDeviceReady(ctx->hi2c,
                                        I2C_ADDR(dev_address),
                                        trials,
                                        timeout))
    {
        return YAA_ERR_OK;
    }

    return YAA_ERR_NORESOURCE;
}

yaa_err_t yaa_i2c_write(yaa_i2c_handle_t handle, uint16_t dev_address, uint16_t reg_address,
                          yaa_i2c_register_size_t reg_size, const uint8_t *buffer, size_t size, bool stop_condition)
{
    yaa_i2c_ctx_t *ctx = handle;

    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (reg_size == YAA_I2C_REGISTER_NONE)
    {
        if (stop_condition)
        {
            /* While the I2C in reception process, user can transmit data
             * through "buffer" buffer */
            if (HAL_I2C_Master_Transmit(ctx->hi2c, I2C_ADDR(dev_address), (uint8_t *)buffer, size, ctx->timeout) !=
                HAL_OK)
            {
                return YAA_ERR_TIMEOUT;
            }
        }
        else
        {
            HAL_I2C_Master_Seq_Transmit_IT(ctx->hi2c, I2C_ADDR(dev_address), (uint8_t *)buffer, size, I2C_FIRST_FRAME);
        }
    }
    else
    {
        uint16_t mem_addr_size = (reg_size == YAA_I2C_REGISTER_SIZE_8 ? I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT);
        if (HAL_I2C_Mem_Write(ctx->hi2c, I2C_ADDR(dev_address), reg_address, mem_addr_size, (uint8_t *)buffer, size,
                              ctx->timeout) != HAL_OK)
        {
            return YAA_ERR_TIMEOUT;
        }
    }
    return YAA_ERR_OK;
}

yaa_err_t yaa_i2c_read(yaa_i2c_handle_t handle, uint16_t dev_address, uint16_t reg_address,
                         yaa_i2c_register_size_t reg_size, uint8_t *buffer, size_t size, bool stop_condition)
{
    yaa_i2c_ctx_t *ctx = handle;

    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (reg_size == YAA_I2C_REGISTER_NONE)
    {
        if (stop_condition)
        {
            if (HAL_I2C_Master_Receive(ctx->hi2c, I2C_ADDR(dev_address), (uint8_t *)buffer, size, ctx->timeout) !=
                HAL_OK)
            {
                return YAA_ERR_TIMEOUT;
            }
        }
        else
        {
            if (HAL_I2C_Master_Seq_Receive_IT(ctx->hi2c, I2C_ADDR(dev_address), (uint8_t *)buffer, size,
                                              I2C_LAST_FRAME) != HAL_OK)
            {
                return YAA_ERR_TIMEOUT;
            }
        }
    }
    else
    {
        uint16_t mem_addr_size = (reg_size == YAA_I2C_REGISTER_SIZE_8 ? I2C_MEMADD_SIZE_8BIT : I2C_MEMADD_SIZE_16BIT);
        if (HAL_I2C_Mem_Read(ctx->hi2c, I2C_ADDR(dev_address), reg_address, mem_addr_size, (uint8_t *)buffer, size,
                             ctx->timeout) != HAL_OK)
        {
            return YAA_ERR_TIMEOUT;
        }
    }

    return YAA_ERR_OK;
}

uint8_t yaa_i2c_set_timeout(yaa_i2c_handle_t handle, uint32_t timeout)
{
    yaa_i2c_ctx_t *ctx = handle;

    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    ctx->timeout = timeout;

    return YAA_ERR_OK;
}

uint8_t yaa_i2c_detect(yaa_i2c_handle_t handle)
{
    uint8_t devices = 0u;
    yaa_i2c_ctx_t *ctx = handle;
    if (ctx == NULL)
    {
        return 0;
    }

    I2C_DEB("Searching for I2C devices on the bus...");
    /* Values outside 0x03 and 0x77 are invalid. */
    for (uint8_t i = 0x03u; i < 0x78u; i++)
    {
        uint8_t address = I2C_ADDR(i);
        /* In case there is a positive feedback, print it out. */
        if (HAL_OK == HAL_I2C_IsDeviceReady(ctx->hi2c, address, 3u,
                                            10u)) // 3 retries, 10ms timeout
        {
            I2C_DEB("Device found: 0x%02X (7bit: 0x%02X)", address, i);
            devices++;
        }
    }
    /* Feedback of the total number of devices. */
    if (0u == devices)
    {
        I2C_DEB("No device found.");
    }
    else
    {
        I2C_DEB("Total found devices: %d", devices);
    }

    return devices;
}

/* ============================================================================
 * Private Function Definitions
 * ==========================================================================*/

static yaa_i2c_handle_t yaa_i2c_get_handle(I2C_HandleTypeDef *hi2c)
{
    if (hi2c != NULL)
    {
        yaa_i2c_ctx_t *ctx = NULL;
        YAA_SLIST_FOR_EACH_CONTAINER(&i2c_ctx_list, yaa_i2c_ctx_t, ctx, next)
        {
            if (ctx->hi2c->Instance == hi2c->Instance)
            {
                return ctx;
            }
        }
    }
    return NULL;
}
