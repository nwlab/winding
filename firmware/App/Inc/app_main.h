/**
 * @file app_main.h
 * @author Software development team
 * @brief Application main FSM implementation
 * @version 1.0
 * @date 2024-09-12
 */

#ifndef APP_MAIN_H
#define APP_MAIN_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Application includes. */

#ifdef __cplusplus
extern "C" {
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/**
 * List of the running platform
 */
#define APP_PLATFORM_DEVEBOX        1
#define APP_PLATFORM_STM32_F4VE     2
#define APP_PLATFORM_RDNX_7037      3

/**
 * Define platform for peripherals. Set in CMakeLists.txt
 */
#ifndef APP_PLATFORM
    // #define APP_PLATFORM                APP_PLATFORM_DEVEBOX
    #define APP_PLATFORM                APP_PLATFORM_STM32_F4VE
    // #define APP_PLATFORM                APP_PLATFORM_RDNX_7037
#endif // APP_PLATFORM

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

uint8_t app_main_init(void);

uint8_t app_main_loop(void);

#ifdef __cplusplus
}
#endif

#endif // APP_MAIN_H
