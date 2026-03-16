/**
 * @file    yaa_nvram.c
 * @author  Software development team
 * @brief   User-level interface for NVRAM key-value access.
 *
 * Provides high-level functions to initialize NVRAM, read and write key-value
 * pairs, and query usage statistics. Relies on internal NVRAM modules:
 * ::entry, ::map, and ::storage.
 *
 * @note
 * - All keys and values are managed in RAM cache (::yaa_nvram_entries).
 * - Proper initialization must be performed with ::yaa_nvram_init before usage.
 */

 /* ============================================================================
 * Include Files
 * ==========================================================================*/

 /* Core includes. */
#include <yaa_macro.h>
#include "yaa_nvram.h"
#include "yaa_nvram_internal.h"
#include "entry.h"
#include "map.h"
#include "yaa_sal.h"
#include "yaa_types.h"
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
 *   ::YAA_NVRAM_MAX_ENTRIES gives an expected leveling factor or
 *   write cycles multiplier.
 * - 0 if NVRAM is too small to store the required entries.
 *
 * @note Must be called before any get/set operations.
 */
yaa_err_t yaa_nvram_init(const yaa_nvram_config_t *interface, yaa_nvram_handle_t *handle)
{
    if (interface == NULL || handle == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (interface->erase_main == NULL ||
        interface->erase_reserve == NULL ||
        interface->write == NULL ||
        interface->read == NULL)
    {
        return YAA_ERR_BADARG;
    }

    struct yaa_nvram_ctx *ctx = YAA_CAST(struct yaa_nvram_ctx *,
                                           yaa_alloc(sizeof(struct yaa_nvram_ctx)));
    if (ctx == NULL)
    {
        return YAA_ERR_NOMEM;
    }

    const yaa_nvram_offset_t main_size = interface->size - interface->reserved;
    const yaa_nvram_offset_t reserve_capacity = interface->reserved / YAA_NVRAM_ENTRY_SIZE;
    const yaa_nvram_offset_t main_capacity = main_size / YAA_NVRAM_ENTRY_SIZE;

    const uint8_t reserve_size_wrong = interface->reserved >= interface->size;
    const uint8_t main_smaller_reserve = main_capacity < reserve_capacity;

    if ((reserve_size_wrong) || (main_smaller_reserve) || (main_capacity <= YAA_NVRAM_MAX_ENTRIES) ||
        (reserve_capacity <= YAA_NVRAM_MAX_ENTRIES))
    {
        return YAA_ERR_NOMEM;
    }

    NVRAM_DEB("Main size %d", (int)main_size);
    NVRAM_DEB("Main capacity %d", (int)main_capacity);
    NVRAM_DEB("Reserve capacity %d", (int)reserve_capacity);

    ctx->interface = *interface;

    yaa_nvram_cold_boot(ctx);

    ctx->initialized = 1;

    *handle = YAA_CAST(yaa_nvram_handle_t, ctx);

    return YAA_ERR_OK;
}

yaa_nvram_offset_t yaa_nvram_capacity(yaa_nvram_handle_t handle)
{
    struct yaa_nvram_ctx *ctx = YAA_CAST(struct yaa_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return 0;
    }
    const yaa_nvram_offset_t main_size = ctx->interface.size - ctx->interface.reserved;
    const yaa_nvram_offset_t main_capacity = main_size / YAA_NVRAM_ENTRY_SIZE;

    /* Number of entry-sized blocks */
    return (yaa_nvram_key_t)main_capacity;
}

/**
 * @brief Read value for a specified key.
 *
 * @param[in]  key    Key to read.
 * @param[out] value  Pointer to store the read value on success.
 *
 * @return
 * - ::YAA_ERR_OK        Success.
 * - ::YAA_ERR_NOTFOUND  Key does not exist.
 * - ::YAA_ERR_NORESOURCE NVRAM not initialized.
 */
yaa_err_t yaa_nvram_get_value(yaa_nvram_handle_t handle, yaa_nvram_key_t key, yaa_nvram_value_t *value)
{
    struct yaa_nvram_ctx *ctx = YAA_CAST(struct yaa_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (!ctx->initialized)
    {
        return YAA_ERR_NORESOURCE;
    }

    yaa_nvram_entry_t *entry;
    if (yaa_nvram_get_entry(ctx, key, &entry))
    {
        NVRAM_ERR("Entry not found");
        return YAA_ERR_NOTFOUND;
    }

    return yaa_nvram_read_entry(ctx, entry->offset, &key, value);
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
 * - ::YAA_ERR_OK        Success.
 * - ::YAA_ERR_NORESOURCE NVRAM not initialized.
 * - ::YAA_ERR_NOMEM     No free entries available in cache.
 */
yaa_err_t yaa_nvram_set_value(yaa_nvram_handle_t handle, yaa_nvram_key_t key, yaa_nvram_value_t value)
{
    struct yaa_nvram_ctx *ctx = YAA_CAST(struct yaa_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return YAA_ERR_BADARG;
    }

    if (!ctx->initialized)
    {
        return YAA_ERR_NORESOURCE;
    }

    yaa_nvram_offset_t offset = yaa_nvram_get_next_block(ctx);
    yaa_nvram_entry_t *entry;

    if (YAA_ERR_NOTFOUND == yaa_nvram_get_entry(ctx, key, &entry) &&
        (0 == yaa_nvram_map_free_entries(ctx)))
    {
        NVRAM_ERR("No space");
        return YAA_ERR_NOMEM;
    }

    yaa_err_t write = yaa_nvram_write_entry(ctx, offset, key, value);
    if (YAA_ERR_OK == write)
    {
        yaa_nvram_update_entry(ctx, key, offset);
    }

    return write;
}

/**
 * @brief Get the number of unique key entries currently stored.
 *
 * @return Number of keys in use.
 */
size_t yaa_nvram_get_entries_number(yaa_nvram_handle_t handle)
{
    struct yaa_nvram_ctx *ctx = YAA_CAST(struct yaa_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return 0;
    }

    return yaa_nvram_get_used_entries(ctx);
}

/**
 * @brief Get the number of available unique key slots.
 *
 * @return Number of free key slots.
 */
size_t yaa_nvram_get_free_entries(yaa_nvram_handle_t handle)
{
    struct yaa_nvram_ctx *ctx = YAA_CAST(struct yaa_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return 0;
    }

    return yaa_nvram_map_free_entries(ctx);
}

const char *yaa_nvram_state_to_str(yaa_nvram_state_t state)
{
    switch (state)
    {
    case YAA_NVRAM_STATE_BLANK:
        return "YAA_NVRAM_STATE_BLANK";
    case YAA_NVRAM_STATE_CLEAN:
        return "YAA_NVRAM_STATE_CLEAN";
    case YAA_NVRAM_STATE_MAIN_ERASE_INTERRUPTED:
        return "YAA_NVRAM_STATE_MAIN_ERASE_INTERRUPTED";
    case YAA_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED:
        return "YAA_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED";
    default:
        break;
    }
    return "UNKNOWN";
}
