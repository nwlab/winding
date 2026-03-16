/**
 * @file yaa_time.c
 * @author Software development team
 * @brief Time functions implementation for STM32 platform
 * @version 1.0
 * @date 2024-09-12
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Core includes. */
#include <../Src/yaa_platform_ctx.h>

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

extern yaa_platform_ctx_t platform_ctx;

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint32_t yaa_systemtime(void)
{
    // Provides a tick value in millisecond
    if (platform_ctx.get_tick != NULL)
    {
        return platform_ctx.get_tick();
    }
    else
    {
        return HAL_GetTick();
    }
}

bool yaa_istimespent(uint32_t start, uint32_t pause)
{
    bool res = 0;

    uint32_t next = start + pause;

    if (next < yaa_systemtime())
    {
        res = 1;
    }
    return res;
}
