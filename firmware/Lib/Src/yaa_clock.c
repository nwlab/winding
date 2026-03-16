/**
 * @file yaa_clock.c
 * @author Software development team
 * @brief Clock functions implementation for STM32 platform
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
#include <yaa_clock.h>

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint32_t yaa_clock_getspeed(yaa_clock_t clock)
{
    uint32_t c = 0;

    /* Return clock speed */
    switch (clock)
    {
    case YAA_CLOCK_HSI:
        c = HSI_VALUE;
        break;
    case YAA_CLOCK_HSE:
        c = HSE_VALUE;
        break;
    case YAA_CLOCK_HCLK:
        c = HAL_RCC_GetHCLKFreq();
        break;
    case YAA_CLOCK_PCLK1:
        c = HAL_RCC_GetPCLK1Freq();
        break;
    case YAA_CLOCK_PCLK2:
        c = HAL_RCC_GetPCLK2Freq();
        break;
    case YAA_CLOCK_SYSCLK:
        c = HAL_RCC_GetSysClockFreq();
        break;
    default:
        break;
    }

    /* Return clock */
    return c;
}
