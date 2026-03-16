/**
 * @file ds2482.c
 * https://www.analog.com/en/resources/app-notes/1wire-search-algorithm.html
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Core includes. */
#include <hal/yaa_i2c.h>
#include <onewire/ds2482.h>
#include <yaa_macro.h>
#include <yaa_sal.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/**
 * The DS2482 registers - there are 3 registers that are addressed by a read
 * pointer. The read pointer is set by the last command executed.
 *
 * To read the data, issue a register read for any address
 */
#define DS2482_CMD_RESET            0xF0 /* No param */
#define DS2482_CMD_SET_READ_PTR     0xE1 /* Param: DS2482_PTR_CODE_xxx */
#define DS2482_CMD_WRITE_CONFIG     0xD2 /* Param: Config byte */
#define DS2482_CMD_1WIRE_RESET      0xB4 /* Param: None */
#define DS2482_CMD_1WIRE_SINGLE_BIT 0x87 /* Param: Bit byte (bit7) */
#define DS2482_CMD_1WIRE_WRITE_BYTE 0xA5 /* Param: Data byte */
#define DS2482_CMD_1WIRE_READ_BYTE  0x96 /* Param: None */
/* Note to read the byte, Set the ReadPtr to Data then read (any addr) */
#define DS2482_CMD_1WIRE_TRIPLET  0x78 /* Param: Dir byte (bit7) */
#define DS2482_CMD_CHANNEL_SELECT 0xC3 /* Param: Channel byte - DS2482-800 only */

/* Values for DS2482_CMD_SET_READ_PTR */
#define DS2482_STATUS    0xF0
#define DS2482_READ_DATA 0xE1
#define DS2482_CONFIG    0xC3
#define DS2482_CHANNEL   0xD2 /* DS2482-800 only */

#define DS2482_CONFIG_APU (1 << 0)
#define DS2482_CONFIG_PPM (1 << 1)
#define DS2482_CONFIG_SPU (1 << 2)
#define DS2482_CONFIG_1WS (1 << 3)

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Struct containing the data for linking an application
 *        to a onewire instance.
 */
typedef struct ds2482_ctx
{
    /**
     * @brief I2C handle
     */
    yaa_i2c_handle_t i2c;

    /**
     * @brief I2C address of the device
     */
    uint8_t address;

    /** @brief Search private */
    int LastDiscrepancy;
    /** @brief Search private */
    int LastFamilyDiscrepancy;
    /** @brief Search private */
    int LastDeviceFlag;

    /** @brief 8-bytes address of last search device */
    unsigned char ROM_NO[8];
    unsigned char crc8;
} ds2482_ctx_t;

typedef struct ds2482_status
{
    uint8_t BUSY : 1;
    uint8_t PPD : 1;
    uint8_t SHORT : 1;
    uint8_t LL : 1;
    uint8_t RST : 1;
    uint8_t SBR : 1;
    uint8_t TSB : 1;
    uint8_t DIR : 1;
} ds2482_status_t;

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

bool ow_first(ds2482_handle_t handle);
bool ow_next(ds2482_handle_t handle);
void ow_target_setup(ds2482_handle_t handle, unsigned char family_code);
void ow_family_skip_setup(ds2482_handle_t handle);
bool ow_reset(ds2482_handle_t handle);
void ow_write_byte(ds2482_handle_t handle, uint8_t byte_value);
void ow_write_bit(ds2482_handle_t handle, uint8_t bit_value);
uint8_t ow_read_byte(ds2482_handle_t handle);
uint8_t ow_read_bit(ds2482_handle_t handle);
bool ow_search(ds2482_handle_t handle);
static unsigned char docrc8(ds2482_handle_t handle, unsigned char value);

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/**
 * It performs a device reset followed by writing the configuration byte
 * to default values:
 *   1-Wire speed (c1WS) = standard (0)
 *   Strong pullup (cSPU) = off (0)
 *   Presence pulse masking (cPPM) = off (0)
 *   Active pullup (cAPU) = on (CONFIG_APU = 0x01)
 */
