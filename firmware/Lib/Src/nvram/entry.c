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
 * - yaa_nvram_key_t
 * - yaa_nvram_value_t
 *
 * The module relies on the configured ::yaa_nvram_config_t to
 * perform the actual storage read/write operations.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <string.h>

/* Core includes. */
#include "yaa_nvram.h"
#include "yaa_nvram_internal.h"
#include "yaa_types.h"
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
 * - ::YAA_ERR_OK        Entry successfully read.
 * - ::YAA_ERR_BADARG    Offset outside valid NVRAM range.
 * - ::YAA_ERR_NOTFOUND  Entry is erased (not programmed).
 * - Other error codes    Propagated from underlying read operation.
 */
yaa_err_t yaa_nvram_read_entry(struct yaa_nvram_ctx *ctx,
                                 const yaa_nvram_offset_t offset,
                                 yaa_nvram_key_t *key,
                                 yaa_nvram_value_t *value)
{
    NVRAM_DEB("Read entry offset %d", (int)offset);
    /* Boundary check */
    if ((offset + YAA_NVRAM_ENTRY_SIZE) > ctx->interface.size)
    {
        NVRAM_ERR("Boundary fail");
        return YAA_ERR_BADARG;
    }

    uint8_t block[YAA_NVRAM_ENTRY_SIZE] = {
        [0 ... YAA_NVRAM_ENTRY_SIZE-1] = YAA_NVRAM_ERASED_BYTE_VALUE
    };

    /* Read raw entry */
    yaa_err_t err = ctx->interface.read((uint8_t *)&block, offset, YAA_NVRAM_ENTRY_SIZE);
    if (err != YAA_ERR_OK)
    {
        NVRAM_ERR("Fail to read entry");
        return err;
    }

    /* Check if block is erased */
    if (yaa_nvram_is_block_erased(block, YAA_NVRAM_ENTRY_SIZE))
    {
        NVRAM_ERR("Block is not erased on offset %d", (int)offset);
        return YAA_ERR_NOTFOUND;
    }

    /* Deserialize key and value */
#if 0
    *key   = *(yaa_nvram_key_t *)&block[0];
    *value = *(yaa_nvram_value_t *)&block[sizeof(yaa_nvram_key_t)];
#else
    memcpy(key,
        &block[0],
        sizeof(yaa_nvram_key_t));

    memcpy(value,
        &block[sizeof(yaa_nvram_key_t)],
        sizeof(yaa_nvram_value_t));
#endif
    return YAA_ERR_OK;
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
 * - ::YAA_ERR_OK     Entry successfully written.
 * - ::YAA_ERR_BADARG Offset outside valid NVRAM range.
 * - ::YAA_ERR_FAIL   Underlying write operation failed.
 */
yaa_err_t yaa_nvram_write_entry(struct yaa_nvram_ctx *ctx,
                                  yaa_nvram_offset_t offset,
                                  yaa_nvram_key_t key,
                                  yaa_nvram_value_t value)
{
    NVRAM_DEB("Write entry key %d offset %d", (int)key, (int)offset);
    /* Boundary check */
    if ((offset + YAA_NVRAM_ENTRY_SIZE) > ctx->interface.size)
    {
        NVRAM_ERR("Boundary fail");
        return YAA_ERR_BADARG;
    }

    uint8_t block[YAA_NVRAM_ENTRY_SIZE] = {
        [0 ... YAA_NVRAM_ENTRY_SIZE-1] = YAA_NVRAM_ERASED_BYTE_VALUE
    };
#if 0
    /* Serialize key and value */
    yaa_nvram_key_t *key_in_block = (yaa_nvram_key_t *)&block[0];

    yaa_nvram_value_t *value_in_block = (yaa_nvram_value_t *)&block[sizeof(yaa_nvram_key_t)];

    *key_in_block   = key;
    *value_in_block = value;
#else
    memcpy(&block[0],
        &key,
        sizeof(yaa_nvram_key_t));

    memcpy(&block[sizeof(yaa_nvram_key_t)],
        &value,
        sizeof(yaa_nvram_value_t));
#endif
    /* Write raw entry */
    yaa_err_t err = ctx->interface.write((uint8_t *)&block, offset, YAA_NVRAM_ENTRY_SIZE);
    if (err != YAA_ERR_OK)
    {
        NVRAM_ERR("Fail to write entry");
        return err;
    }

    return YAA_ERR_OK;
}

/**
 * @brief Check whether a memory block is fully erased.
 *
 * A block is considered erased if all bytes are equal to
 * ::YAA_NVRAM_ERASED_BYTE_VALUE (typically 0xFF for flash memory).
 *
 * @param[in] data  Pointer to memory block to verify.
 * @param[in] size  Size of the memory block in bytes.
 *
 * @retval 1  Block is fully erased.
 * @retval 0  Block contains programmed data.
 */
uint8_t yaa_nvram_is_block_erased(const uint8_t *data,
                                   const yaa_nvram_offset_t size)
{
    uint8_t erased_bytes = 0;

    for (yaa_nvram_offset_t i = 0; i < size; i++)
    {
        if (data[i] == YAA_NVRAM_ERASED_BYTE_VALUE)
        {
            erased_bytes += 1;
        }
    }

    NVRAM_DEB("Not erased bytes %d", erased_bytes);
    return (erased_bytes == size);
}
