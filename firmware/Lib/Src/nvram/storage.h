/**
 * @file    storage.h
 * @author  Software development team
 * @brief   NVRAM storage and block management interface.
 *
 * This module provides functions to initialize NVRAM on cold boot
 * and to allocate the next free block in the main area.
 *
 * @note
 * - Cold boot initializes the NVRAM state, performing recovery
 *   or defragmentation if required.
 * - Blocks are allocated in units of ::YAA_NVRAM_ENTRY_SIZE.
 */

#ifndef YAA_NVRAM_STORAGE_H
#define YAA_NVRAM_STORAGE_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

 /* Core includes */
#include <yaa_nvram.h>
#include "yaa_nvram_internal.h"

/* ============================================================================
 * Public Function Prototypes
 * ==========================================================================*/

/**
 * @brief Perform cold boot initialization of NVRAM.
 *
 * Determines the current NVRAM state and executes appropriate
 * initialization or recovery procedures.
 *
 * @see ::yaa_nvram_cold_boot implementation in yaa_nvram_state.c
 *
 * @param[in,out] ctx  Pointer to initialized NVRAM context.
 */
void yaa_nvram_cold_boot(struct yaa_nvram_ctx *ctx);

/**
 * @brief Reserve the next free block in main NVRAM area.
 *
 * Allocates space for a new entry. If all main NVRAM is used,
 * a defragmentation and backup procedure is triggered automatically.
 *
 * @param[in,out] ctx  Pointer to initialized NVRAM context.
 *
 * @return Offset of the first byte of the new block.
 *
 * @see ::YAA_NVRAM_ENTRY_SIZE
 */
yaa_nvram_offset_t yaa_nvram_get_next_block(struct yaa_nvram_ctx *ctx);

#endif /* YAA_NVRAM_STORAGE_H */
