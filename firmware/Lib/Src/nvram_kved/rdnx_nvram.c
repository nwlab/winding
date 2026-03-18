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

/* Standard includes */
#include <stdio.h>

 /* Core includes. */
#include <rdnx_macro.h>
#include <rdnx_nvram.h>
#include <rdnx_sal.h>
#include <rdnx_types.h>

#include "kved.h"
#include "kved_flash.h"

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/** @brief Debug and Error log macros for the NVRAM driver. */
#ifdef DEBUG
    #define NVRAM_DEB(fmt, ...) printf("[NVRAM]:" fmt "\r\n", ##__VA_ARGS__)
    #define NVRAM_ERR(fmt, ...) printf("[NVRAM](ERROR)(%s:%d):" fmt "\r\n", __func__, __LINE__, ##__VA_ARGS__)
#else
    #define NVRAM_DEB(fmt, ...)   ((void)0)
    #define NVRAM_ERR(fmt, ...)   ((void)0)
#endif

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

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
} rdnx_nvram_ctx_t;

/* ============================================================================
 * Private Variable Definitions
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

    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *,
                                           rdnx_alloc(sizeof(struct rdnx_nvram_ctx)));
    if (ctx == NULL)
    {
        return RDNX_ERR_NOMEM;
    }

    ctx->interface = *interface;

    kved_init();

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

    const rdnx_nvram_offset_t main_capacity = 0;

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
 * - RDNX_ERR_OK        Success.
 * - RDNX_ERR_NOTFOUND  Key does not exist.
 * - RDNX_ERR_BADARG    NVRAM not initialized.
 */
rdnx_err_t rdnx_nvram_get_value(rdnx_nvram_handle_t handle, rdnx_nvram_key_t key, rdnx_nvram_value_t *value)
{
    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    kved_data_t kv1 = {
	    .key = {'A', key + '0', 0x00},
    };

    if (kved_data_read(&kv1))
    {
        NVRAM_DEB("Read Value: %d (0x%08X), key: %d", (int)kv1.value.u32, (int)kv1.value.u32, (int)key);
        *value = kv1.value.u32;
        return RDNX_ERR_OK;
    }

    return RDNX_ERR_NOTFOUND;
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
 * - RDNX_ERR_OK        Success.
 * - RDNX_ERR_BADARG    NVRAM not initialized.
 * - RDNX_ERR_FAIL      No free entries available in cache.
 */
rdnx_err_t rdnx_nvram_set_value(rdnx_nvram_handle_t handle, rdnx_nvram_key_t key, rdnx_nvram_value_t value)
{
    struct rdnx_nvram_ctx *ctx = RDNX_CAST(struct rdnx_nvram_ctx *, handle);
    if (ctx == NULL)
    {
        return RDNX_ERR_BADARG;
    }

    kved_data_t kv1 = {
        .type = KVED_DATA_TYPE_UINT32,
        .key = {'A', key + '0', 0x00},
        .value.u32 = value
    };

    if (kved_data_write(&kv1))
    {
        NVRAM_DEB("Written Value: %d (0x%08X), key: %d", (int)kv1.value.u32, (int)kv1.value.u32, (int)key);
        return RDNX_ERR_OK;
    }

    NVRAM_DEB("Fail to write");

    return RDNX_ERR_FAIL;
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

    return kved_total_entries_get();
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

    return kved_free_entries_get();
}

#ifdef __ARM_ARCH
#include "cmsis_gcc.h"
void kved_cpu_critical_section_enter(void)
{
    __disable_irq();
}

void kved_cpu_critical_section_leave(void)
{
    __enable_irq();
}
#else
void kved_cpu_critical_section_enter(void)
{
}

void kved_cpu_critical_section_leave(void)
{
}
#endif
