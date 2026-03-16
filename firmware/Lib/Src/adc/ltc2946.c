/**
 * @file    ltc2946.c
 * @brief   Driver for the LTC2946 Battery Gas Gauge and Monitor.
 *
 * This file implements the functions to interact with the LTC2946 device via I2C.
 * It includes initialization, reading and writing data, and managing the device registers.
 *
 * @version 1.0
 * @date    2026-02-04
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdio.h>

/* Core includes. */
#include <adc/ltc2946.h>
#include <hal/yaa_i2c.h>
#include <yaa_macro.h>
#include <yaa_sal.h>
#include <yaa_types.h>

// clang-format off

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macroses for the LTC2946 driver. */
#ifdef DEBUG
    #define LTC2946_DEB(fmt, ...) printf("[LTC2946](%s:%d):"fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__)
    #define LTC2946_ERR(fmt, ...) printf("[LTC2946](ERROR):" fmt "\r\n", ##__VA_ARGS__)
#else
    #define LTC2946_DEB(fmt, ...)   ((void)0)
    #define LTC2946_ERR(fmt, ...)   ((void)0)
#endif

// Enable for sending dummy values without hardware
// #define LTC2946_DUMMY

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Struct representing the context of an LTC2946 device.
 *
 * This structure contains the necessary data for communicating with an LTC2946
 * device, including the I2C handle and the device's I2C address.
 */
typedef struct ltc2946_ctx
{
    /**
     * @brief I2C handle for communication with the LTC2946 device.
     */
    yaa_i2c_handle_t i2c;

    /**
     * @brief I2C address of the LTC2946 device.
     */
    uint8_t address;
} ltc2946_ctx_t;

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

/**
 * @brief Write an 8-bit value to a specified register in the LTC2946.
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to write to.
 * @param code 8-bit data to write.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_write(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                uint8_t code) YAA_UNUSED_FUNC;

/**
 * @brief Write a 16-bit value to a specified register in the LTC2946.
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to write to.
 * @param code 16-bit data to write.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_write_16_bits(ltc2946_ctx_t *ctx,
                                        uint8_t adc_command,
                                        uint16_t code) YAA_UNUSED_FUNC;

/**
 * @brief Write a 24-bit value to a specified register in the LTC2946.
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to write to.
 * @param code 24-bit data to write.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_write_24_bits(ltc2946_ctx_t *ctx,
                                        uint8_t adc_command,
                                        uint32_t code) YAA_UNUSED_FUNC;

/**
 * @brief Write a 32-bit value to a specified register in the LTC2946.
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to write to.
 * @param code 32-bit data to write.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_write_32_bits(ltc2946_ctx_t *ctx,
                                        uint8_t adc_command,
                                        uint32_t code) YAA_UNUSED_FUNC;

/**
 * @brief Read an 8-bit value from a specified register in the LTC2946.
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to read from.
 * @param adc_code Pointer to store the 8-bit read value.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_read(ltc2946_ctx_t *ctx, uint8_t adc_command,
                               uint8_t *adc_code) YAA_UNUSED_FUNC;

/**
 * @brief Read a 12-bit value from a specified register in the LTC2946.
 *
 * LTC2946 12-bit registers are stored across two bytes:
 *  - First byte: MSB [11:4]
 *  - Second byte: LSB [3:0 in upper nibble]
 *
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to read from.
 * @param adc_code Pointer to store the 12-bit read value.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_read_12_bits(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                       uint16_t *adc_code) YAA_UNUSED_FUNC;

/**
 * @brief Read a 16-bit value from a specified register in the LTC2946.
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to read from.
 * @param adc_code Pointer to store the 16-bit read value.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_read_16_bits(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                       uint16_t *adc_code) YAA_UNUSED_FUNC;

/**
 * @brief Read a 24-bit value from a specified register in the LTC2946.
 *
 * LTC2946 24-bit registers are stored as:
 *   - First byte: MSB [23:16]
 *   - Second byte: MID [15:8]
 *   - Third byte: LSB [7:0]
 *
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to read from.
 * @param adc_code Pointer to store the 24-bit read value.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_read_24_bits(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                       uint32_t *adc_code) YAA_UNUSED_FUNC;

/**
 * @brief Read a 32-bit value from a specified register in the LTC2946.
 *
 * LTC2946 32-bit registers are stored as:
 *   - First byte: MSB [31:24]
 *   - Second byte: [23:16]
 *   - Third byte: [15:8]
 *   - Fourth byte: LSB [7:0]
 *
 * @param ctx Context structure for LTC2946.
 * @param adc_command Register address to read from.
 * @param adc_code Pointer to store the 32-bit read value.
 * @return yaa_err_t Error code.
 */
