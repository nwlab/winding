/**
 * @file    storage.c
 * @author  Software development team
 * @brief   NVRAM state management and defragmentation.
 *
 * This module handles the state of two NVRAM areas:
 * - Main: used for normal operations.
 * - Reserved: used as a backup copy during wrap-around or defragmentation,
 *   allowing recovery in case of power loss.
 *
 * The module provides:
 * - Cold boot initialization.
 * - Area preparation and recovery after interrupted erase operations.
 * - Linear indexing of entries.
 * - Backup and restore between main and reserved areas.
 * - Allocation of next free blocks for writing.
 *
 * @note
 * - All block allocation and transfers are aligned to ::YAA_NVRAM_ENTRY_SIZE.
 * - Metadata in both areas is used to track erase progress for recovery.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Core includes */
#include <yaa_nvram.h>
#include "yaa_nvram_internal.h"
#include "entry.h"
#include "map.h"
#include "storage.h"

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

/* ============================================================================
 * Private helper function declarations
 * ==========================================================================*/

static yaa_nvram_state_t get_nvram_state(struct yaa_nvram_ctx *ctx);
static void load_map(struct yaa_nvram_ctx *ctx);
static void prepare_for_first_use(struct yaa_nvram_ctx *ctx);
static void recover_after_interrupted_main_erase(struct yaa_nvram_ctx *ctx);
static void recover_after_interrupted_reserve_erase(struct yaa_nvram_ctx *ctx);
static void prepare_area(struct yaa_nvram_ctx *ctx, yaa_nvram_area_t area);
static void transfer_main_to_reserve(struct yaa_nvram_ctx *ctx);
static void transfer_reserve_to_main(struct yaa_nvram_ctx *ctx);

/* ============================================================================
 * Public Function Declarations (externs in header file)
 * ==========================================================================*/

const char *yaa_nvram_state_to_str(yaa_nvram_state_t state);

/* ============================================================================
 * Public Function Implementations
 * ==========================================================================*/

/**
 * @brief Perform cold boot initialization of NVRAM.
 *
 * Calculates the current NVRAM state and starts the appropriate
 * initialization or recovery procedure.
 */
void yaa_nvram_cold_boot(struct yaa_nvram_ctx *ctx)
{
    yaa_nvram_reset_map(ctx);

    const yaa_nvram_state_t nvram_state = get_nvram_state(ctx);

    NVRAM_DEB("State %s", yaa_nvram_state_to_str(nvram_state));

    switch (nvram_state)
    {
        case YAA_NVRAM_STATE_CLEAN:
            load_map(ctx);
            break;

        case YAA_NVRAM_STATE_BLANK:
            prepare_for_first_use(ctx);
            break;

        case YAA_NVRAM_STATE_MAIN_ERASE_INTERRUPTED:
            recover_after_interrupted_main_erase(ctx);
            break;

        case YAA_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED:
            recover_after_interrupted_reserve_erase(ctx);
            break;

        default:
            prepare_for_first_use(ctx);
            break;
    }
}

/**
 * @brief Scan main area and index its entries into cache.
 *
 * Performs a linear scan from metadata offset, stops at first free block.
 * A block is free if all bytes equal ::YAA_NVRAM_ERASED_BYTE_VALUE.
 */
static void load_map(struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Load map");

    yaa_nvram_reset_map(ctx);

    yaa_nvram_offset_t offset;
    for (offset = YAA_NVRAM_METADATA_SIZE;
         (offset + YAA_NVRAM_ENTRY_SIZE) <= (ctx->interface.size - ctx->interface.reserved);
         offset += YAA_NVRAM_ENTRY_SIZE)
    {
        yaa_nvram_key_t key;
        yaa_nvram_value_t value;
        yaa_err_t ret = yaa_nvram_read_entry(ctx, offset, &key, &value);

        if (YAA_ERR_NOTFOUND == ret)
        {
            break;
        }

        if (YAA_ERR_OK == ret)
        {
            yaa_nvram_update_entry(ctx, key, offset);
        }
    }

    ctx->next_block = offset;
}

/**
 * @brief Prepare NVRAM for first use.
 *
 * Erases both main and reserved areas and initializes metadata.
 */
