/**
 * @file app_main.c
 * @author Software development team
 * @brief Application main FSM implementation
 * @version 1.0
 * @date 2024-09-12
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <string.h>     // for memcpy
#include <stdio.h>

/* Core includes. */
#include <rdnx_types.h>
#include <rdnx_macro.h>
#include <rdnx_platform.h>
#include <rdnx_bootloader.h>

/* App includes. */
#include "app_main.h"
#include "app_log.h"
#include "app_cnc.h"
#include "main.h"

#include "StepperHAL_Config.h"
#include "StepperHAL_STM32F4x1.h"

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Private Variable Definitions
 * ==========================================================================*/

static const rdnx_platform_params_t app_platform =
{
    APP_PLATFORM,
    "Winding",
    NULL, // Use DWT
    HAL_GetTick
};

StepperHAL motor1(MOTOR1_PARAMS);

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

extern int illegal_instruction_execution(void);

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint8_t app_main_init(void)
{
    uint8_t res = 0;

    // Init platform
    (void)rdnx_platform_init(&app_platform);

    printf("Git Revision : %s\n\r", GIT_COMMIT_HASH);
    printf("Flash Kb     : %d\n\r", (int)rdnx_bootloader_flash_size());

    res = app_log_init();
    if (res != 0)
    {
        // TODO Print error
        return res;
    }

    res = app_cnc_init();
    if (res != 0)
    {
        // TODO Print error
        return res;
    }

    return 0;
}

uint8_t app_main_loop(void)
{
    uint8_t res = 0;

    res = app_cnc_loop();
    if (res != 0)
    {
        // TODO Print error
        return res;
    }

    return 0;
}
