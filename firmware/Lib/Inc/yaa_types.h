/**
 * @file yaa_types.h
 * @author Software development team
 * @brief Global types definitions
 * @version 1.0
 * @date 2024-09-09
 */

#ifndef YAA_TYPES_H
#define YAA_TYPES_H

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

#define YAA_TIMO_FOREVER (uint32_t) - 1
#define YAA_TIMO_NOWAIT  (uint32_t)0

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief List of errors
 */
// clang-format off
typedef enum yaa_err
{
    YAA_ERR_OK = 0,
    YAA_ERR_FAIL = 1,          /* Operation failed */
    YAA_ERR_BADARG = 2,        /* Invalid argument */
    YAA_ERR_INVAL = YAA_ERR_BADARG,
    YAA_ERR_BUSY = 3,          /* Device or resource busy */
    YAA_ERR_NOMEM = 4,         /* Out of memory */
    YAA_ERR_NORESOURCE = 5,    /* Resource not initialized */
    YAA_ERR_TIMEOUT = 6,       /* Timer expired */
    YAA_ERR_IO = 7,            /* I/O error */
    YAA_ERR_NOTFOUND = 8,      /* Device or resource not found */
    YAA_ERR_NOTSUP = 9,        /* Not supported */
    YAA_ERR_MAX,
} yaa_err_t;
// clang-format on

/**
 * @brief Possible system reset causes
 */
typedef enum yaa_boot_reason
{
    /** @brief No reset source detected. Flags are already cleared */
    YAA_BOOT_REASON_UNKNOWN = 0,
    /** @brief Low-power management reset occurs */
    YAA_BOOT_REASON_LOW_POWER_RESET,
    /** @brief Window watchdog reset occurs */
    YAA_BOOT_REASON_WINDOW_WATCHDOG_RESET,
    /** @brief Independent watchdog reset occurs */
    YAA_BOOT_REASON_INDEPENDENT_WATCHDOG_RESET,
    /** @brief Software reset occurs */
    YAA_BOOT_REASON_SOFTWARE_RESET,
    /** @brief POR/PDR reset occurs */
    YAA_BOOT_REASON_POWER_ON_POWER_DOWN_RESET,
    /** @brief NRST pin is set to low by hardware reset, hardware reset */
    YAA_BOOT_REASON_EXTERNAL_RESET_PIN_RESET,
    /** @brief BOR reset occurs */
    YAA_BOOT_REASON_BROWNOUT_RESET,
} yaa_boot_reason_t;

/**
 * @brief Structure describing a callback to be registered
 */
typedef struct yaa_callback_desc
{
    /** Callback to be called when the event an event occurs. */
    void (*callback)(void *context);
    /** Parameter to be passed when the callback is called */
    void *ctx;
} yaa_callback_desc_t;

#ifdef __cplusplus
}
#endif

#endif // YAA_TYPES_H
