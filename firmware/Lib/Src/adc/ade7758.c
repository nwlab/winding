/**
 * @file    ade7758.c
 * @author  Software development team
 * @brief   ADE7758 3-Phase Energy Metering IC driver implementation
 *
 * @version 1.0
 * @date    2026-02-09
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Core includes. */
#include <adc/ade7758.h>
#include <hal/yaa_gpio.h>
#include <hal/yaa_spi.h>
#include <yaa_sal.h>
#include <yaa_types.h>
#include <yaa_macro.h>
#include <yaa_util.h>

/* Temporary use LL API */
#include "stm32f4xx_ll_spi.h"
#include <stm32f4xx_hal.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macros for the ADE7758 driver. */
#ifdef DEBUG
    #if defined(__GNUC__) || defined(__clang__)
        // GCC/Clang: allow zero arguments safely with ##__VA_ARGS__
        #define ADE7758_DEB(fmt, ...) printf("[ADE7758](%s:%d):"fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__)
        #define ADE7758_ERR(fmt, ...) printf("[ADE7758](ERROR):" fmt "\r\n", ##__VA_ARGS__)
    #else
        // Portable C99: requires at least one argument
        #define ADE7758_DEB(fmt, ...) printf(fmt, __VA_ARGS__)
        #define ADE7758_ERR(fmt, ...) printf(fmt, __VA_ARGS__)
    #endif
#else
    #define ADE7758_DEB(fmt, ...)   ((void)0)
    #define ADE7758_ERR(fmt, ...)   ((void)0)
#endif

/* ============================================================================
 * Private Types
 * ==========================================================================*/

/**
 * @brief Internal ADE7758 context
 */
typedef struct ade7758_ctx
{
    yaa_spi_handle_t  spi; /**< SPI handle */
    yaa_gpio_handle_t cs;  /**< Chip-select GPIO handle */
    yaa_gpio_handle_t irq; /**< IRQ pin GPIO handle */
    /* Function pointers for user-defined lock/unlock mechanisms */
    yaa_err_t (*lock)(void);     /**< User-defined lock function */
    yaa_err_t (*unlock)(void);   /**< User-defined unlock function */
} ade7758_ctx_t;

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

static void ade7758_irq_callback(yaa_gpio_port_t port, yaa_gpio_pin_t pin, void *ctx);

/* ============================================================================
 * Private Helpers
 * ==========================================================================*/

/**
 * @brief Pull CS line low
 *
 * @param ctx ADE7758 context
 *
 * @return yaa_err_t
 */
static inline yaa_err_t ade7758_cs_low(const ade7758_ctx_t *ctx)
{
    return yaa_gpio_set(ctx->cs, 0);
}

/**
 * @brief Pull CS line high
 *
 * @param ctx ADE7758 context
 *
 * @return yaa_err_t
 */
static inline yaa_err_t ade7758_cs_high(const ade7758_ctx_t *ctx)
{
    return yaa_gpio_set(ctx->cs, 1);
}

static inline yaa_err_t ade7758_lock(const ade7758_ctx_t *ctx)
{
    /* Call user-defined lock function if provided */
    if (ctx->lock != NULL)
    {
        yaa_err_t err = ctx->lock();  // Lock the device
        if (err != YAA_ERR_OK)
        {
            return err;  // If lock failed, return error
        }
    }
    return YAA_ERR_OK;
}

static inline yaa_err_t ade7758_unlock(const ade7758_ctx_t *ctx)
{
    /* Call user-defined unlock function if provided */
    if (ctx->unlock != NULL)
    {
        yaa_err_t err = ctx->unlock();  // Unlock the device after initialization
        if (err != YAA_ERR_OK)
        {
            return err;  // If unlock failed, return error
        }
    }
    return YAA_ERR_OK;
}

/**
 * @brief Perform SPI transfer
 *
 * @param ctx ADE7758 context
 * @param tx Transmit buffer
 * @param rx Receive buffer
 * @param len Number of bytes
 *
 * @return yaa_err_t
 */
static yaa_err_t ade7758_spi_transfer(ade7758_ctx_t *ctx, const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    return yaa_spi_transmitreceive(ctx->spi, tx, rx, len, len);
}

/**
 * @brief Reset the device using SW reset.
 * @param ctx ADE7758 context
 * @return yaa_err_t
 */
