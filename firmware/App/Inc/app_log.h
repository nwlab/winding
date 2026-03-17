/**
 * @file app_log.h
 * @author Software development team
 * @brief Application logging implementation
 * @version 1.0
 * @date 2024-09-12
 */

#ifndef APP_LOG_H
#define APP_LOG_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h>
#include <stdint.h>

/* Core includes. */

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

#ifndef LOG_ENABLE
#define LOG_ENABLE 1
#endif

#if defined(LOG_ENABLE) && LOG_ENABLE == 1
// clang-format off
#define LOG_ERR(_fmt, ...) app_log_print("%08d ERR[%s:%d]:" _fmt "\n\r", \
                                         (int)yaa_systemtime(),         \
                                         __func__, __LINE__,             \
                                         ##__VA_ARGS__)

#define LOG_DEB(_fmt, ...)                                                    \
        app_log_print("%08d " _fmt "\n\r", yaa_systemtime(), ##__VA_ARGS__);

// clang-format on
#else
#define LOG_ERR(_fmt, ...)
#define LOG_DEB(_fmt, ...)
#endif

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Initialize logging subsystem.
 *
 * @return RDNX_ERR_OK
 */
uint8_t app_log_init(void);

/**
 * @brief Push message to log.
 */
void app_log_print(const char *format, ...);

#ifdef __cplusplus
}
#endif

#endif // APP_LOG_H