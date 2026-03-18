/**
 * @file rdnx_types.h
 * @author Software development team
 * @brief Global types definitions
 * @version 1.0
 * @date 2024-09-09
 */

#ifndef RDNX_TYPES_H
#define RDNX_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

#define RDNX_TIMO_FOREVER (uint32_t) - 1
#define RDNX_TIMO_NOWAIT  (uint32_t)0

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief List of errors
 */
// clang-format off
typedef enum rdnx_err
{
    RDNX_ERR_OK = 0,
    RDNX_ERR_FAIL = 1,          /* Operation failed */
    RDNX_ERR_BADARG = 2,        /* Invalid argument */
    RDNX_ERR_INVAL = RDNX_ERR_BADARG,
    RDNX_ERR_BUSY = 3,          /* Device or resource busy */
    RDNX_ERR_NOMEM = 4,         /* Out of memory */
    RDNX_ERR_NORESOURCE = 5,    /* Resource not initialized */
    RDNX_ERR_TIMEOUT = 6,       /* Timer expired */
    RDNX_ERR_IO = 7,            /* I/O error */
    RDNX_ERR_NOTFOUND = 8,      /* Device or resource not found */
    RDNX_ERR_NOTSUP = 9,        /* Not supported */
    RDNX_ERR_MAX,
} rdnx_err_t;
// clang-format on

/**
 * @brief Possible system reset causes
 */
typedef enum rdnx_boot_reason
{
    /** @brief No reset source detected. Flags are already cleared */
    RDNX_BOOT_REASON_UNKNOWN = 0,
    /** @brief Low-power management reset occurs */
    RDNX_BOOT_REASON_LOW_POWER_RESET,
    /** @brief Window watchdog reset occurs */
    RDNX_BOOT_REASON_WINDOW_WATCHDOG_RESET,
    /** @brief Independent watchdog reset occurs */
    RDNX_BOOT_REASON_INDEPENDENT_WATCHDOG_RESET,
    /** @brief Software reset occurs */
    RDNX_BOOT_REASON_SOFTWARE_RESET,
    /** @brief POR/PDR reset occurs */
    RDNX_BOOT_REASON_POWER_ON_POWER_DOWN_RESET,
    /** @brief NRST pin is set to low by hardware reset, hardware reset */
    RDNX_BOOT_REASON_EXTERNAL_RESET_PIN_RESET,
    /** @brief BOR reset occurs */
    RDNX_BOOT_REASON_BROWNOUT_RESET,
} rdnx_boot_reason_t;

/**
 * @brief Structure describing a callback to be registered
 */
typedef struct rdnx_callback_desc
{
    /** Callback to be called when the event an event occurs. */
    void (*callback)(void *context);
    /** Parameter to be passed when the callback is called */
    void *ctx;
} rdnx_callback_desc_t;

#ifdef __cplusplus
}
#endif

#endif // RDNX_TYPES_H
