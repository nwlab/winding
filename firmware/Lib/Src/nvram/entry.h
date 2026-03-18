/**
 * @file    entry.h
 * @author  Software development team
 * @brief   Low-level NVRAM entry access interface.
 *
 * @details
 * This module provides helper functions for reading and writing
 * individual key–value entries in non-volatile memory (NVRAM).
 *
 * Functions operate on raw offsets within the NVRAM region and
 * are intended for use by higher-level NVRAM management modules.
 */

#ifndef RDNX_NVRAM_ENTRY_H
#define RDNX_NVRAM_ENTRY_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes */
#include <stdint.h>

/* Core includes */
#include <rdnx_nvram.h>
#include "rdnx_nvram_internal.h"

/* ============================================================================
 * Public Function Prototypes
 * ==========================================================================*/

/**
 * @brief Read a single key–value entry from NVRAM.
 *
 * Reads the key and value stored at a given offset within the NVRAM area.
 *
 * @param[in]  ctx     NVRAM context handle
 * @param[in]  offset  Offset in NVRAM where the entry is stored
 * @param[out] key     Pointer to variable that receives the entry key
 * @param[out] value   Pointer to variable that receives the entry value
 *
 * @return
 * - ::RDNX_ERR_OK        Entry successfully read
 * - ::RDNX_ERR_BADARG    One or more arguments are invalid
 * - ::RDNX_ERR_IO        Underlying storage access error
 */
rdnx_err_t rdnx_nvram_read_entry(struct rdnx_nvram_ctx *ctx,
                                 rdnx_nvram_offset_t offset,
                                 rdnx_nvram_key_t *key,
                                 rdnx_nvram_value_t *value);

/**
 * @brief Write a single key–value entry to NVRAM.
 *
 * Stores a key and its associated value at the specified offset in NVRAM.
 *
 * @param[in] ctx     NVRAM context handle
 * @param[in] offset  Offset in NVRAM where the entry will be written
 * @param[in] key     Key to store
 * @param[in] value   Value associated with the key
 *
 * @return
 * - ::RDNX_ERR_OK        Entry successfully written
 * - ::RDNX_ERR_BADARG    One or more arguments are invalid
 * - ::RDNX_ERR_IO        Underlying storage access error
 */
rdnx_err_t rdnx_nvram_write_entry(struct rdnx_nvram_ctx *ctx,
                                  rdnx_nvram_offset_t offset,
                                  rdnx_nvram_key_t key,
                                  rdnx_nvram_value_t value);

/**
 * @brief Check whether a memory block is fully erased.
 *
 * Determines if a memory region contains only erased values (typically 0xFF
 * for flash-based NVRAM).
 *
 * @param[in] data  Pointer to memory block to check
 * @param[in] size  Size of the memory block in bytes
 *
 * @retval 1  Memory block is fully erased
 * @retval 0  Memory block contains programmed data
 */
uint8_t rdnx_nvram_is_block_erased(const uint8_t *data,
                                   rdnx_nvram_offset_t size);

#endif /* RDNX_NVRAM_ENTRY_H */
