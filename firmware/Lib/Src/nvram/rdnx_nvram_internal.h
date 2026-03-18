/**
 * @file    rdnx_nvram_internal.h
 * @author  Software development team
 * @brief   Internal definitions for RDNX NVRAM module.
 *
 * @details
 * This header contains private data structures used internally by the
 * RDNX NVRAM implementation. It is not intended for direct use by
 * application code.
 */

#ifndef RDNX_NVRAM_INTERNAL_H
#define RDNX_NVRAM_INTERNAL_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes */
#include <stdio.h>

/* Core includes */
#include "rdnx_nvram.h"

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macros for the W5500 driver. */
#if defined(DEBUG) && 0
    #define NVRAM_DEB(fmt, ...) printf("[NVRAM]:" fmt "\n\r", ##__VA_ARGS__)
    #define NVRAM_ERR(fmt, ...) printf("[NVRAM](ERROR)(%s:%d):" fmt "\n\r", __func__, __LINE__, ##__VA_ARGS__)
#else
    #define NVRAM_DEB(fmt, ...)   ((void)0)
    #define NVRAM_ERR(fmt, ...)   ((void)0)
#endif

/** @brief Offset of ERASE_STARTED flag inside metadata */
#define RDNX_NVRAM_OFFSET_ERASE_STARTED     (0)

/** @brief Offset of ERASE_FINISHED flag inside metadata */
#define RDNX_NVRAM_OFFSET_ERASE_FINISHED    (1)

/** @brief Number of bytes reserved for internal metadata in each area */
#define RDNX_NVRAM_METADATA_SIZE            (4)

/** @brief Magic value indicating erase procedure has started */
#define RDNX_NVRAM_ERASE_STARTED            (0xE2)

/** @brief Magic value indicating erase procedure finished successfully */
#define RDNX_NVRAM_ERASE_FINISHED           (0x3E)

/** @brief Size in bytes of a single key/value entry stored in memory */
#define RDNX_NVRAM_ENTRY_SIZE \
    (sizeof(rdnx_nvram_key_t) + sizeof(rdnx_nvram_value_t))

/** @brief Minimal storage size required for NVRAM (metadata + one entry) */
#define RDNX_NVRAM_MINIMAL_SIZE \
    (RDNX_NVRAM_ENTRY_SIZE + RDNX_NVRAM_METADATA_SIZE)

/** @brief Value of erased byte in NVRAM (typically 0xFF for Flash) */
#define RDNX_NVRAM_ERASED_BYTE_VALUE (0xFF)

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/** @enum rdnx_nvram_area_t
 *  @brief Storage area selector
 */
typedef enum
{
    RDNX_NVRAM_AREA_MAIN = 0, /**< Main storage area */
    RDNX_NVRAM_AREA_RESERVED  /**< Reserved (swap) area */
} rdnx_nvram_area_t;

/** @enum rdnx_nvram_state_t
 *  @brief NVRAM integrity state
 */
typedef enum
{
    RDNX_NVRAM_STATE_BLANK = 0,                /**< Fully erased memory */
    RDNX_NVRAM_STATE_CLEAN,                    /**< Last shutdown was clean */
    RDNX_NVRAM_STATE_MAIN_ERASE_INTERRUPTED,   /**< Main erase interrupted */
    RDNX_NVRAM_STATE_RESERVE_ERASE_INTERRUPTED /**< Reserved erase interrupted */
} rdnx_nvram_state_t;

/** @struct rdnx_nvram_entry
 *  @brief Runtime index entry mapping key to its latest offset in memory.
 */
typedef struct rdnx_nvram_entry
{
    rdnx_nvram_key_t    key;    /**< Key identifier */
    rdnx_nvram_offset_t offset; /**< Offset of latest value in memory */
} rdnx_nvram_entry_t;

static_assert(sizeof(rdnx_nvram_entry_t) == 8);

/**
 * @brief Internal NVRAM runtime context.
 *
 * @details
 * This structure holds the complete runtime state of the NVRAM module,
 * including:
 * - Hardware access interface
 * - Initialization state
 * - Cached entries stored in RAM
 * - Allocation metadata for the main NVRAM area
 *
 * The context is initialized during cold boot and is used by all
 * internal storage, recovery, and allocation routines.
 */
typedef struct rdnx_nvram_ctx
{
    /** @brief NVRAM hardware interface. */
    rdnx_nvram_config_t interface;

    /** @brief Initialization flag (1 = initialized, 0 = not initialized). */
    uint8_t initialized;

    /** @brief Internal cache storage for NVRAM entries. */
    rdnx_nvram_entry_t rdnx_nvram_entries[RDNX_NVRAM_MAX_ENTRIES];

    /** @brief Number of currently used entries in the cache. */
    size_t used_entries;

    /** @brief Offset to the next free NVRAM block in main area. */
    rdnx_nvram_offset_t next_block;
} rdnx_nvram_ctx_t;

#endif // RDNX_NVRAM_INTERNAL_H
