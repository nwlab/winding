/**
 * @file    map.h
 * @author  Software development team
 * @brief   RAM-based NVRAM entry cache interface.
 *
 * @details
 * This module provides a lightweight RAM cache of NVRAM entries,
 * allowing fast lookup of entry offsets by key and management of
 * cached entries.
 *
 * The cache is implemented as a simple array of ::rdnx_nvram_entry_t.
 *
 * @note
 * - Lookup is linear (O(n)), so performance may degrade for large numbers
 *   of entries (::RDNX_NVRAM_MAX_ENTRIES).
 * - All entries reside in RAM. Adjust ::RDNX_NVRAM_MAX_ENTRIES or the
 *   types ::rdnx_nvram_key_t and ::rdnx_nvram_offset_t to reduce memory usage.
 */

#ifndef RDNX_NVRAM_MAP_H
#define RDNX_NVRAM_MAP_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes */
#include <stdint.h>
#include <stddef.h>

/* Core includes */
#include <rdnx_nvram.h>
#include "rdnx_nvram_internal.h"

/* ============================================================================
 * Public Function Prototypes
 * ==========================================================================*/

/**
 * @brief Retrieve a cached entry by key.
 *
 * Searches the RAM cache for an entry with the specified key.
 *
 * @param[in]  ctx    NVRAM context handle
 * @param[in]  key    Key to search for
 * @param[out] entry  Pointer to receive address of matching entry on success
 *
 * @return
 * - ::RDNX_ERR_OK        Entry found
 * - ::RDNX_ERR_NOTFOUND  Entry does not exist in cache
 */
rdnx_err_t rdnx_nvram_get_entry(struct rdnx_nvram_ctx *ctx,
                                const rdnx_nvram_key_t key,
                                rdnx_nvram_entry_t **entry);

/**
 * @brief Retrieve a cached entry by index.
 *
 * @param[in] ctx     NVRAM context handle
 * @param[in] number  Entry index (0-based)
 *
 * @return
 * - Pointer to ::rdnx_nvram_entry_t if index is valid
 * - NULL if index is out of range
 */
rdnx_nvram_entry_t *rdnx_nvram_get_entry_by_id(struct rdnx_nvram_ctx *ctx,
                                               const rdnx_nvram_key_t number);

/**
 * @brief Allocate a new entry in the RAM cache.
 *
 * @warning Caller must ensure there is available space by checking
 *          ::rdnx_nvram_map_free_entries() before calling.
 *
 * @param[in] ctx  NVRAM context handle
 *
 * @return Pointer to newly allocated ::rdnx_nvram_entry_t, or NULL if no free slot
 */
rdnx_nvram_entry_t *rdnx_nvram_create_entry(struct rdnx_nvram_ctx *ctx);

/**
 * @brief Update an existing cache entry or create a new one.
 *
 * Sets or updates the offset associated with a key. If the key does
 * not exist, a new entry is created.
 *
 * @param[in] ctx     NVRAM context handle
 * @param[in] key     Key of the entry
 * @param[in] offset  Logical offset in NVRAM (bytes)
 *
 * @return
 * - ::RDNX_ERR_OK     Entry updated or created successfully
 * - ::RDNX_ERR_NOMEM  No free cache entries available
 */
rdnx_err_t rdnx_nvram_update_entry(struct rdnx_nvram_ctx *ctx,
                                   const rdnx_nvram_key_t key,
                                   const rdnx_nvram_offset_t offset);

/**
 * @brief Reset the NVRAM cache.
 *
 * Clears all stored entries. Previously cached mappings become invalid.
 *
 * @param[in] ctx  NVRAM context handle
 */
void rdnx_nvram_reset_map(struct rdnx_nvram_ctx *ctx);

/**
 * @brief Get the number of used entries in the cache.
 *
 * @param[in] ctx  NVRAM context handle
 *
 * @return Number of currently stored unique keys
 */
size_t rdnx_nvram_get_used_entries(const struct rdnx_nvram_ctx *ctx);

/**
 * @brief Get the number of free entry slots in the cache.
 *
 * @param[in] ctx  NVRAM context handle
 *
 * @return Remaining available slots for unique keys
 */
size_t rdnx_nvram_map_free_entries(const struct rdnx_nvram_ctx *ctx);

#endif /* RDNX_NVRAM_MAP_H */