static yaa_err_t ade7758_sw_reset(ade7758_ctx_t *ctx)
{
    yaa_err_t err = YAA_ERR_OK;

    /* Software reset */
    uint8_t opmode = ADE7758_OPMODE_SWRST;
    err = ade7758_write_reg(ctx, ADE7758_REG_OPMODE, &opmode, 1U);
    if (err == YAA_ERR_OK)
    {
        /* Datasheet: Software Chip Reset. A data transfer to the ADE7758 should not take place for at least 166 μs after a software reset.*/
        /* Datasheet: wait at least 18 µs after reset */
        yaa_udelay(20);
    }
    return err;
}

/**
 * @brief Get interrupt indicator from STATUS register.
 * @param ctx ADE7758 context
 * @param msk - Interrupt mask.
 * @param status - Status indicator.
 * @return yaa_err_t
 */
yaa_err_t ade7758_get_int_status(ade7758_ctx_t *ctx, uint32_t msk, uint8_t *status)
{
    yaa_err_t err;
    /* register value read */
    uint32_t reg_val = 0x00000000;

    if (!status)
    {
        return YAA_ERR_BADARG;
    }

    err = ade7758_read_reg(ctx, ADE7758_REG_STATUS, (uint8_t *)&reg_val, 3);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    ADE7758_DEB("REG status : 0x%08X", (int)reg_val);

    *status = yaa_test_bit(yaa_find_first_set_bit(msk), &reg_val);

    return err;
}

/**
 * @brief Clear irq status flags.
 * @param ctx ADE7758 context
 * @param reg_data - value of the status register
 * @return 0 in case of success, negative error code otherwise.
 */
yaa_err_t ade7758_clear_irq_status(ade7758_ctx_t *ctx, uint32_t *reg_data)
{
    yaa_err_t err;
    uint32_t data;

    if (!reg_data)
    {
        return YAA_ERR_BADARG;
    }

    err = ade7758_read_reg(ctx, ADE7758_REG_RSTATUS, (uint8_t*)&data, 3);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    *reg_data = data;

    return err;
}

/* ============================================================================
 * Public API
 * ==========================================================================*/

/**
 * @brief Initialize ADE7758 device
 *
 * Allocates context, initializes CS GPIO and SPI, performs software reset.
 *
 * @param[in]  param   Initialization parameters
 * @param[out] handle  Device handle
 *
 * @return yaa_err_t
 * @retval YAA_ERR_OK        Success
 * @retval YAA_ERR_BADARG    NULL pointer passed
 * @retval YAA_ERR_NOMEM     Memory allocation failed
 */
