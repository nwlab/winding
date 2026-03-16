/**
 * @file yaa_clock.h
 * @author Software development team
 * @brief Software application layer APIs
 * @version 1.0
 * @date 2024-09-09
 */
#ifndef YAA_CLOCK_H
#define YAA_CLOCK_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Core includes. */
#include <yaa_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief  Clock speed enumeration
 */
typedef enum yaa_clock
{
    YAA_CLOCK_HSI,    /*!< High speed internal clock */
    YAA_CLOCK_HSE,    /*!< High speed external clock */
    YAA_CLOCK_SYSCLK, /*!< System core clock */
    YAA_CLOCK_PCLK1,  /*!< PCLK1 (APB1) peripheral clock */
    YAA_CLOCK_PCLK2,  /*!< PCLK2 (APB2) peripheral clock */
    YAA_CLOCK_HCLK    /*!< HCLK (AHB1) high speed clock */
} yaa_clock_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief  Gets specific clock speed value from STM32F4xx device
 * @param  clock: Clock type you want to know speed for.
 *                This parameter can be a value of @ref yaa_clock_t enumeration
 * @retval Clock speed in units of hertz
 */
uint32_t yaa_clock_getspeed(yaa_clock_t clock);

#ifdef __cplusplus
}
#endif

#endif // YAA_CLOCK_H