yaa_err_t ds2482_init(const ds2482_params_t *param, ds2482_handle_t *handle)
{
    yaa_err_t status;

    if (param == NULL || param->i2c == NULL)
    {
        return YAA_ERR_BADARG;
    }

    ds2482_ctx_t *ctx = (ds2482_ctx_t *)yaa_alloc(sizeof(ds2482_ctx_t));
    if (ctx == NULL)
    {
        return YAA_ERR_NOMEM;
    }

    ctx->address = param->address;
    ctx->i2c = param->i2c;

    yaa_i2c_set_timeout(ctx->i2c, DS2482_TIMEOUT_I2C);

    status = ds2482_reset(ctx);
    if (status != YAA_ERR_OK)
    {
        yaa_free(ctx);
        return status;
    }

    status = ds2482_write_config(ctx, DS2482_CONFIG_APU);
    if (status != YAA_ERR_OK)
    {
        yaa_free(ctx);
        return status;
    }

    uint8_t config = 0;
    status = yaa_i2c_read(ctx->i2c, ctx->address, 0, YAA_I2C_REGISTER_NONE, &config, 1, true);
    if (status != YAA_ERR_OK)
    {
        yaa_free(ctx);
        return status;
    }
    else if (config != DS2482_CONFIG_APU)
    {
        yaa_free(ctx);
        return YAA_ERR_FAIL;
    }

    *handle = ctx;

    return YAA_ERR_OK;
}

yaa_err_t ds2482_destroy(ds2482_handle_t handle)
{
    ds2482_ctx_t *ctx = handle;

    if (ctx != NULL)
    {
        yaa_free(ctx);
    }

    return YAA_ERR_OK;
}

yaa_err_t ds2482_reset(ds2482_handle_t handle)
{
    uint8_t data[1] = { DS2482_CMD_RESET };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, data, sizeof(data), true);
    return status;
}

yaa_err_t ds2482_write_config(ds2482_handle_t handle, uint8_t config)
{
    uint8_t data[2] = { DS2482_CMD_WRITE_CONFIG, config | (~config << 4) };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, data, sizeof(data), true);
    return status;
}

yaa_err_t ds2482_set_read_ptr(ds2482_handle_t handle, uint8_t read_ptr)
{
    uint8_t data[2] = { DS2482_CMD_SET_READ_PTR, read_ptr };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, data, sizeof(data), true);
    return status;
}

yaa_err_t ds2482_1w_reset(ds2482_handle_t handle, bool *const presence)
{
    uint8_t data[1] = { DS2482_CMD_1WIRE_RESET };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)data, sizeof(data), true);
    if (status != YAA_ERR_OK)
    {
        return status;
    }
    ds2482_status_t status_reg = { .BUSY = 1 };
    uint32_t timeout = yaa_systemtime();
    do
    {
        status =
            yaa_i2c_read(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)&status_reg, 1, true);
        if (status != YAA_ERR_OK)
        {
            return status;
        }
        if (yaa_istimespent(timeout, DS2482_TIMEOUT_1W))
        {
            return YAA_ERR_TIMEOUT;
        }
    } while (status_reg.BUSY);

    if (status_reg.SHORT)
    {
        return YAA_ERR_IO;
    }

    if (presence != NULL)
    {
        *presence = status_reg.PPD;
    }

    return YAA_ERR_OK;
}

yaa_err_t ds2482_1w_write_byte(ds2482_handle_t handle, uint8_t byte)
{
    uint8_t data[2] = { DS2482_CMD_1WIRE_WRITE_BYTE, byte };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)data, sizeof(data), true);
    if (status != YAA_ERR_OK)
    {
        return status;
    }

    ds2482_status_t status_reg = { .BUSY = 1 };
    uint32_t timeout = yaa_systemtime();
    do
    {
        status =
            yaa_i2c_read(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)&status_reg, 1, true);
        if (status != YAA_ERR_OK)
        {
            return status;
        }
        if (yaa_istimespent(timeout, DS2482_TIMEOUT_1W))
        {
            return YAA_ERR_TIMEOUT;
        }
    } while (status_reg.BUSY);

    return YAA_ERR_OK;
}

yaa_err_t ds2482_1w_read_byte(ds2482_handle_t handle, uint8_t *const byte)
{
    uint8_t data[1] = { DS2482_CMD_1WIRE_READ_BYTE };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)data, sizeof(data), true);
    if (status != YAA_ERR_OK)
    {
        return status;
    }

    ds2482_status_t status_reg = { .BUSY = 1 };
    uint32_t timeout = yaa_systemtime();
    do
    {
        status =
            yaa_i2c_read(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)&status_reg, 1, true);
        if (status != YAA_ERR_OK)
        {
            return status;
        }
        if (yaa_istimespent(timeout, DS2482_TIMEOUT_1W))
        {
            return YAA_ERR_TIMEOUT;
        }
    } while (status_reg.BUSY);

    status = ds2482_set_read_ptr(handle, DS2482_READ_DATA);
    if (status != YAA_ERR_OK)
    {
        return status;
    }

    return yaa_i2c_read(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)byte, 1, true);
}

