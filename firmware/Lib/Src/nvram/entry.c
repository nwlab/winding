/**
 * @file    entry.c
 * @author  Software development team
 * @brief   NVRAM entry access and serialization implementation.
 *
 * This module provides low-level access to NVRAM entries and implements
 * serialization/deserialization of key–value pairs stored in fixed-size
 * blocks.
 *
 * Each entry consists of:
 * - rdnx_nvram_key_t
 * - rdnx_nvram_value_t
 *
 * The module relies on the configured ::rdnx_nvram_config_t to
 * perform the actual storage read/write operations.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <string.h>

/* Core includes. */
#include "rdnx_nvram.h"
#include "rdnx_nvram_internal.h"
#include "rdnx_types.h"
#include "entry.h"

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Public Function Implementations
 * ==========================================================================*/

/**
 * @brief Read a key–value entry from NVRAM.
 *
 * This function reads a fixed-size entry from the specified offset,
 * validates that the block is not erased, and deserializes the key
 * and value fields.
 *
 * @param[in]  offset  Offset in bytes within the NVRAM region.
 * @param[out] key     Pointer to variable that will receive the entry key.
 * @param[out] value   Pointer to variable that will receive the entry value.
 *
 * @return
 * - ::RDNX_ERR_OK        Entry successfully read.
 * - ::RDNX_ERR_BADARG    Offset outside valid NVRAM range.
 * - ::RDNX_ERR_NOTFOUND  Entry is erased (not programmed).
 * - Other error codes    Propagated from underlying read operation.
 */
rdnx_err_t rdnx_nvram_read_entry(struct rdnx_nvram_ctx *ctx,
                                 const rdnx_nvram_offset_t offset,
                                 rdnx_nvram_key_t *key,
                                 rdnx_nvram_value_t *value)
{
    NVRAM_DEB("Read entry offset %d", (int)offset);
    /* Boundary check */
    if ((offset + RDNX_NVRAM_ENTRY_SIZE) > ctx->interface.size)
    {
        NVRAM_ERR("Boundary fail");
        return RDNX_ERR_BADARG;
    }

    uint8_t block[RDNX_NVRAM_ENTRY_SIZE] = {
        [0 ... RDNX_NVRAM_ENTRY_SIZE-1] = RDNX_NVRAM_ERASED_BYTE_VALUE
    };

    /* Read raw entry */
    rdnx_err_t err = ctx->interface.read((uint8_t *)&block, offset, RDNX_NVRAM_ENTRY_SIZE);
    if (err != RDNX_ERR_OK)
    {
        NVRAM_ERR("Fail to read entry");
        return err;
    }

    /* Check if block is erased */
    if (rdnx_nvram_is_block_erased(block, RDNX_NVRAM_ENTRY_SIZE))
    {
        NVRAM_ERR("Block is not erased on offset %d", (int)offset);
        return RDNX_ERR_NOTFOUND;
    }

    /* Deserialize key and value */
#if 0
    *key   = *(rdnx_nvram_key_t *)&block[0];
    *value = *(rdnx_nvram_value_t *)&block[sizeof(rdnx_nvram_key_t)];
#else
    memcpy(key,
        &block[0],
        sizeof(rdnx_nvram_key_t));

    memcpy(value,
        &block[sizeof(rdnx_nvram_key_t)],
        sizeof(rdnx_nvram_value_t));
#endif
    return RDNX_ERR_OK;
}

/**
 * @brief Write a key–value entry to NVRAM.
 *
 * This function serializes the given key–value pair into a temporary
 * buffer and writes it to the specified offset.
 *
 * @param[in] offset  Offset in bytes within the NVRAM region.
 * @param[in] key     Entry key to store.
 * @param[in] value   Entry value to store.
 *
 * @return
 * - ::RDNX_ERR_OK     Entry successfully written.
 * - ::RDNX_ERR_BADARG Offset outside valid NVRAM range.
 * - ::RDNX_ERR_FAIL   Underlying write operation failed.
 */
rdnx_err_t rdnx_nvram_write_entry(struct rdnx_nvram_ctx *ctx,
                                  rdnx_nvram_offset_t offset,
                                  rdnx_nvram_key_t key,
                                  rdnx_nvram_value_t value)
{
    NVRAM_DEB("Write entry key %d offset %d", (int)key, (int)offset);
    /* Boundary check */
    if ((offset + RDNX_NVRAM_ENTRY_SIZE) > ctx->interface.size)
    {
        NVRAM_ERR("Boundary fail");
        return RDNX_ERR_BADARG;
    }

    uint8_t block[RDNX_NVRAM_ENTRY_SIZE] = {
        [0 ... RDNX_NVRAM_ENTRY_SIZE-1] = RDNX_NVRAM_ERASED_BYTE_VALUE
    };
#if 0
    /* Serialize key and value */
    rdnx_nvram_key_t *key_in_block = (rdnx_nvram_key_t *)&block[0];

    rdnx_nvram_value_t *value_in_block = (rdnx_nvram_value_t *)&block[sizeof(rdnx_nvram_key_t)];

    *key_in_block   = key;
    *value_in_block = value;
#else
    memcpy(&block[0],
        &key,
        sizeof(rdnx_nvram_key_t));

    memcpy(&block[sizeof(rdnx_nvram_key_t)],
        &value,
        sizeof(rdnx_nvram_value_t));
#endif
    /* Write raw entry */
    rdnx_err_t err = ctx->interface.write((uint8_t *)&block, offset, RDNX_NVRAM_ENTRY_SIZE);
    if (err != RDNX_ERR_OK)
    {
        NVRAM_ERR("Fail to write entry");
        return err;
    }

    return RDNX_ERR_OK;
}

/**
 * @brief Check whether a memory block is fully erased.
 *
 * A block is considered erased if all bytes are equal to
 * ::RDNX_NVRAM_ERASED_BYTE_VALUE (typically 0xFF for flash memory).
 *
 * @param[in] data  Pointer to memory block to verify.
 * @param[in] size  Size of the memory block in bytes.
 *
 * @retval 1  Block is fully erased.
 * @retval 0  Block contains programmed data.
 */
uint8_t rdnx_nvram_is_block_erased(const uint8_t *data,
                                   const rdnx_nvram_offset_t size)
{
    uint8_t erased_bytes = 0;

    for (rdnx_nvram_offset_t i = 0; i < size; i++)
    {
        if (data[i] == RDNX_NVRAM_ERASED_BYTE_VALUE)
        {
            erased_bytes += 1;
        }
    }

    NVRAM_DEB("Not erased bytes %d", erased_bytes);
    return (erased_bytes == size);
}