static void prepare_for_first_use(struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Prepare for first use");

    ctx->interface.erase_main();
    ctx->interface.erase_reserve();

    uint8_t main_metadata[YAA_NVRAM_METADATA_SIZE] =
        { YAA_NVRAM_ERASE_STARTED, YAA_NVRAM_ERASE_FINISHED, 0xFF, 0xFF };

    if( ctx->interface.write(main_metadata, 0, YAA_NVRAM_METADATA_SIZE) != YAA_ERR_OK)
    {
        NVRAM_ERR("Fail writing to flash");
    }

    ctx->next_block = YAA_NVRAM_METADATA_SIZE;
}

/**
 * @brief Recover from interrupted main area erase.
 */
static void recover_after_interrupted_main_erase(struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Recover after interrupted main erase");
    ctx->interface.erase_main();
    transfer_reserve_to_main(ctx);
    prepare_area(ctx, YAA_NVRAM_AREA_RESERVED);
}

/**
 * @brief Recover from interrupted reserved area erase.
 */
static void recover_after_interrupted_reserve_erase(struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Recover after interrupted reserve erase");
    ctx->interface.erase_reserve();
    load_map(ctx);
}

/**
 * @brief Get absolute offset within reserved area.
 *
 * @param[in] offset  Offset relative to start of reserved area.
 * @return Absolute offset in NVRAM.
 */
static inline yaa_nvram_offset_t get_reserve_offset(struct yaa_nvram_ctx *ctx, yaa_nvram_offset_t offset)
{
    const yaa_nvram_offset_t reserve_offset = ctx->interface.size - ctx->interface.reserved + offset;
    NVRAM_DEB("Get reserve offset %d", (int)reserve_offset);
    return reserve_offset;
}

/**
 * @brief Transfer entries from reserved area back to main area.
 */
static void transfer_reserve_to_main(struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Transfer reserve to main, reserved %d", (int)ctx->interface.reserved);
    const yaa_nvram_offset_t reserve_offset = get_reserve_offset(ctx, 0);

    yaa_nvram_offset_t offset;
    for (offset = YAA_NVRAM_METADATA_SIZE;
         (offset + YAA_NVRAM_ENTRY_SIZE) <= ctx->interface.reserved;
         offset += YAA_NVRAM_ENTRY_SIZE)
    {
        yaa_nvram_key_t key = 0;
        yaa_nvram_value_t value = 0;
        yaa_err_t ret = yaa_nvram_read_entry(ctx, reserve_offset + offset, &key, &value);

        if (YAA_ERR_NOTFOUND == ret)
        {
            NVRAM_DEB("Key %d not found, offset %d", (int)key, (int)offset);
            break;
        }

        if (YAA_ERR_OK == ret)
        {
            NVRAM_DEB("Write entry key %d, offset %d", (int)key, (int)offset);
            yaa_nvram_write_entry(ctx, offset, key, value);
            yaa_nvram_update_entry(ctx, key, offset);
        }
    }

    ctx->next_block = offset;
}

/**
 * @brief Backup main area to reserved area.
 */
static void transfer_main_to_reserve(struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Main to reserve");
    yaa_nvram_offset_t reserve_offset = get_reserve_offset(ctx, YAA_NVRAM_METADATA_SIZE);

    for (yaa_nvram_key_t i = 0; i < yaa_nvram_get_used_entries(ctx); i++)
    {
        const yaa_nvram_entry_t *entry = yaa_nvram_get_entry_by_id(ctx, i);
        if (entry != NULL)
        {
            yaa_nvram_key_t key;
            yaa_nvram_value_t value;
            yaa_nvram_read_entry(ctx, entry->offset, &key, &value);
            yaa_nvram_write_entry(ctx, reserve_offset, key, value);
            reserve_offset += YAA_NVRAM_ENTRY_SIZE;
        }
    }
}

/**
 * @brief Determine current NVRAM state.
 *
 * Checks metadata and block contents to identify clean, blank, or
 * interrupted erase states.
 *
 * @return Current NVRAM state as ::yaa_nvram_state_t.
 */
