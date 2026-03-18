/**
 * @file    rdnx_nvram.h
 * @author  Software development team
 * @brief   RDNX Non-Volatile Key/Value Storage Interface
 * @version 1.0
 * @date    2026-02-16
 *
 * @details
 * This module provides a lightweight key/value storage layer for
 * non-volatile memory (Flash, EEPROM, external NVRAM, etc.).
 *
 * Features:
 *  - Log-structured storage for power-loss resilience
 *  - Wear leveling via main/reserved areas
 *  - Minimal RAM usage
 *  - Deterministic behavior
 *
 * Users must provide low-level read/write/erase functions through
 * @ref rdnx_nvram_config_t.
 *
 * The library operates on logical addresses starting from zero.
 */

#ifndef RDNX_NVRAM_H
#define RDNX_NVRAM_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes */
#include <stdint.h>
#include <stddef.h>

/* Core includes */
#include <rdnx_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/** @brief Maximum number of unique keys stored in RAM index */
#ifndef RDNX_NVRAM_MAX_ENTRIES
#define RDNX_NVRAM_MAX_ENTRIES       (20)
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/** @typedef rdnx_nvram_key_t
 *  @brief Type representing a key identifier.
 *  Adjust this type to reduce storage usage if a large key space is not required.
 */
typedef uint16_t rdnx_nvram_key_t;

// static_assert(sizeof(rdnx_nvram_key_t) == 4);

/** @typedef rdnx_nvram_value_t
 *  @brief Type representing a stored value.
 */
typedef int32_t rdnx_nvram_value_t;

static_assert(sizeof(rdnx_nvram_value_t) == 4);

/** @typedef rdnx_nvram_offset_t
 *  @brief Logical address type inside NVRAM.
 *  Can be reduced if memory size is small to save RAM.
 */
typedef uint32_t rdnx_nvram_offset_t;

/** @typedef rdnx_nvram_erase_fn
 *  @brief Prototype for erase functions.
 *  @return 0 on success, negative value on error.
 */
typedef rdnx_err_t (*rdnx_nvram_erase_fn)(void);

/** @struct rdnx_nvram_interface
 *  @brief Low-level storage interface provided by the user.
 *
 * The library operates on logical addresses starting from 0.
 * erase_main() and erase_reserve() must only erase their respective areas.
 */
typedef struct rdnx_nvram_interface
{
    /** @brief Read data from storage.
     *  @param[out] data  Destination buffer
     *  @param[in]  start Logical start offset
     *  @param[in]  size  Number of bytes to read
     *  @return 0 on success, negative on error
     */
    rdnx_err_t (*read)(uint8_t *data, rdnx_nvram_offset_t start, rdnx_nvram_offset_t size);

    /** @brief Write data to storage.
     *  @param[in] data  Source buffer
     *  @param[in] start Logical start offset
     *  @param[in] size  Number of bytes to write
     *  @return 0 on success, negative on error
     */
    rdnx_err_t (*write)(const uint8_t *data, rdnx_nvram_offset_t start, rdnx_nvram_offset_t size);

    /** @brief Erase main storage area */
    rdnx_err_t (*erase_main)(void);

    /** @brief Erase reserved storage area */
    rdnx_err_t (*erase_reserve)(void);

    /** @brief Total storage size */
    rdnx_nvram_offset_t size;

    /** @brief Reserved area size */
    rdnx_nvram_offset_t reserved;
} rdnx_nvram_config_t;

 /** @brief Handle to an initialized NVRAM instance */
typedef struct rdnx_nvram_ctx *rdnx_nvram_handle_t;

/* ============================================================================
 * Public Function Prototypes
 * ==========================================================================*/

/**
 * @brief Initialize the NVRAM subsystem.
 *
 * Performs memory validation, recovery if needed, and builds the runtime key index.
 *
 * @param[in]  nvram_interface  Pointer to user-provided storage interface
 * @param[out] handle           Pointer to NVRAM handle
 * @return RDNX_OK on success, error code otherwise
 */
rdnx_err_t rdnx_nvram_init(const rdnx_nvram_config_t *nvram_interface,
                           rdnx_nvram_handle_t *handle);

/**
 * @brief Get maximum number of entries that can fit in main NVRAM area.
 *
 * Calculates usable storage space excluding metadata and returns the total
 * number of entry-sized blocks available.
 *
 * @param[in] handle  Initialized NVRAM handle
 * @return Maximum number of storable entries
 */
rdnx_nvram_offset_t rdnx_nvram_capacity(rdnx_nvram_handle_t handle);

/**
 * @brief Get number of stored unique keys.
 *
 * @param[in] handle  Initialized NVRAM handle
 * @return Number of active entries
 */
size_t rdnx_nvram_get_entries_number(rdnx_nvram_handle_t handle);

/**
 * @brief Get number of free key slots available.
 *
 * @param[in] handle  Initialized NVRAM handle
 * @return Remaining free entries
 */
size_t rdnx_nvram_get_free_entries(rdnx_nvram_handle_t handle);

/**
 * @brief Retrieve value by key.
 *
 * @param[in]  handle  Initialized NVRAM handle
 * @param[in]  key     Key identifier
 * @param[out] value   Pointer to store retrieved value
 * @return RDNX_OK on success, RDNX_ERR_NOTFOUND if key does not exist, other error codes on failure
 */
rdnx_err_t rdnx_nvram_get_value(rdnx_nvram_handle_t handle,
                                rdnx_nvram_key_t key,
                                rdnx_nvram_value_t *value);

/**
 * @brief Store value by key.
 *
 * If key already exists, a new entry is appended and the old entry becomes obsolete.
 *
 * @param[in] handle  Initialized NVRAM handle
 * @param[in] key     Key identifier
 * @param[in] value   Value to store
 * @return RDNX_OK on success, error code on failure
 */
rdnx_err_t rdnx_nvram_set_value(rdnx_nvram_handle_t handle,
                                rdnx_nvram_key_t key,
                                rdnx_nvram_value_t value);

#ifdef __cplusplus
}
#endif

#endif /* RDNX_NVRAM_H */