yaa_err_t ds2482_1w_read_bit(ds2482_handle_t handle, bool *const bit)
{
    uint8_t data[2] = { DS2482_CMD_1WIRE_SINGLE_BIT, 0xFF };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)data, sizeof(data), true);
    if (status != YAA_ERR_OK)
    {
        return status;
    }

    ds2482_status_t status_reg = { .BUSY = 1 };
    uint32_t timeout = yaa_systemtime();
    do
    {
        status =
            yaa_i2c_read(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)&status_reg, 1, true);
        if (status != YAA_ERR_OK)
        {
            return status;
        }
        if (yaa_istimespent(timeout, DS2482_TIMEOUT_1W))
        {
            return YAA_ERR_TIMEOUT;
        }
    } while (status_reg.BUSY);

    *bit = status_reg.SBR;

    return YAA_ERR_OK;
}

yaa_err_t ds2482_1w_write_bit(ds2482_handle_t handle, bool bit)
{
    uint8_t data[2] = { DS2482_CMD_1WIRE_SINGLE_BIT, bit ? 0xFF : 0x00 };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)data, sizeof(data), true);
    if (status != YAA_ERR_OK)
    {
        return status;
    }

    ds2482_status_t status_reg = { .BUSY = 1 };
    uint32_t timeout = yaa_systemtime();
    do
    {
        status =
            yaa_i2c_read(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, (uint8_t *)&status_reg, 1, true);
        if (status != YAA_ERR_OK)
        {
            return status;
        }
        if (yaa_istimespent(timeout, DS2482_TIMEOUT_1W))
        {
            return YAA_ERR_TIMEOUT;
        }
    } while (status_reg.BUSY);

    return YAA_ERR_OK;
}

yaa_err_t ds2482_1w_triplet(ds2482_handle_t handle, uint8_t dir)
{
    uint8_t data[2] = { DS2482_CMD_1WIRE_TRIPLET, dir ? 0xFF : 0x00 };
    yaa_err_t status =
        yaa_i2c_write(handle->i2c, handle->address, 0, YAA_I2C_REGISTER_NONE, data, sizeof(data), true);
    return status;
}

yaa_err_t ds2482_1w_search(ds2482_handle_t handle, uint16_t max_devices, uint64_t devices[static max_devices])
{
    uint16_t count = 0;

    ow_first(handle);
    do
    {
        uint64_t device = 0;
        for (int i = 0; i < 8; i++)
        {
            device |= (uint64_t)handle->ROM_NO[i] << (i * 8);
        }
        devices[count++] = device;

    } while (ow_next(handle) && count < max_devices);

    return YAA_ERR_OK;
}

/* Verify the device with the ROM number in ROM_NO buffer is present. */
yaa_err_t ds2482_1w_verify_device(ds2482_handle_t handle, uint64_t device, bool *const present)
{
    YAA_UNUSED(present);
    uint8_t rom_backup[8];
    int i;
    yaa_err_t rslt;

    for (int i = 0; i < 8; i++)
    {
        handle->ROM_NO[i] = (device >> (i * 8)) & 0xFF;
        rom_backup[i] = handle->ROM_NO[i];
    }

    /* Set search to find the same device */
    handle->LastDiscrepancy = 64;
    handle->LastDeviceFlag = false;

    if (ow_search(handle))
    {
        // check if same device found
        rslt = YAA_ERR_OK;
        for (i = 0; i < 8; i++)
        {
            if (rom_backup[i] != handle->ROM_NO[i])
            {
                rslt = YAA_ERR_NOTFOUND;
                break;
            }
        }
    }
    else
        rslt = YAA_ERR_NOTFOUND;

    /* Return the result of the verify */
    return rslt;
}

/*
 * Setup the search to find the device type 'family_code' on the next call
 * to ow_next() if it is present.
 */
[[maybe_unused]] void ow_target_setup(ds2482_handle_t handle, unsigned char family_code)
{
    int i;

    // set the search state to find SearchFamily type devices
    handle->ROM_NO[0] = family_code;
    for (i = 1; i < 8; i++)
    {
        handle->ROM_NO[i] = 0;
    }
    handle->LastDiscrepancy = 64;
    handle->LastFamilyDiscrepancy = 0;
    handle->LastDeviceFlag = false;
}

