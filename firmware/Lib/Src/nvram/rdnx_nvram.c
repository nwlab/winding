/**
 * @file    rdnx_nvram.c
 * @author  Software development team
 * @brief   User-level interface for NVRAM key-value access.
 *
 * Provides high-level functions to initialize NVRAM, read and write key-value
 * pairs, and query usage statistics. Relies on internal NVRAM modules:
 * ::entry, ::map, and ::storage.
 *
 * @note
 * - All keys and values are managed in RAM cache (::rdnx_nvram_entries).
 * - Proper initialization must be performed with ::rdnx_nvram_init before usage.
 */

 /* ============================================================================
 * Include Files
 * ==========================================================================*/

 /* Core includes. */
#include <rdnx_macro.h>
#include "rdnx_nvram.h"
#include "rdnx_nvram_internal.h"
#include "entry.h"
#include "map.h"
#include "rdnx_sal.h"
#include "rdnx_types.h"
#include "storage.h"

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/**
 * @brief Initialize NVRAM for user-level access.
 *
 * Reads NVRAM content and builds its RAM cache map.
 *
 * @param[in] interface  Pointer to NVRAM access interface.
 *
 * @return
 * - NVRAM capacity in entries (main area only). This value divided by
 *   ::RDNX_NVRAM_MAX_ENTRIES gives an expected leveling factor or
 *   write cycles multiplier.
 * - 0 if NVRAM is too small to store the required entries.
 *
 * @note Must be called before any get/set operations.
 */
rdnx_err_t rdnx_nvram_init(const rdnx_nvram_config_t *interface, rdnx_nvram_handle_t *handle)
{
    if (interface == NULL || handle == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    if (interface->erase_main == NULL ||
        interface->erase_reserve == NULL ||
        interface->write == NULL ||
        interface->read == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *,
                                           rdnx_alloc(sizeof(struct rdnx_nvram_ctx)));
    if (ctx == NULL)
    {
        return RDNX_ERR_NOMEM;
    }

    const rdnx_nvram_offset_t main_size = interface->size - interface->reserved;
    const rdnx_nvram_offset_t reserve_capacity = interface->reserved / RDNX_NVRAM_ENTRY_SIZE;
    const rdnx_nvram_offset_t main_capacity = main_size / RDNX_NVRAM_ENTRY_SIZE;

    const uint8_t reserve_size_wrong = interface->reserved >= interface->size;
    const uint8_t main_smaller_reserve = main_capacity < reserve_capacity;

    if ((reserve_size_wrong) || (main_smaller_reserve) || (main_capacity <= RDNX_NVRAM_MAX_ENTRIES) ||
        (reserve_capacity <= RDNX_NVRAM_MAX_ENTRIES))
    {
        return RDNX_ERR_NOMEM;
    }

    NVRAM_DEB("Main size %d", (int)main_size);
    NVRAM_DEB("Main capacity %d", (int)main_capacity);
    NVRAM_DEB("Reserve capacity %d", (int)reserve_capacity);

    ctx->interface = *interface;

    rdnx_nvram_cold_boot(ctx);

    ctx->initialized = 1;

    *handle = RDNX_CAST(rdnx_nvram_handle_t, ctx);

    return RDNX_ERR_OK;
}

rdnx_nvram_offset_t rdnx_nvram_capacity(rdnx_nvram_handle_t handle)
{
    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return 0;
    }
    const rdnx_nvram_offset_t main_size = ctx->interface.size - ctx->interface.reserved;
    const rdnx_nvram_offset_t main_capacity = main_size / RDNX_NVRAM_ENTRY_SIZE;

    /* Number of entry-sized blocks */
    return (rdnx_nvram_key_t)main_capacity;
}

/**
 * @brief Read value for a specified key.
 *
 * @param[in]  key    Key to read.
 * @param[out] value  Pointer to store the read value on success.
 *
 * @return
 * - ::RDNX_ERR_OK        Success.
 * - ::RDNX_ERR_NOTFOUND  Key does not exist.
 * - ::RDNX_ERR_NORESOURCE NVRAM not initialized.
 */
rdnx_err_t rdnx_nvram_get_value(rdnx_nvram_handle_t handle, rdnx_nvram_key_t key, rdnx_nvram_value_t *value)
{
    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    if (!ctx->initialized)
    {
        return RDNX_ERR_NORESOURCE;
    }

    rdnx_nvram_entry_t *entry;
    if (rdnx_nvram_get_entry(ctx, key, &entry))
    {
        NVRAM_ERR("Entry not found");
        return RDNX_ERR_NOTFOUND;
    }

    return rdnx_nvram_read_entry(ctx, entry->offset, &key, value);
}

/**
 * @brief Write value for a specified key.
 *
 * Allocates a new block in main NVRAM area if needed and updates cache.
 *
 * @param[in] key    Key to write.
 * @param[in] value  Value to be stored.
 *
 * @return
 * - ::RDNX_ERR_OK        Success.
 * - ::RDNX_ERR_NORESOURCE NVRAM not initialized.
 * - ::RDNX_ERR_NOMEM     No free entries available in cache.
 */
rdnx_err_t rdnx_nvram_set_value(rdnx_nvram_handle_t handle, rdnx_nvram_key_t key, rdnx_nvram_value_t value)
{
    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    if (!ctx->initialized)
    {
        return RDNX_ERR_NORESOURCE;
    }

    rdnx_nvram_offset_t offset = rdnx_nvram_get_next_block(ctx);
    rdnx_nvram_entry_t *entry;

    if (RDNX_ERR_NOTFOUND == rdnx_nvram_get_entry(ctx, key, &entry) &&
        (0 == rdnx_nvram_map_free_entries(ctx)))
    {
        NVRAM_ERR("No space");
        return RDNX_ERR_NOMEM;
    }

    rdnx_err_t write = rdnx_nvram_write_entry(ctx, offset, key, value);
    if (RDNX_ERR_OK == write)
    {
        rdnx_nvram_update_entry(ctx, key, offset);
    }

    return write;
}

/**
 * @brief Get the number of unique key entries currently stored.
 *
 * @return Number of keys in use.
 */
size_t rdnx_nvram_get_entries_number(rdnx_nvram_handle_t handle)
{
    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return 0;
    }

    return rdnx_nvram_get_used_entries(ctx);
}

/**
 * @brief Get the number of available unique key slots.
 *
 * @return Number of free key slots.
 */
size_t rdnx_nvram_get_free_entries(rdnx_nvram_handle_t handle)
{
    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return 0;
    }

    return rdnx_nvram_map_free_entries(ctx);
}

const char *rdnx_nvram_state_to_str(rdnx_nvram_state_t state)
{
    switch (state)
    {
    case RDNX_NVRAM_STATE_BLANK:
        return "RDNX_NVRAM_STATE_BLANK";
    case RDNX_NVRAM_STATE_CLEAN:
        return "RDNX_NVRAM_STATE_CLEAN";
    case RDNX_NVRAM_STATE_MAIN_ERASE_INTERRUPTED:
        return "RDNX_NVRAM_STATE_MAIN_ERASE_INTERRUPTED";
    case RDNX_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED:
        return "RDNX_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED";
    default:
        break;
    }
    return "UNKNOWN";
}
