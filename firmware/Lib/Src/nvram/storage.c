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
 * - All block allocation and transfers are aligned to ::RDNX_NVRAM_ENTRY_SIZE.
 * - Metadata in both areas is used to track erase progress for recovery.
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Core includes */
#include <rdnx_nvram.h>
#include "rdnx_nvram_internal.h"
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

static rdnx_nvram_state_t get_nvram_state(struct rdnx_nvram_ctx *ctx);
static void load_map(struct rdnx_nvram_ctx *ctx);
static void prepare_for_first_use(struct rdnx_nvram_ctx *ctx);
static void recover_after_interrupted_main_erase(struct rdnx_nvram_ctx *ctx);
static void recover_after_interrupted_reserve_erase(struct rdnx_nvram_ctx *ctx);
static void prepare_area(struct rdnx_nvram_ctx *ctx, rdnx_nvram_area_t area);
static void transfer_main_to_reserve(struct rdnx_nvram_ctx *ctx);
static void transfer_reserve_to_main(struct rdnx_nvram_ctx *ctx);

/* ============================================================================
 * Public Function Declarations (externs in header file)
 * ==========================================================================*/

const char *rdnx_nvram_state_to_str(rdnx_nvram_state_t state);

/* ============================================================================
 * Public Function Implementations
 * ==========================================================================*/

/**
 * @brief Perform cold boot initialization of NVRAM.
 *
 * Calculates the current NVRAM state and starts the appropriate
 * initialization or recovery procedure.
 */