static yaa_err_t LTC2946_read_32_bits(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                       uint32_t *adc_code) YAA_UNUSED_FUNC;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/**
 * @brief Initializes the LTC2946 device.
 *
 * This function initializes the LTC2946 device by setting its default register
 * values and performing necessary checks, such as verifying the I2C address
 * and checking the device readiness.
 *
 * @param param Pointer to the initialization parameters for the LTC2946 device.
 * @param handle Pointer to store the context (handle) of the initialized device.
 * @return yaa_err_t Error code, YAA_ERR_OK on success.
 */
yaa_err_t ltc2946_init(const ltc2946_params_t *param,
                        ltc2946_handle_t *handle)
{
    // Validating I2C address
    if (!(param->address == LTC2946_I2C_ADDRESS_0 ||
          param->address == LTC2946_I2C_ADDRESS_1 ||
          param->address == LTC2946_I2C_ADDRESS_2 ||
          param->address == LTC2946_I2C_ADDRESS_3 ||
          param->address == LTC2946_I2C_ADDRESS_4 ||
          param->address == LTC2946_I2C_ADDRESS_5 ||
          param->address == LTC2946_I2C_ADDRESS_6 ||
          param->address == LTC2946_I2C_ADDRESS_7 ||
          param->address == LTC2946_I2C_ADDRESS_8 ||
          param->address == LTC2946_I2C_MASS_WRITE_7BIT))
    {
        LTC2946_ERR("LTC2946 bad address");
        return YAA_ERR_BADARG;
    }

    // Check if device is ready
#ifndef LTC2946_DUMMY
    if (yaa_i2c_isready(param->i2c, param->address, 3u, 10u) != YAA_ERR_OK)
    {
        LTC2946_ERR("LTC2946 not ready on 0x%02X", param->address);
        return YAA_ERR_NOTFOUND;
    }
#endif

    // Allocate memory for the context
    ltc2946_ctx_t *ctx = (ltc2946_ctx_t *)yaa_alloc(sizeof(ltc2946_ctx_t));
    if (ctx == NULL)
    {
        LTC2946_ERR("LTC2946 no memory");
        return YAA_ERR_NOMEM;
    }

    ctx->address = param->address;
    ctx->i2c = param->i2c;

    // Set default register values
    uint8_t CTRLA = LTC2946_CHANNEL_CONFIG_V_C_3 |
                    LTC2946_SENSE_PLUS |
                    LTC2946_OFFSET_CAL_EVERY |
                    LTC2946_ADIN_GND;
    uint8_t CTRLB = LTC2946_DISABLE_ALERT_CLEAR &
                    LTC2946_DISABLE_SHUTDOWN &
                    LTC2946_DISABLE_CLEARED_ON_READ &
                    LTC2946_DISABLE_STUCK_BUS_RECOVER &
                    LTC2946_ENABLE_ACC &
                    LTC2946_DISABLE_AUTO_RESET;
    uint8_t GPIO_CFG = LTC2946_GPIO1_OUT_LOW |
                       LTC2946_GPIO2_IN_ACC |
                       LTC2946_GPIO3_OUT_ALERT;
    uint8_t GPIO3_CTRL = LTC2946_GPIO3_OUT_HIGH_Z;

    // Apply register configurations
    (void)LTC2946_write(ctx, LTC2946_CTRLA_REG, CTRLA);
    (void)LTC2946_write(ctx, LTC2946_CTRLB_REG, CTRLB);
    (void)LTC2946_write(ctx, LTC2946_GPIO_CFG_REG, GPIO_CFG);
    (void)LTC2946_write(ctx, LTC2946_GPIO3_CTRL_REG, GPIO3_CTRL);

    *handle = ctx;

    return YAA_ERR_OK;
}

/**
 * @brief Frees the resources allocated for the LTC2946 context.
 *
 * This function frees the memory allocated for the LTC2946 context structure.
 *
 * @param handle The handle (context) of the initialized LTC2946 device.
 * @return yaa_err_t Error code, YAA_ERR_OK on success.
 */
yaa_err_t ltc2946_destroy(ltc2946_handle_t handle)
{
    ltc2946_ctx_t *ctx = handle;

    if (ctx != NULL)
    {
        yaa_free(ctx);
    }

    return YAA_ERR_OK;
}

/**
 * @brief Reads values from the LTC2946 device.
 *
 * This function reads various power, current, voltage, energy, and charge values
 * from the LTC2946 device and stores them in the provided structure.
 *
 * @param handle Handle of the initialized LTC2946 device.
 * @param values Pointer to a structure to store the read values.
 * @return yaa_err_t Error code, YAA_ERR_OK on success.
 */
