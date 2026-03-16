/**
 * @file    map.c
 * @author  Software development team
 * @brief   In-RAM cache for NVRAM entry mapping.
 *
 * This module implements a simple key-to-offset cache used to track
 * logical positions of entries stored in NVRAM.
 *
 * The cache is currently implemented as a linear array of
 * ::yaa_nvram_entry_t structures.
 *
 * @note
 * - Lookup complexity is O(n) due to linear search.
 * - Performance may degrade as the number of unique keys approaches
 *   ::YAA_NVRAM_MAX_ENTRIES.
 * - The cache resides entirely in RAM.
 *
 * To reduce RAM usage:
 * - Decrease ::YAA_NVRAM_MAX_ENTRIES.
 * - Adjust types ::yaa_nvram_key_t and ::yaa_nvram_offset_t.
 * - Consider packing ::yaa_nvram_entry_t if alignment allows.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h>

/* Core includes. */
#include "yaa_types.h"
#include "yaa_macro.h"
#include "map.h"
#include "entry.h"
#include "yaa_nvram.h"
#include "yaa_nvram_internal.h"

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

/* ============================================================================
 * Public Function Implementations
 * ==========================================================================*/

/**
 * @brief Retrieve entry by key.
 *
 * Performs a linear search in the internal cache for the specified key.
 *
 * @param[in]  key    Key to search for.
 * @param[out] entry  Pointer to pointer that will receive the address
 *                    of the matching entry on success.
 *
 * @return
 * - ::YAA_ERR_OK        Entry found.
 * - ::YAA_ERR_NOTFOUND  No entry with the specified key exists.
 */
yaa_err_t yaa_nvram_get_entry(struct yaa_nvram_ctx *ctx,
                                const yaa_nvram_key_t key,
                                yaa_nvram_entry_t **entry)
{
    for (yaa_nvram_key_t i = 0; i < ctx->used_entries; i++)
    {
        YAA_ASSERT(i < YAA_NVRAM_MAX_ENTRIES);
        *entry = &ctx->yaa_nvram_entries[i];

        if (key == ctx->yaa_nvram_entries[i].key)
        {
            return YAA_ERR_OK;
        }
    }

    return YAA_ERR_NOTFOUND;
}

/**
 * @brief Retrieve entry by index.
 *
 * Returns a pointer to an entry based on its index within the cache.
 *
 * @param[in] number  Entry index (0-based).
 *
 * @return
 * - Pointer to ::yaa_nvram_entry_t if index is valid.
 * - NULL if index is out of range.
 */
yaa_nvram_entry_t *yaa_nvram_get_entry_by_id(struct yaa_nvram_ctx *ctx,
                                               const yaa_nvram_key_t number)
{
    if (number >= YAA_NVRAM_MAX_ENTRIES)
    {
        NVRAM_ERR("Boundary fail");
        return NULL;
    }

    return &ctx->yaa_nvram_entries[number];
}

/**
 * @brief Allocate a new entry in the cache.
 *
 * Reserves space for one additional entry and returns its address.
 *
 * @warning
 * This function does NOT check for available space.
 * The caller must ensure that free entries are available by
 * calling ::yaa_nvram_map_free_entries().
 *
 * @return Pointer to newly allocated ::yaa_nvram_entry_t.
 */
yaa_nvram_entry_t *yaa_nvram_create_entry(struct yaa_nvram_ctx *ctx)
{
    const yaa_nvram_key_t key = ctx->used_entries;
    ctx->used_entries += 1;

    NVRAM_DEB("Create entry key %d", (int)key);

    YAA_ASSERT(key < YAA_NVRAM_MAX_ENTRIES);
    return &ctx->yaa_nvram_entries[key];
}

/**
 * @brief Create or update entry mapping.
 *
 * Updates the offset associated with the specified key.
 * If the key does not exist in the cache, a new entry is created.
 *
 * @param[in] key     Key identifying the entry.
 * @param[in] offset  Logical offset of the entry in NVRAM (in bytes).
 *
 * @return
 * - ::YAA_ERR_OK     Entry successfully updated or created.
 * - ::YAA_ERR_NOMEM  No free cache entries available.
 */
yaa_err_t yaa_nvram_update_entry(struct yaa_nvram_ctx *ctx,
                                   const yaa_nvram_key_t key,
                                   const yaa_nvram_offset_t offset)
{
    yaa_nvram_entry_t *entry;

    NVRAM_DEB("Update entry key %d offset %d", (int)key, (int)offset);

    if (YAA_ERR_NOTFOUND == yaa_nvram_get_entry(ctx, key, &entry))
    {
        if (0 == yaa_nvram_map_free_entries(ctx))
        {
            return YAA_ERR_NOMEM;
        }

        entry = yaa_nvram_create_entry(ctx);
        entry->key = key;
    }

    entry->offset = offset;

    return YAA_ERR_OK;
}

/**
 * @brief Reset the internal cache.
 *
 * Clears all stored entries by resetting the usage counter.
 * Previously stored mappings become invalid.
 */
void yaa_nvram_reset_map(struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Set entries to 0");
    ctx->used_entries = 0;
}

/**
 * @brief Get number of used entries.
 *
 * @return Number of currently stored unique keys.
 */
size_t yaa_nvram_get_used_entries(const struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Use entries used : %d", (int)ctx->used_entries);
    return ctx->used_entries;
}

/**
 * @brief Get number of free entry slots.
 *
 * @return Remaining available entry slots in the cache.
 */
size_t yaa_nvram_map_free_entries(const struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Free entries max : %d, used : %d", (int)YAA_NVRAM_MAX_ENTRIES, (int)ctx->used_entries);
    return (YAA_NVRAM_MAX_ENTRIES - ctx->used_entries);
}