static yaa_nvram_state_t get_nvram_state(struct yaa_nvram_ctx *ctx)
{
    uint8_t main_metadata[YAA_NVRAM_MINIMAL_SIZE];
    uint8_t reserve_metadata[YAA_NVRAM_MINIMAL_SIZE];

    (void)ctx->interface.read(main_metadata, 0, YAA_NVRAM_MINIMAL_SIZE);
    (void)ctx->interface.read(reserve_metadata, get_reserve_offset(ctx, 0), YAA_NVRAM_MINIMAL_SIZE);

    const uint8_t main_started   = (YAA_NVRAM_ERASE_STARTED == reserve_metadata[YAA_NVRAM_OFFSET_ERASE_STARTED]);
    const uint8_t reserve_started = (YAA_NVRAM_ERASE_STARTED == main_metadata[YAA_NVRAM_OFFSET_ERASE_STARTED]);
    const uint8_t main_finished  = (YAA_NVRAM_ERASE_FINISHED == reserve_metadata[YAA_NVRAM_OFFSET_ERASE_FINISHED]);
    const uint8_t reserve_finished = (YAA_NVRAM_ERASE_FINISHED == main_metadata[YAA_NVRAM_OFFSET_ERASE_FINISHED]);
    const uint8_t main_clean     = yaa_nvram_is_block_erased(main_metadata, YAA_NVRAM_MINIMAL_SIZE);
    const uint8_t reserve_clean  = yaa_nvram_is_block_erased(reserve_metadata, YAA_NVRAM_MINIMAL_SIZE);

    if (reserve_finished && reserve_clean)
    {
        NVRAM_DEB("YAA_NVRAM_STATE_CLEAN");
        return YAA_NVRAM_STATE_CLEAN;
    }

    if ((main_started || main_finished) && !main_clean)
    {
        NVRAM_DEB("YAA_NVRAM_STATE_MAIN_ERASE_INTERRUPTED");
        return YAA_NVRAM_STATE_MAIN_ERASE_INTERRUPTED;
    }

    if ((reserve_finished && !reserve_clean) || (reserve_started && !reserve_finished))
    {
        NVRAM_DEB("YAA_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED");
        return YAA_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED;
    }

    NVRAM_DEB("YAA_NVRAM_STATE_BLANK");
    return YAA_NVRAM_STATE_BLANK;
}

/**
 * @brief Erase a specified area with metadata tracking.
 *
 * @param[in] area  Area to erase (main or reserved).
 */
static void prepare_area(struct yaa_nvram_ctx *ctx, yaa_nvram_area_t area)
{
    NVRAM_DEB("Prepare area %s", YAA_NVRAM_AREA_RESERVED == area ? "RESERVED" : "MAIN");
    uint8_t operation_flag = YAA_NVRAM_ERASE_STARTED;
    yaa_nvram_offset_t base_address = get_reserve_offset(ctx, 0);
    yaa_nvram_erase_fn erase_function = ctx->interface.erase_main;

    if (YAA_NVRAM_AREA_RESERVED == area)
    {
        base_address = 0;
        erase_function = ctx->interface.erase_reserve;
    }

    (void)ctx->interface.write(&operation_flag, base_address, 1);
    erase_function();
    operation_flag = YAA_NVRAM_ERASE_FINISHED;
    (void)ctx->interface.write(&operation_flag, base_address + 1, 1);
}

/**
 * @brief Defragment main area by backing up to reserved area.
 */
static void restart_map(struct yaa_nvram_ctx *ctx)
{
    NVRAM_DEB("Restart map");
    transfer_main_to_reserve(ctx);
    prepare_area(ctx, YAA_NVRAM_AREA_MAIN);
    transfer_reserve_to_main(ctx);
    prepare_area(ctx, YAA_NVRAM_AREA_RESERVED);
}

/**
 * @brief Reserve next free block in main area.
 *
 * If all NVRAM is used, triggers defragmentation and backup.
 *
 * @return Offset of the first byte of the new block.
 */
yaa_nvram_offset_t yaa_nvram_get_next_block(struct yaa_nvram_ctx *ctx)
{
    if ((ctx->next_block + YAA_NVRAM_ENTRY_SIZE) > (ctx->interface.size - ctx->interface.reserved))
    {
        restart_map(ctx);
    }

    yaa_nvram_offset_t block = ctx->next_block;
    ctx->next_block += YAA_NVRAM_ENTRY_SIZE;

    NVRAM_DEB("Next block %d", (int)block);

    return block;
}