void rdnx_nvram_cold_boot(struct rdnx_nvram_ctx *ctx)
{
    rdnx_nvram_reset_map(ctx);

    const rdnx_nvram_state_t nvram_state = get_nvram_state(ctx);

    NVRAM_DEB("State %s", rdnx_nvram_state_to_str(nvram_state));

    switch (nvram_state)
    {
        case RDNX_NVRAM_STATE_CLEAN:
            load_map(ctx);
            break;

        case RDNX_NVRAM_STATE_BLANK:
            prepare_for_first_use(ctx);
            break;

        case RDNX_NVRAM_STATE_MAIN_ERASE_INTERRUPTED:
            recover_after_interrupted_main_erase(ctx);
            break;

        case RDNX_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED:
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
 * A block is free if all bytes equal ::RDNX_NVRAM_ERASED_BYTE_VALUE.
 */
static void load_map(struct rdnx_nvram_ctx *ctx)
{
    NVRAM_DEB("Load map");

    rdnx_nvram_reset_map(ctx);

    rdnx_nvram_offset_t offset;
    for (offset = RDNX_NVRAM_METADATA_SIZE;
         (offset + RDNX_NVRAM_ENTRY_SIZE) <= (ctx->interface.size - ctx->interface.reserved);
         offset += RDNX_NVRAM_ENTRY_SIZE)
    {
        rdnx_nvram_key_t key;
        rdnx_nvram_value_t value;
        rdnx_err_t ret = rdnx_nvram_read_entry(ctx, offset, &key, &value);

        if (RDNX_ERR_NOTFOUND == ret)
        {
            break;
        }

        if (RDNX_ERR_OK == ret)
        {
            rdnx_nvram_update_entry(ctx, key, offset);
        }
    }

    ctx->next_block = offset;
}

/**
 * @brief Prepare NVRAM for first use.
 *
 * Erases both main and reserved areas and initializes metadata.
 */
static void prepare_for_first_use(struct rdnx_nvram_ctx *ctx)
{
    NVRAM_DEB("Prepare for first use");

    ctx->interface.erase_main();
    ctx->interface.erase_reserve();

    uint8_t main_metadata[RDNX_NVRAM_METADATA_SIZE] =
        { RDNX_NVRAM_ERASE_STARTED, RDNX_NVRAM_ERASE_FINISHED, 0xFF, 0xFF };

    if( ctx->interface.write(main_metadata, 0, RDNX_NVRAM_METADATA_SIZE) != RDNX_ERR_OK)
    {
        NVRAM_ERR("Fail writing to flash");
    }

    ctx->next_block = RDNX_NVRAM_METADATA_SIZE;
}

/**
 * @brief Recover from interrupted main area erase.
 */
static void recover_after_interrupted_main_erase(struct rdnx_nvram_ctx *ctx)
{
    NVRAM_DEB("Recover after interrupted main erase");
    ctx->interface.erase_main();
    transfer_reserve_to_main(ctx);
    prepare_area(ctx, RDNX_NVRAM_AREA_RESERVED);
}

/**
 * @brief Recover from interrupted reserved area erase.
 */
static void recover_after_interrupted_reserve_erase(struct rdnx_nvram_ctx *ctx)
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
static inline rdnx_nvram_offset_t get_reserve_offset(struct rdnx_nvram_ctx *ctx, rdnx_nvram_offset_t offset)
{
    const rdnx_nvram_offset_t reserve_offset = ctx->interface.size - ctx->interface.reserved + offset;
    NVRAM_DEB("Get reserve offset %d", (int)reserve_offset);
    return reserve_offset;
}

/**
 * @brief Transfer entries from reserved area back to main area.
 */
static void transfer_reserve_to_main(struct rdnx_nvram_ctx *ctx)
{
    NVRAM_DEB("Transfer reserve to main, reserved %d", (int)ctx->interface.reserved);
    const rdnx_nvram_offset_t reserve_offset = get_reserve_offset(ctx, 0);

    rdnx_nvram_offset_t offset;
    for (offset = RDNX_NVRAM_METADATA_SIZE;
         (offset + RDNX_NVRAM_ENTRY_SIZE) <= ctx->interface.reserved;
         offset += RDNX_NVRAM_ENTRY_SIZE)
    {
        rdnx_nvram_key_t key = 0;
        rdnx_nvram_value_t value = 0;
        rdnx_err_t ret = rdnx_nvram_read_entry(ctx, reserve_offset + offset, &key, &value);

        if (RDNX_ERR_NOTFOUND == ret)
        {
            NVRAM_DEB("Key %d not found, offset %d", (int)key, (int)offset);
            break;
        }

        if (RDNX_ERR_OK == ret)
        {
            NVRAM_DEB("Write entry key %d, offset %d", (int)key, (int)offset);
            rdnx_nvram_write_entry(ctx, offset, key, value);
            rdnx_nvram_update_entry(ctx, key, offset);
        }
    }

    ctx->next_block = offset;
}

/**
 * @brief Backup main area to reserved area.
 */
static void transfer_main_to_reserve(struct rdnx_nvram_ctx *ctx)
{
    NVRAM_DEB("Main to reserve");
    rdnx_nvram_offset_t reserve_offset = get_reserve_offset(ctx, RDNX_NVRAM_METADATA_SIZE);

    for (rdnx_nvram_key_t i = 0; i < rdnx_nvram_get_used_entries(ctx); i++)
    {
        const rdnx_nvram_entry_t *entry = rdnx_nvram_get_entry_by_id(ctx, i);
        if (entry != NULL)
        {
            rdnx_nvram_key_t key;
            rdnx_nvram_value_t value;
            rdnx_nvram_read_entry(ctx, entry->offset, &key, &value);
            rdnx_nvram_write_entry(ctx, reserve_offset, key, value);
            reserve_offset += RDNX_NVRAM_ENTRY_SIZE;
        }
    }
}

/**
 * @brief Determine current NVRAM state.
 *
 * Checks metadata and block contents to identify clean, blank, or
 * interrupted erase states.
 *
 * @return Current NVRAM state as ::rdnx_nvram_state_t.
 */
static rdnx_nvram_state_t get_nvram_state(struct rdnx_nvram_ctx *ctx)
{
    uint8_t main_metadata[RDNX_NVRAM_MINIMAL_SIZE];
    uint8_t reserve_metadata[RDNX_NVRAM_MINIMAL_SIZE];

    (void)ctx->interface.read(main_metadata, 0, RDNX_NVRAM_MINIMAL_SIZE);
    (void)ctx->interface.read(reserve_metadata, get_reserve_offset(ctx, 0), RDNX_NVRAM_MINIMAL_SIZE);

    const uint8_t main_started   = (RDNX_NVRAM_ERASE_STARTED == reserve_metadata[RDNX_NVRAM_OFFSET_ERASE_STARTED]);
    const uint8_t reserve_started = (RDNX_NVRAM_ERASE_STARTED == main_metadata[RDNX_NVRAM_OFFSET_ERASE_STARTED]);
    const uint8_t main_finished  = (RDNX_NVRAM_ERASE_FINISHED == reserve_metadata[RDNX_NVRAM_OFFSET_ERASE_FINISHED]);
    const uint8_t reserve_finished = (RDNX_NVRAM_ERASE_FINISHED == main_metadata[RDNX_NVRAM_OFFSET_ERASE_FINISHED]);
    const uint8_t main_clean     = rdnx_nvram_is_block_erased(main_metadata, RDNX_NVRAM_MINIMAL_SIZE);
    const uint8_t reserve_clean  = rdnx_nvram_is_block_erased(reserve_metadata, RDNX_NVRAM_MINIMAL_SIZE);

    if (reserve_finished && reserve_clean)
    {
        NVRAM_DEB("RDNX_NVRAM_STATE_CLEAN");
        return RDNX_NVRAM_STATE_CLEAN;
    }

    if ((main_started || main_finished) && !main_clean)
    {
        NVRAM_DEB("RDNX_NVRAM_STATE_MAIN_ERASE_INTERRUPTED");
        return RDNX_NVRAM_STATE_MAIN_ERASE_INTERRUPTED;
    }

    if ((reserve_finished && !reserve_clean) || (reserve_started && !reserve_finished))
    {
        NVRAM_DEB("RDNX_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED");
        return RDNX_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED;
    }

    NVRAM_DEB("RDNX_NVRAM_STATE_BLANK");
    return RDNX_NVRAM_STATE_BLANK;
}

/**
 * @brief Erase a specified area with metadata tracking.
 *
 * @param[in] area  Area to erase (main or reserved).
 */
static void prepare_area(struct rdnx_nvram_ctx *ctx, rdnx_nvram_area_t area)
{
    NVRAM_DEB("Prepare area %s", RDNX_NVRAM_AREA_RESERVED == area ? "RESERVED" : "MAIN");
    uint8_t operation_flag = RDNX_NVRAM_ERASE_STARTED;
    rdnx_nvram_offset_t base_address = get_reserve_offset(ctx, 0);
    rdnx_nvram_erase_fn erase_function = ctx->interface.erase_main;

    if (RDNX_NVRAM_AREA_RESERVED == area)
    {
        base_address = 0;
        erase_function = ctx->interface.erase_reserve;
    }

    (void)ctx->interface.write(&operation_flag, base_address, 1);
    erase_function();
    operation_flag = RDNX_NVRAM_ERASE_FINISHED;
    (void)ctx->interface.write(&operation_flag, base_address + 1, 1);
}

/**
 * @brief Defragment main area by backing up to reserved area.
 */
static void restart_map(struct rdnx_nvram_ctx *ctx)
{
    NVRAM_DEB("Restart map");
    transfer_main_to_reserve(ctx);
    prepare_area(ctx, RDNX_NVRAM_AREA_MAIN);
    transfer_reserve_to_main(ctx);
    prepare_area(ctx, RDNX_NVRAM_AREA_RESERVED);
}

/**
 * @brief Reserve next free block in main area.
 *
 * If all NVRAM is used, triggers defragmentation and backup.
 *
 * @return Offset of the first byte of the new block.
 */
rdnx_nvram_offset_t rdnx_nvram_get_next_block(struct rdnx_nvram_ctx *ctx)
{
    if ((ctx->next_block + RDNX_NVRAM_ENTRY_SIZE) > (ctx->interface.size - ctx->interface.reserved))
    {
        restart_map(ctx);
    }

    rdnx_nvram_offset_t block = ctx->next_block;
    ctx->next_block += RDNX_NVRAM_ENTRY_SIZE;

    NVRAM_DEB("Next block %d", (int)block);

    return block;
}