/*
 * Setup the search to skip the current device type on the next call
 * to ow_next().
 */
[[maybe_unused]] void ow_family_skip_setup(ds2482_handle_t handle)
{
    /* Set the Last discrepancy to last family discrepancy */
    handle->LastDiscrepancy = handle->LastFamilyDiscrepancy;
    handle->LastFamilyDiscrepancy = 0;

    /* Check for end of list */
    if (handle->LastDiscrepancy == 0)
    {
        handle->LastDeviceFlag = true;
    }
}

/* ============================================================================
 * 1-Wire Functions to be implemented for a particular platform
 * ==========================================================================*/

/*
 * Reset the 1-Wire bus and return the presence of any device
 * Return true  : device present
 *        false : no device present
 */
bool ow_reset(ds2482_handle_t handle)
{
    bool presence = false;
    (void)ds2482_1w_reset(handle, &presence);
    return presence;
}

/*
 * Send 8 bits of data to the 1-Wire bus
 */
void ow_write_byte(ds2482_handle_t handle, uint8_t byte_value)
{
    (void)ds2482_1w_write_byte(handle, byte_value);
}

/*
 * Send 1 bit of data to teh 1-Wire bus
 */
void ow_write_bit(ds2482_handle_t handle, uint8_t bit_value)
{
    (void)ds2482_1w_write_bit(handle, bit_value);
}

/*
 * Read 1-Wire data byte and return it
 */
uint8_t ow_read_byte(ds2482_handle_t handle)
{
    uint8_t loop, result = 0;

    for (loop = 0; loop < 8; loop++)
    {
        // shift the result to get it ready for the next bit
        result >>= 1;

        // if result is one, then set MS bit
        if (ow_read_bit(handle))
            result |= 0x80;
    }
    return result;
}

/*
 *  Read 1 bit of data from the 1-Wire bus
 *  Return 1 : bit read is 1
 *         0 : bit read is 0
 */
uint8_t ow_read_bit(ds2482_handle_t handle)
{
    bool bit = 0;
    (void)ds2482_1w_read_bit(handle, &bit);
    return bit;
}

/*
 * Find the 'first' devices on the 1-Wire bus
 * Return true  : device found, ROM number in ROM_NO buffer
 *        false : no device present
 */
bool ow_first(ds2482_handle_t handle)
{
    /* Reset the search state */
    handle->LastDiscrepancy = 0;
    handle->LastDeviceFlag = false;
    handle->LastFamilyDiscrepancy = 0;

    return ow_search(handle);
}

/*
 * Find the 'next' devices on the 1-Wire bus
 * Return true  : device found, ROM number in ROM_NO buffer
 *        false : device not found, end of search
 */
bool ow_next(ds2482_handle_t handle)
{
    return ow_search(handle);
}

/* Perform the 1-Wire Search Algorithm on the 1-Wire bus using the existing
 * search state.
 * Return true  : device found, ROM number in ROM_NO buffer
 *        false : device not found, end of search
 */
