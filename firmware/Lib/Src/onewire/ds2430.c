/**
 * @file ds2430.c
 * https://www.analog.com/en/resources/app-notes/1wire-search-algorithm.html
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Core includes. */
#include <hal/rdnx_i2c.h>
#include <onewire/ds2482.h>
#include <onewire/one_wire.h>
#include <rdnx_macro.h>
#include <rdnx_sal.h>
#include <rdnx_types.h>

// clang-format off

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

#define W1_EEPROM_DS2430 0x14

#define W1_F14_EEPROM_SIZE 32
#define W1_F14_PAGE_COUNT  1
#define W1_F14_PAGE_BITS   5
#define W1_F14_PAGE_SIZE   (1 << W1_F14_PAGE_BITS)
#define W1_F14_PAGE_MASK   0x1F

#define W1_F14_SCRATCH_BITS 5
#define W1_F14_SCRATCH_SIZE (1 << W1_F14_SCRATCH_BITS)
#define W1_F14_SCRATCH_MASK (W1_F14_SCRATCH_SIZE - 1)

#define W1_F14_READ_EEPROM      0xF0    // Read memory
#define W1_F14_WRITE_SCRATCH    0x0F    // Write to EPROM or the Scratchpad
#define W1_F14_READ_SCRATCH     0xAA    // Read the status fields [EPROM] or the Scratchpad [EEPROM]
#define W1_F14_COPY_SCRATCH     0x55    // Write to the status fields [EPROM] or commit Scratchpad [EEPROM]
#define W1_F14_VALIDATION_KEY   0xa5    // Either verifies or resumes depending on EEPROM

#define W1_F14_TPROG_MS     11
#define W1_F14_READ_RETRIES 10
#define W1_F14_READ_MAXLEN  W1_F14_SCRATCH_SIZE

// clang-format on

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

rdnx_err_t ds2430_writeMemory(ds2482_handle_t handle, const uint8_t *source, uint8_t length, uint8_t position)
{
    /* Assume we have single device on the bus. */
    rdnx_err_t status = ds2482_1w_send_command_all(handle, W1_F14_WRITE_SCRATCH);
    if (status != RDNX_ERR_OK)
    {
        return status;
    }

    status = ds2482_1w_write_byte(handle, position); // Starting address.
    if (status != RDNX_ERR_OK)
    {
        return status;
    }

    /* Write "length" bytes to the scratchpad */
    for (int i = 0; i < length; i++)
    {
        status = ds2482_1w_write_byte(handle, source[i]);
        if (status != RDNX_ERR_OK)
        {
            return status;
        }
    }

    /* Commit data (DS2430a will transfer data from internal memory to eeprom).
       Assume we have single device on the bus. */
    status = ds2482_1w_send_command_all(handle, W1_F14_COPY_SCRATCH);
    if (status != RDNX_ERR_OK)
    {
        return status;
    }

    // Validation key.
    status = ds2482_1w_write_byte(handle, W1_F14_VALIDATION_KEY);
    if (status != RDNX_ERR_OK)
    {
        return status;
    }

    /* Sleep for tprog ms to wait for the write to complete */
    rdnx_mdelay(W1_F14_TPROG_MS);

    /* Reset the bus to wake up the EEPROM  */
    ow_reset(handle);

    return RDNX_ERR_OK;
}

rdnx_err_t ds2430_readMemory(ds2482_handle_t handle, uint8_t *destination, uint16_t length, uint16_t position)
{
    /* Assume we have single device on the bus. */
    rdnx_err_t status = ds2482_1w_send_command_all(handle, W1_F14_READ_EEPROM);
    if (status != RDNX_ERR_OK)
    {
        return status;
    }

    status = ds2482_1w_write_byte(handle, position); // Starting address.
    if (status != RDNX_ERR_OK)
    {
        return status;
    }

    for (uint8_t i = 0; i < length; i++)
    {
        destination[i] = ow_read_byte(handle);
    }

    return RDNX_ERR_OK;
}