yaa_err_t ade7758_init(const ade7758_params_t *param, ade7758_handle_t *handle)
{
    yaa_err_t err = YAA_ERR_OK;

    if ((param == NULL) || (handle == NULL))
    {
        ADE7758_ERR("Bad arguments");
        return YAA_ERR_BADARG;
    }

    ade7758_ctx_t *ctx = yaa_alloc(sizeof(struct ade7758_ctx));
    if (ctx == NULL)
    {
        ADE7758_ERR("No memory");
        return YAA_ERR_NOMEM;
    }

    ctx->lock = param->lock;
    ctx->unlock = param->unlock;
    ctx->spi = param->spi;

    if (param->cs_port != YAA_GPIO_PORT_NONE)
    {
        yaa_gpio_params_t gpio_cs_param = {
            .port = param->cs_port,
            .pin = param->cs_pin,
            .pull = YAA_GPIO_PULL_UP,
            .direction = YAA_GPIO_DIRECTION_OUTPUT,
            .cb = NULL,
        };
        err = yaa_gpio_init(&gpio_cs_param, &ctx->cs);
        if (err != YAA_ERR_OK)
        {
            yaa_free(ctx);
            return err;
        }
        (void)yaa_gpio_set(ctx->cs, 1);
    }

    if (param->irq_port != YAA_GPIO_PORT_NONE)
    {
        yaa_gpio_params_t gpio_irq_param = {
            .port = param->irq_port,
            .pin = param->irq_pin,
            .pull = YAA_GPIO_PULL_UP,
            .direction = YAA_GPIO_DIRECTION_INTERRUPT,
            .irq_trigger = YAA_GPIO_IRQ_TRIGGER_EDGE_FALLING,
            .cb = ade7758_irq_callback,
            .user_ctx = ctx
        };
        err = yaa_gpio_init(&gpio_irq_param, &ctx->irq);
        if (err != YAA_ERR_OK)
        {
            yaa_free(ctx);
            return err;
        }
        (void)yaa_gpio_set(ctx->cs, 1);
    }

    ade7758_cs_high(ctx);

    err = ade7758_sw_reset(ctx);
    if (err != YAA_ERR_OK)
    {
        yaa_free(ctx);
        return err;
    }

    /* value read from register */
    uint32_t status = 0;
    /* timeout value */
    uint16_t timeout = 10;
    /* wait for reset ack */
    do
    {
        err = ade7758_get_int_status(ctx, ADE7758_INT_RESET, (uint8_t *)&status);
        if (err != YAA_ERR_OK)
        {
            ADE7758_ERR("Fail to get IRQ status");
            yaa_free(ctx);
            return err;
        }
        yaa_mdelay(100);
    } while ((!status) && (--timeout));
    if (!timeout)
    {
        ADE7758_ERR("Reset software timeout");
        yaa_free(ctx);
        return YAA_ERR_TIMEOUT;
    }

    /* reset the status register */
    uint32_t reg_val = 0;
    err = ade7758_clear_irq_status(ctx, &reg_val);
    if (err != YAA_ERR_OK)
    {
        ADE7758_ERR("Fail to clear IRQ status");
        yaa_free(ctx);
        return err;
    }

    uint8_t version = 0xFF;
    (void)ade7758_get_version(ctx, &version);
    ADE7758_DEB("ADE7758 Module version : 0x%02X", version);

    if (version == 0xFF)
    {
        ADE7758_ERR("The version is 0xFF, there is no device");
        yaa_free(ctx);
        return YAA_ERR_NORESOURCE;
    }

    /* For debug to check SPI communication with default values after reset on the ADE7758 */
#if 1
    uint8_t data = 0xFF;
    (void)ade7758_read_reg(ctx, ADE7758_REG_MMODE, (uint8_t*)&data, 1);
    if (data != 0xFC) {
        ADE7758_DEB("ADE7758 MMODE : 0x%02X", data);
    }
    (void)ade7758_read_reg(ctx, ADE7758_REG_COMPMODE, (uint8_t*)&data, 1);
    if (data != 0x1C) {
        ADE7758_DEB("ADE7758 COMPMODE : 0x%02X", data);
    }
    (void)ade7758_read_reg(ctx, ADE7758_REG_LCYCMODE, (uint8_t*)&data, 1);
    if (data != 0x78) {
        ADE7758_DEB("ADE7758 LCYCMODE : 0x%02X", data);
    }
#endif

    *handle = YAA_CAST(ade7758_handle_t, ctx);

    return YAA_ERR_OK;
}

/**
 * @brief Destroy ADE7758 handle and free resources
 *
 * Deinitializes GPIO and releases memory. After this call, the handle is invalid.
 *
 * @param handle ADE7758 device handle
 *
 * @return yaa_err_t
 * @retval YAA_ERR_OK      Success
 * @retval YAA_ERR_BADARG  NULL handle
 */
yaa_err_t ade7758_destroy(ade7758_handle_t handle)
{
    if (handle == NULL)
    {
        return YAA_ERR_BADARG;
    }

    ade7758_cs_high(handle);

    yaa_free(handle);
    return YAA_ERR_OK;
}

/**
 * @brief Read ADE7758 register(s)
 *
 * @param handle ADE7758 device handle
 * @param reg Register address
 * @param data Output buffer
 * @param length Number of bytes to read
 *
 * @return yaa_err_t
 */