yaa_err_t ltc2946_read(ltc2946_handle_t handle, ltc2946_values_t *values)
{
    ltc2946_ctx_t *ctx = handle;

    if (ctx == NULL || values == NULL)
    {
        return YAA_ERR_BADARG;
    }

#ifndef LTC2946_DUMMY
    // Read various register values from the LTC2946 device
    LTC2946_read_24_bits(ctx, LTC2946_POWER_MSB2_REG, &values->power_code);
    LTC2946_read_24_bits(ctx, LTC2946_MAX_POWER_MSB2_REG, &values->max_power_code);
    LTC2946_read_24_bits(ctx, LTC2946_MIN_POWER_MSB2_REG, &values->min_power_code);
    LTC2946_read_12_bits(ctx, LTC2946_DELTA_SENSE_MSB_REG, &values->current_code);
    LTC2946_read_12_bits(ctx, LTC2946_MAX_DELTA_SENSE_MSB_REG, &values->max_current_code);
    LTC2946_read_12_bits(ctx, LTC2946_MIN_DELTA_SENSE_MSB_REG, &values->min_current_code);
    LTC2946_read_12_bits(ctx, LTC2946_VIN_MSB_REG, &values->VIN_code);
    LTC2946_read_12_bits(ctx, LTC2946_MAX_VIN_MSB_REG, &values->max_VIN_code);
    LTC2946_read_12_bits(ctx, LTC2946_MIN_VIN_MSB_REG, &values->min_VIN_code);
    LTC2946_read_12_bits(ctx, LTC2946_ADIN_MSB_REG, &values->ADIN_code);
    LTC2946_read_12_bits(ctx, LTC2946_MAX_ADIN_MSB_REG, &values->max_ADIN_code);
    LTC2946_read_12_bits(ctx, LTC2946_MIN_ADIN_MSB_REG, &values->min_ADIN_code);
    LTC2946_read_32_bits(ctx, LTC2946_ENERGY_MSB3_REG, &values->energy_code);
    LTC2946_read_32_bits(ctx, LTC2946_CHARGE_MSB3_REG, &values->charge_code);
    LTC2946_read_32_bits(ctx, LTC2946_TIME_COUNTER_MSB3_REG, &values->time_code);
#else
    // Dummy values when no hardware is present
    values->power_code = 0x800;
    values->max_power_code = 0xFFF;
    values->min_power_code = 0x000;
    values->current_code = 0x600;
    values->max_current_code = 0xFFF;
    values->min_current_code = 0x000;
    values->VIN_code = 0x800;
    values->max_VIN_code = 0xFFF;
    values->min_VIN_code = 0x000;
    values->ADIN_code = 0x100;
    values->max_ADIN_code = 0xFFF;
    values->min_ADIN_code = 0x000;
    values->energy_code = 0;
    values->charge_code = 0;
    values->time_code = 0;
#endif
    return YAA_ERR_OK;
}

// Write an 8-bit code to the LTC2946.
static yaa_err_t LTC2946_write(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                uint8_t code)
{
    return yaa_i2c_write(ctx->i2c, ctx->address, adc_command,
                          YAA_I2C_REGISTER_SIZE_8, &code, 1, 0);
}

// Write a 16-bit code to the LTC2946
static yaa_err_t LTC2946_write_16_bits(ltc2946_ctx_t *ctx,
                                        uint8_t adc_command, uint16_t code)
{
    uint8_t buf[2];

    buf[0] = (code >> 8) & 0xFF; // MSB
    buf[1] = code & 0xFF;        // LSB

    return yaa_i2c_write(ctx->i2c, ctx->address, adc_command,
                          YAA_I2C_REGISTER_SIZE_8, buf, 2, 0);
}

// Write a 24-bit code to the LTC2946
static yaa_err_t LTC2946_write_24_bits(ltc2946_ctx_t *ctx,
                                        uint8_t adc_command, uint32_t code)
{
    uint8_t buf[3];

    buf[0] = (code >> 16) & 0xFF; // MSB
    buf[1] = (code >> 8) & 0xFF;  // MID
    buf[2] = code & 0xFF;         // LSB

    return yaa_i2c_write(ctx->i2c, ctx->address, adc_command,
                          YAA_I2C_REGISTER_SIZE_8, buf, 3, 0);
}

// Write a 32-bit code to the LTC2946
static yaa_err_t LTC2946_write_32_bits(ltc2946_ctx_t *ctx,
                                        uint8_t adc_command, uint32_t code)
{
    uint8_t buf[4];

    buf[0] = (code >> 24) & 0xFF; // MSB
    buf[1] = (code >> 16) & 0xFF;
    buf[2] = (code >> 8) & 0xFF;
    buf[3] = code & 0xFF;         // LSB

    return yaa_i2c_write(ctx->i2c, ctx->address, adc_command,
                          YAA_I2C_REGISTER_SIZE_8, buf, 4, 0);
}

