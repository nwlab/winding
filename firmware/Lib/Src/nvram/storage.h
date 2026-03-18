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
 * - Blocks are allocated in units of ::RDNX_NVRAM_ENTRY_SIZE.
 */

#ifndef RDNX_NVRAM_STORAGE_H
#define RDNX_NVRAM_STORAGE_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

 /* Core includes */
#include <rdnx_nvram.h>
#include "rdnx_nvram_internal.h"

/* ============================================================================
 * Public Function Prototypes
 * ==========================================================================*/

/**
 * @brief Perform cold boot initialization of NVRAM.
 *
 * Determines the current NVRAM state and executes appropriate
 * initialization or recovery procedures.
 *
 * @see ::rdnx_nvram_cold_boot implementation in rdnx_nvram_state.c
 *
 * @param[in,out] ctx  Pointer to initialized NVRAM context.
 */
void rdnx_nvram_cold_boot(struct rdnx_nvram_ctx *ctx);

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
 * @see ::RDNX_NVRAM_ENTRY_SIZE
 */
rdnx_nvram_offset_t rdnx_nvram_get_next_block(struct rdnx_nvram_ctx *ctx);

#endif /* RDNX_NVRAM_STORAGE_H */