yaa_err_t ade7758_read_reg(ade7758_handle_t handle, ade7758_reg_t reg, uint8_t *data, uint16_t length)
{
    yaa_err_t err = YAA_ERR_OK;

    if ((handle == NULL) || (data == NULL) || (length == 0))
    {
        return YAA_ERR_BADARG;
    }

    uint8_t cmd = (uint8_t)reg | ADE7758_SPI_CMD_READ;

    (void)ade7758_cs_low(handle);

    // 50 ns (min) CS falling edge to first SCLK falling edge
    yaa_udelay(50);

    err = ade7758_spi_transfer(handle, &cmd, NULL, 1);
    if (err == YAA_ERR_OK)
    {
        // 4 μs (min) Minimum time between read command (that is, a write to communication register) and data read
        yaa_udelay(5);
        err = ade7758_spi_transfer(handle, NULL, data, length);
    }
    // 100 ns (max), 10 ns (min) Bus relinquish time after falling edge of SCLK
    yaa_udelay(50);

    (void)ade7758_cs_high(handle);

    // If multiple bytes are read, swap them to match the system's expected endianness
    if (err == YAA_ERR_OK && length > 1)
    {
        for (uint16_t i = 0; i < length / 2; i++)
        {
            uint8_t temp = data[i];
            data[i] = data[length - i - 1];
            data[length - i - 1] = temp;
        }
    }

#if defined(DEBUG) && 0
    if (length == 1) {
        ADE7758_DEB("Reg : 0x%02X [%02X]", reg, data[0]);
    } else if (length == 2) {
        ADE7758_DEB("Reg : 0x%02X [%02X, %02X]", reg, data[0], data[1]);
    } else if (length == 3) {
        ADE7758_DEB("Reg : 0x%02X [%02X, %02X, %02X]", reg, data[0], data[1], data[2]);
    }
#endif
    return err;
}

/**
 * @brief Write ADE7758 register(s)
 *
 * @param handle ADE7758 device handle
 * @param reg Register address
 * @param data Input buffer
 * @param length Number of bytes to write
 *
 * @return yaa_err_t
 */
yaa_err_t ade7758_write_reg(ade7758_handle_t handle, ade7758_reg_t reg, const uint8_t *data, uint16_t length)
{
    if ((handle == NULL) || (data == NULL) || (length == 0))
    {
        return YAA_ERR_BADARG;
    }

    uint8_t cmd = (uint8_t)reg | ADE7758_SPI_CMD_WRITE;

    ade7758_cs_low(handle);

    // 50 ns (min) CS falling edge to first SCLK falling edge
    yaa_udelay(50);

    yaa_err_t err = ade7758_spi_transfer(handle, &cmd, NULL, 1);
    if (err == YAA_ERR_OK)
    {
        // If more than one byte, swap them (for multi-byte registers)
        uint8_t swapped_data[length];
        if (length > 1)
        {
            // Swap bytes to match the system's expected endianness
            for (uint16_t i = 0; i < length / 2; i++)
            {
                swapped_data[i] = data[length - i - 1];
                swapped_data[length - i - 1] = data[i];
            }

            // If length is odd, leave the middle byte in place (no swap)
            if (length % 2 != 0)
            {
                swapped_data[length / 2] = data[length / 2];
            }
        }
        else
        {
            // No swap needed for a single byte
            memcpy(swapped_data, data, length);
        }
        // 400 ns (min) Minimum time between byte transfers during a serial write
        yaa_udelay(50);
        err = ade7758_spi_transfer(handle, swapped_data, NULL, length);
    }
    // 100 ns (min) CS hold time after SCLK falling edge
    yaa_udelay(50);

    (void)ade7758_cs_high(handle);

#ifdef DEBUG
    if (length == 1) {
        ADE7758_DEB("Reg : 0x%02X [%02X]", reg, data[0]);
    } else if (length == 2) {
        ADE7758_DEB("Reg : 0x%02X [%02X, %02X]", reg, data[0], data[1]);
    } else if (length == 3) {
        ADE7758_DEB("Reg : 0x%02X [%02X, %02X, %02X]", reg, data[0], data[1], data[2]);
    }
#endif
    return err;
}

/**
 * @brief Read instantaneous active energy (AWATTHR)
 *
 * @param handle ADE7758 device handle
 * @param value Pointer to energy value (16-bit signed)
 *
 * @return yaa_err_t
 */
yaa_err_t ade7758_read_active_energy(ade7758_handle_t handle, int32_t *value)
{
    if ((handle == NULL) || (value == NULL))
    {
        return YAA_ERR_BADARG;
    }

    uint8_t buf[2];

    yaa_err_t err = ade7758_read_reg(handle, ADE7758_REG_AWATTHR, buf, sizeof(buf));
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    /* Big-endian 16-bit signed */
    *value = (int16_t)((buf[0] << 8) | buf[1]);

    return YAA_ERR_OK;
}