// Reads an 8-bit adc_code from LTC2946
static yaa_err_t LTC2946_read(ltc2946_ctx_t *ctx, uint8_t adc_command,
                               uint8_t *code)
{
    return yaa_i2c_read(ctx->i2c, ctx->address, adc_command,
                         YAA_I2C_REGISTER_SIZE_8, code, 1, 0);
}

// Reads a 12-bit adc_code from LTC2946
static yaa_err_t LTC2946_read_12_bits(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                       uint16_t *code)
{
    yaa_err_t res;
    uint8_t buf[2];

    // Read 2 bytes from LTC2946
    res = yaa_i2c_read(ctx->i2c, ctx->address, adc_command,
                         YAA_I2C_REGISTER_SIZE_8, buf, 2, 0);
    if (res != YAA_ERR_OK)
    {
        return res;
    }

    // Reconstruct 12-bit ADC value: MSB[11:4], LSB[3:0]
    *code = (((uint16_t)buf[0] << 4) | (buf[1] >> 4)) & 0x0FFF;

    return YAA_ERR_OK;
}

// Reads a 16-bit adc_code from LTC2946
static yaa_err_t LTC2946_read_16_bits(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                       uint16_t *code)
{
    yaa_err_t res;
    uint8_t buf[2];

    // Read 2 bytes from LTC2946
    res = yaa_i2c_read(ctx->i2c, ctx->address, adc_command,
                         YAA_I2C_REGISTER_SIZE_8, buf, 2, 0);
    if (res != YAA_ERR_OK)
    {
        return res;
    }

    // Combine MSB and LSB into 16-bit code
    *code = ((uint16_t)buf[0] << 8) | buf[1];

    return YAA_ERR_OK;
}

// Reads a 24-bit adc_code from LTC2946
static yaa_err_t LTC2946_read_24_bits(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                       uint32_t *code)
{
    yaa_err_t res;
    uint8_t buf[3];

    // Read 3 bytes from LTC2946
    res = yaa_i2c_read(ctx->i2c, ctx->address, adc_command,
                         YAA_I2C_REGISTER_SIZE_8, buf, 3, 0);
    if (res != YAA_ERR_OK)
    {
        return res;
    }

    // Combine bytes into 24-bit value
    *code = ((uint32_t)buf[0] << 16) | ((uint32_t)buf[1] << 8) | buf[2];

    return YAA_ERR_OK;
}

// Reads a 32-bit adc_code from LTC2946
static yaa_err_t LTC2946_read_32_bits(ltc2946_ctx_t *ctx, uint8_t adc_command,
                                       uint32_t *code)
{
    yaa_err_t res;
    uint8_t buf[4];

    // Read 4 bytes from LTC2946
    res = yaa_i2c_read(ctx->i2c, ctx->address, adc_command,
                         YAA_I2C_REGISTER_SIZE_8, buf, 4, 0);
    if (res != YAA_ERR_OK)
        return res;

    // Combine bytes into 32-bit value (MSB first)
    *code = ((uint32_t)buf[0] << 24) |
            ((uint32_t)buf[1] << 16) |
            ((uint32_t)buf[2] << 8)  |
            buf[3];

    return YAA_ERR_OK;
}

void ltc2946_print(const ltc2946_values_t *v)
{
    if (v == NULL)
    {
        printf("ltc2946_values: NULL pointer\n\r");
        return;
    }

    printf("LTC2946 Values:\n\r");

    printf("  Power:\n\r");
    printf("    current : %lu\n\r", (unsigned long)v->power_code);
    printf("    max     : %lu\n\r", (unsigned long)v->max_power_code);
    printf("    min     : %lu\n\r", (unsigned long)v->min_power_code);

    printf("  Current:\n\r");
    printf("    current : %u\n\r", v->current_code);
    printf("    max     : %u\n\r", v->max_current_code);
    printf("    min     : %u\n\r", v->min_current_code);

    printf("  VIN:\n\r");
    printf("    current : %u\n\r", v->VIN_code);
    printf("    max     : %u\n\r", v->max_VIN_code);
    printf("    min     : %u\n\r", v->min_VIN_code);

    printf("  ADIN:\n\r");
    printf("    current : %u\n\r", v->ADIN_code);
    printf("    max     : %u\n\r", v->max_ADIN_code);
    printf("    min     : %u\n\r", v->min_ADIN_code);

    printf("  Accumulators:\n\r");
    printf("    energy  : %lu\n\r", (unsigned long)v->energy_code);
    printf("    charge  : %lu\n\r", (unsigned long)v->charge_code);
    printf("    time    : %lu\n\r", (unsigned long)v->time_code);
}

// clang-format on
