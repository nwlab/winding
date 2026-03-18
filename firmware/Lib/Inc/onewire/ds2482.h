/**
 * @file    ds2482.h
 * @author  Software development team
 * @brief   Driver for DS2482 device, I2C-to-onewire bridge IC.
 * @version 1.0
 * @date    2024-10-29
 */

#ifndef DS2482_H
#define DS2482_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Core includes. */
#include <hal/rdnx_i2c.h>
#include <rdnx_macro.h>
#include <rdnx_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

#define DS248_DEFAULT_ADDRESS (0x18 << 1)

#ifndef DS2482_TIMEOUT_1W
#define DS2482_TIMEOUT_1W 100 /* timeout in ms for 1-wire operations */
#endif
#ifndef DS2482_TIMEOUT_I2C
#define DS2482_TIMEOUT_I2C 100 /* timeout in ms for I2C operations */
#endif

/* "DS2482_TIMEOUT_1W must be less than 65535" */
RDNX_STATIC_ASSERT(DS2482_TIMEOUT_1W < UINT16_MAX);
/* "DS2482_TIMEOUT_I2C must be less than 4294967295" */
RDNX_STATIC_ASSERT(DS2482_TIMEOUT_I2C < UINT32_MAX);
/* "DS2482_TIMEOUT_1W cannot be negative" */
RDNX_STATIC_ASSERT(DS2482_TIMEOUT_1W > 0);
/* "DS2482_TIMEOUT_I2C cannot be negative" */
RDNX_STATIC_ASSERT(DS2482_TIMEOUT_I2C > 0);

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

typedef enum
{
    DS2482_1W_ROM_MATCH = 0x55,
    DS2482_1W_ROM_SKIP = 0xCC,
    DS2482_1W_ROM_SEARCH = 0xF0
} ds2482_wire_cmd_t;

/**
 * @brief Parameters for the DS2482 device
 */
typedef struct ds2482_params
{
    /**
     * @brief I2C handle
     */
    rdnx_i2c_handle_t i2c;

    /**
     * @brief I2C address of the device
     */
    uint8_t address;
} ds2482_params_t;

/**
 * @brief Handle to an initialized DS2482 device
 */
typedef struct ds2482_ctx *ds2482_handle_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

rdnx_err_t ds2482_init(const ds2482_params_t *param, ds2482_handle_t *handle);
rdnx_err_t ds2482_destroy(ds2482_handle_t handle);
rdnx_err_t ds2482_reset(ds2482_handle_t handle);
rdnx_err_t ds2482_write_config(ds2482_handle_t handle, uint8_t config);
rdnx_err_t ds2482_set_read_ptr(ds2482_handle_t handle, uint8_t read_ptr);
rdnx_err_t ds2482_1w_reset(ds2482_handle_t handle, bool *const presence);
rdnx_err_t ds2482_1w_write_byte(ds2482_handle_t handle, uint8_t byte);
rdnx_err_t ds2482_1w_read_byte(ds2482_handle_t handle, uint8_t *const byte);
rdnx_err_t ds2482_1w_read_bit(ds2482_handle_t handle, bool *const bit);
rdnx_err_t ds2482_1w_write_bit(ds2482_handle_t handle, bool bit);
rdnx_err_t ds2482_1w_triplet(ds2482_handle_t handle, uint8_t dir);

rdnx_err_t ds2482_1w_search(ds2482_handle_t handle, uint16_t max_devices, uint64_t devices[static max_devices]);
rdnx_err_t ds2482_1w_verify_device(ds2482_handle_t handle, uint64_t device, bool *const present);

/* ============================================================================
 * Helper functions
 * ==========================================================================*/

RDNX_STATIC_INLINE rdnx_err_t ds2482_1w_select_device(ds2482_handle_t handle, uint64_t device)
{
    bool present;
    rdnx_err_t status = ds2482_1w_reset(handle, &present);
    if (status != RDNX_ERR_OK)
        return status;
    if (!present)
        return RDNX_ERR_IO;
    status = ds2482_1w_write_byte(handle, DS2482_1W_ROM_MATCH);
    if (status != RDNX_ERR_OK)
        return status;
    for (int i = 0; i < 8; i++)
    {
        status = ds2482_1w_write_byte(handle, (device >> (i * 8)) & 0xFF);
        if (status != RDNX_ERR_OK)
            return status;
    }
    return RDNX_ERR_OK;
}

RDNX_STATIC_INLINE rdnx_err_t ds2482_1w_select_all(ds2482_handle_t handle)
{
    bool present;
    rdnx_err_t status = ds2482_1w_reset(handle, &present);
    if (status != RDNX_ERR_OK)
        return status;
    if (!present)
        return RDNX_ERR_IO;
    return ds2482_1w_write_byte(handle, DS2482_1W_ROM_SKIP);
}

RDNX_STATIC_INLINE rdnx_err_t ds2482_1w_send_command(ds2482_handle_t handle, uint64_t device, uint8_t command)
{
    rdnx_err_t status = ds2482_1w_select_device(handle, device);
    if (status != RDNX_ERR_OK)
        return status;
    return ds2482_1w_write_byte(handle, command);
}

RDNX_STATIC_INLINE rdnx_err_t ds2482_1w_send_command_all(ds2482_handle_t handle, uint8_t command)
{
    rdnx_err_t status = ds2482_1w_select_all(handle);
    if (status != RDNX_ERR_OK)
        return status;
    return ds2482_1w_write_byte(handle, command);
}

#ifdef __cplusplus
}
#endif

#endif // DS2482_H