/**
 * @brief Get device version
 *
 * @param handle ADE7758 device handle
 * @param version Pointer to store version register value
 *
 * @return yaa_err_t
 */
yaa_err_t ade7758_get_version(ade7758_handle_t handle, uint8_t *version)
{
    if ((handle == NULL) || (version == NULL))
    {
        return YAA_ERR_BADARG;
    }

    return ade7758_read_reg(handle, ADE7758_REG_VERSION, version, 1U);
}

/**
 * @brief Read all primary ADE7758 measurements
 *
 * @param handle  ADE7758 device handle
 * @param values  Pointer to measurement structure
 *
 * @return yaa_err_t
 */
yaa_err_t ade7758_read_all(ade7758_handle_t handle, ade7758_values_t *values)
{
    if ((handle == NULL) || (values == NULL))
    {
        return YAA_ERR_BADARG;
    }

    yaa_err_t err;

    /* --- Active energy --- */
    err = ade7758_read_reg(handle, ADE7758_REG_AWATTHR, (uint8_t *)&values->awatt_code, 2);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    err = ade7758_read_reg(handle, ADE7758_REG_BWATTHR, (uint8_t *)&values->bwatt_code, 2);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    err = ade7758_read_reg(handle, ADE7758_REG_CWATTHR, (uint8_t *)&values->cwatt_code, 2);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    /* --- RMS current --- */
    err = ade7758_read_reg(handle, ADE7758_REG_AIRMS, (uint8_t *)&values->airms_code, 3);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    err = ade7758_read_reg(handle, ADE7758_REG_BIRMS, (uint8_t *)&values->birms_code, 3);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    err = ade7758_read_reg(handle, ADE7758_REG_CIRMS, (uint8_t *)&values->cirms_code, 3);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    /* --- RMS voltage --- */
    err = ade7758_read_reg(handle, ADE7758_REG_AVRMS, (uint8_t *)&values->avrms_code, 3);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    err = ade7758_read_reg(handle, ADE7758_REG_BVRMS, (uint8_t *)&values->bvrms_code, 3);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    err = ade7758_read_reg(handle, ADE7758_REG_CVRMS, (uint8_t *)&values->cvrms_code, 3);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    /* --- System values --- */
    err = ade7758_read_reg(handle, ADE7758_REG_FREQ, (uint8_t *)&values->freq_code, 2);
    if (err != YAA_ERR_OK)
    {
        return err;
    }

    err = ade7758_read_reg(handle, ADE7758_REG_TEMP, (uint8_t *)&values->temp_code, 1);

    return err;
}

yaa_err_t ade7758_get_temperature(ade7758_handle_t handle,
                                   uint16_t *temperature)
{
    if ((handle == NULL) || (temperature == NULL))
    {
        return YAA_ERR_BADARG;
    }

    yaa_err_t err;
    uint8_t value;

    err = ade7758_read_reg(handle, ADE7758_REG_TEMP, &value, 1);

    *temperature = value;

    return err;
}

yaa_err_t ade7758_stop_device(ade7758_handle_t handle)
{
    if (handle == NULL)
    {
        return YAA_ERR_BADARG;
    }

    yaa_err_t err;
    uint8_t value = 0x00;

    (void)ade7758_read_reg(handle, ADE7758_REG_OPMODE, &value, 1);

    value |= 7 << 3;  /* ADE7758 powered down */

    err = ade7758_write_reg(handle, ADE7758_REG_OPMODE, &value, 1);

	return err;
}

void ade7758_irq_handler(ade7758_handle_t handle)
{
    uint32_t irq_status = 0;

    (void)ade7758_read_reg(handle, ADE7758_REG_STATUS, (uint8_t *)&irq_status, 3);

    if (irq_status & ADE7758_INT_PKV) {
        // Handle peak vlotage event
    }

    if (irq_status & ADE7758_INT_PKI) {
        // Handle peak current event (overcurrent)
    }

    // clear the IRQ flags
    (void)ade7758_write_reg(handle, ADE7758_REG_RSTATUS, (const uint8_t *)&irq_status, 3);
}

float ade7758_code_to_voltage(uint32_t code,
                              float vref_mv,
                              float divider)
{
    /* 24-bit RMS register */
    return ((float)code / (1UL << 24)) * (vref_mv / 1000.0f) * divider;
}