bool ow_search(ds2482_handle_t handle)
{
    int id_bit_number;
    int last_zero, rom_byte_number, search_result;
    int id_bit, cmp_id_bit;
    uint8_t rom_byte_mask, search_direction;

    /* Initialize for search */
    id_bit_number = 1;
    last_zero = 0;
    rom_byte_number = 0;
    rom_byte_mask = 1;
    search_result = 0;
    handle->crc8 = 0;

    /* If the last call was not the last one */
    if (!handle->LastDeviceFlag)
    {
        // 1-Wire reset
        if (!ow_reset(handle))
        {
            // reset the search
            handle->LastDiscrepancy = 0;
            handle->LastDeviceFlag = false;
            handle->LastFamilyDiscrepancy = 0;
            return false;
        }

        /* Issue the search command */
        ow_write_byte(handle, DS2482_1W_ROM_SEARCH);

        /* Loop to do the search */
        do
        {
            /* Read a bit and its complement */
            id_bit = ow_read_bit(handle);
            cmp_id_bit = ow_read_bit(handle);

            /* Check for no devices on 1-wire */
            if ((id_bit == 1) && (cmp_id_bit == 1))
                break;
            else
            {
                /* All devices coupled have 0 or 1 */
                if (id_bit != cmp_id_bit)
                    search_direction = id_bit; // bit write value for search
                else
                {
                    /* If this discrepancy if before the Last Discrepancy
                       on a previous next then pick the same as last time */
                    if (id_bit_number < handle->LastDiscrepancy)
                        search_direction = ((handle->ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
                    else
                        /* If equal to last pick 1, if not then pick 0 */
                        search_direction = (id_bit_number == handle->LastDiscrepancy);

                    /* If 0 was picked then record its position in LastZero */
                    if (search_direction == 0)
                    {
                        last_zero = id_bit_number;

                        /* Check for Last discrepancy in family */
                        if (last_zero < 9)
                            handle->LastFamilyDiscrepancy = last_zero;
                    }
                }

                /* Set or clear the bit in the ROM byte rom_byte_number
                   with mask rom_byte_mask */
                if (search_direction == 1)
                    handle->ROM_NO[rom_byte_number] |= rom_byte_mask;
                else
                    handle->ROM_NO[rom_byte_number] &= ~rom_byte_mask;

                /* Serial number search direction write bit */
                ow_write_bit(handle, search_direction);

                /* Increment the byte counter id_bit_number
                   and shift the mask rom_byte_mask */
                id_bit_number++;
                rom_byte_mask <<= 1;

                /* If the mask is 0 then go to new SerialNum byte
                 * rom_byte_number and reset mask */
                if (rom_byte_mask == 0)
                {
                    docrc8(handle,
                           handle->ROM_NO[rom_byte_number]); // accumulate the CRC
                    rom_byte_number++;
                    rom_byte_mask = 1;
                }
            }
        } while (rom_byte_number < 8); // loop until through all ROM bytes 0-7

        /* If the search was successful then */
        if (!((id_bit_number < 65) || (handle->crc8 != 0)))
        {
            /* Search successful so set
             * LastDiscrepancy,LastDeviceFlag,search_result */
            handle->LastDiscrepancy = last_zero;

            /* Check for last device */
            if (handle->LastDiscrepancy == 0)
                handle->LastDeviceFlag = true;

            search_result = true;
        }
    }

    /* If no device found then reset counters so next 'search' will be like a
     * first */
    if (!search_result || !handle->ROM_NO[0])
    {
        handle->LastDiscrepancy = 0;
        handle->LastDeviceFlag = false;
        handle->LastFamilyDiscrepancy = 0;
        search_result = false;
    }

    return search_result;
}

// clang-format off
static const unsigned char dscrc_table[] = {
        0, 94,188,226, 97, 63,221,131,194,156,126, 32,163,253, 31, 65,
      157,195, 33,127,252,162, 64, 30, 95,  1,227,189, 62, 96,130,220,
       35,125,159,193, 66, 28,254,160,225,191, 93,  3,128,222, 60, 98,
      190,224,  2, 92,223,129, 99, 61,124, 34,192,158, 29, 67,161,255,
       70, 24,250,164, 39,121,155,197,132,218, 56,102,229,187, 89,  7,
      219,133,103, 57,186,228,  6, 88, 25, 71,165,251,120, 38,196,154,
      101, 59,217,135,  4, 90,184,230,167,249, 27, 69,198,152,122, 36,
      248,166, 68, 26,153,199, 37,123, 58,100,134,216, 91,  5,231,185,
      140,210, 48,110,237,179, 81, 15, 78, 16,242,172, 47,113,147,205,
       17, 79,173,243,112, 46,204,146,211,141,111, 49,178,236, 14, 80,
      175,241, 19, 77,206,144,114, 44,109, 51,209,143, 12, 82,176,238,
       50,108,142,208, 83, 13,239,177,240,174, 76, 18,145,207, 45,115,
      202,148,118, 40,171,245, 23, 73,  8, 86,180,234,105, 55,213,139,
       87,  9,235,181, 54,104,138,212,149,203, 41,119,244,170, 72, 22,
      233,183, 85, 11,136,214, 52,106, 43,117,151,201, 74, 20,246,168,
      116, 42,200,150, 21, 75,169,247,182,232, 10, 84,215,137,107, 53
};

// clang-format on

/*
 * Calculate the CRC8 of the byte value provided with the current
 * global 'crc8' value.
 * Returns current global crc8 value
 */
static unsigned char docrc8(ds2482_handle_t handle, unsigned char value)
{
    // See Application Note 27
    handle->crc8 = dscrc_table[handle->crc8 ^ value];
    return handle->crc8;
}
