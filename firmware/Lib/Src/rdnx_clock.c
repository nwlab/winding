/**
 * @file rdnx_clock.c
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
#include <rdnx_clock.h>

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint32_t rdnx_clock_getspeed(rdnx_clock_t clock)
{
    uint32_t c = 0;

    /* Return clock speed */
    switch (clock)
    {
    case RDNX_CLOCK_HSI:
        c = HSI_VALUE;
        break;
    case RDNX_CLOCK_HSE:
        c = HSE_VALUE;
        break;
    case RDNX_CLOCK_HCLK:
        c = HAL_RCC_GetHCLKFreq();
        break;
    case RDNX_CLOCK_PCLK1:
        c = HAL_RCC_GetPCLK1Freq();
        break;
    case RDNX_CLOCK_PCLK2:
        c = HAL_RCC_GetPCLK2Freq();
        break;
    case RDNX_CLOCK_SYSCLK:
        c = HAL_RCC_GetSysClockFreq();
        break;
    default:
        break;
    }

    /* Return clock */
    return c;
}