float ade7758_code_to_current(uint32_t code,
                              float vref_mv,
                              float shunt_ohm)
{
    return ((float)code / (1UL << 24)) *
           (vref_mv / 1000.0f) / shunt_ohm;
}

float ade7758_code_to_energy(int32_t code,
                             float energy_lsb)
{
    return (float)code * energy_lsb;
}

float ade7758_code_to_frequency(uint16_t code)
{
    /* Datasheet: line period derived frequency */
    return (code != 0U) ? (256000.0f / code) : 0.0f;
}

float ade7758_code_to_temperature(int16_t code)
{
    /* Typical ADE7758 temp slope */
    return 25.0f + ((float)code / 32.0f);
}

/**
 * @brief Print ADE7758 values
 *
 * This function prints the values from the ADE7758 device, including active energy,
 * reactive energy, apparent energy, RMS current, RMS voltage, and system measurements.
 *
 * @param v Pointer to ADE7758 values structure
 */
void ade7758_print(const ade7758_values_t *v, float vref_mv, float divider, float shunt_ohm, float energy_lsb)
{
    if (v == NULL)
    {
        printf("ade7758_values: NULL pointer\n\r");
        return;
    }

    printf("ADE7758 Values:\n\r");

    /* Active Energy */
    printf("  Active Energy:\n\r");
    printf("    Phase A : %.2f Wh\n\r", ade7758_code_to_energy(v->awatt_code, energy_lsb));
    printf("    Phase B : %.2f Wh\n\r", ade7758_code_to_energy(v->bwatt_code, energy_lsb));
    printf("    Phase C : %.2f Wh\n\r", ade7758_code_to_energy(v->cwatt_code, energy_lsb));

    /* Reactive Energy */
    printf("  Reactive Energy:\n\r");
    printf("    Phase A : %.2f VARh\n\r", ade7758_code_to_energy(v->avar_code, energy_lsb));
    printf("    Phase B : %.2f VARh\n\r", ade7758_code_to_energy(v->bvar_code, energy_lsb));
    printf("    Phase C : %.2f VARh\n\r", ade7758_code_to_energy(v->cvar_code, energy_lsb));

    /* Apparent Energy */
    printf("  Apparent Energy:\n\r");
    printf("    Phase A : %.2f VAh\n\r", ade7758_code_to_energy(v->ava_code, energy_lsb));
    printf("    Phase B : %.2f VAh\n\r", ade7758_code_to_energy(v->bva_code, energy_lsb));
    printf("    Phase C : %.2f VAh\n\r", ade7758_code_to_energy(v->cva_code, energy_lsb));

    /* RMS Current */
    printf("  RMS Current:\n\r");
    printf("    Phase A : %.3f A\n\r", ade7758_code_to_current(v->airms_code, vref_mv, shunt_ohm));
    printf("    Phase B : %.3f A\n\r", ade7758_code_to_current(v->birms_code, vref_mv, shunt_ohm));
    printf("    Phase C : %.3f A\n\r", ade7758_code_to_current(v->cirms_code, vref_mv, shunt_ohm));

    /* RMS Voltage */
    printf("  RMS Voltage:\n\r");
    printf("    Phase A : %.3f V\n\r", ade7758_code_to_voltage(v->avrms_code, vref_mv, divider));
    printf("    Phase B : %.3f V\n\r", ade7758_code_to_voltage(v->bvrms_code, vref_mv, divider));
    printf("    Phase C : %.3f V\n\r", ade7758_code_to_voltage(v->cvrms_code, vref_mv, divider));

    /* System Measurements */
    printf("  System Measurements:\n\r");
    printf("    Line Frequency : %.2f Hz\n\r", ade7758_code_to_frequency(v->freq_code));
    printf("    Temperature    : %.2f °C\n\r", ade7758_code_to_temperature(v->temp_code));
    printf("\n\r");
}

static void ade7758_irq_callback(yaa_gpio_port_t port, yaa_gpio_pin_t pin, void *ctx)
{
    YAA_UNUSED(port);
    YAA_UNUSED(pin);
    YAA_UNUSED(ctx);
    ADE7758_DEB("Irq port : %d, pin :%d", port, (int)pin);
    ade7758_irq_handler(ctx);
}
