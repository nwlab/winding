/**
 * @file app_log.c
 * @author Software development team
 * @brief Application logging implementation
 * @version 1.0
 * @date 2024-09-12
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

/* Library includes. */
#include <stm32f4xx_hal.h>
#include <stm32f4xx_hal_uart.h>

/* Core includes. */
#include "yaa_macro.h"
#include "rtt/yaa_rtt.h"

/* App includes. */
#include "app_log.h"
#include "main.h"

// clang-format off

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

 #define APP_LOG_BUFFER_SIZE    128

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

#if defined(USE_RTT) && (USE_RTT != 0)
    extern rdnx_rtt_handle_t rtt_handle;
#else
    #if (APP_PLATFORM == APP_PLATFORM_468396_018) || (APP_PLATFORM == APP_PLATFORM_STM32_F4VE)
        /**
        * Debug console is connected via USART1.
        *  USART3_TX PB10
        *  USART3_RX PB11
        */
        extern UART_HandleTypeDef huart3;
        #define UART_INSTANCE	(&huart3)
    #else
        /**
        * Debug console is connected via USART1.
        *  USART1_TX PA9
        *  USART1_RX PA10
        */
        extern UART_HandleTypeDef huart1;
        #define UART_INSTANCE	(&huart1)
    #endif // APP_PLATFORM
#endif // USE_RTT

/* ============================================================================
 * Private Variable Declarations
 * ==========================================================================*/

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint8_t app_log_init(void)
{
    app_log_print("\n\r== IO Monitor applicaiton ==\n\r");
    return YAA_ERR_OK;
}

void app_log_print(const char *format, ...)
{
    va_list args;
    char   buffer[APP_LOG_BUFFER_SIZE];

    va_start(args, format);
    vsnprintf(buffer, APP_LOG_BUFFER_SIZE - 1, format, args);
    va_end(args);

#if defined(USE_RTT) && (USE_RTT != 0)
    (void)rdnx_rtt_write(rtt_handle, buffer, strlen(buffer));
#else
    while (__HAL_UART_GET_FLAG(UART_INSTANCE, UART_FLAG_TC) != SET) {};
    if (HAL_UART_Transmit(UART_INSTANCE, (uint8_t *)buffer, strlen(buffer), 0xFFFF) != HAL_OK)
    {
        Error_Handler();
    }
#endif
}

__attribute__((optimize("O0"))) void hardfault_print(const char *string)
{
#if defined(USE_RTT) && (USE_RTT != 0)
    (void)rdnx_rtt_write(rtt_handle, string, strlen(string));
#else
    while (__HAL_UART_GET_FLAG(UART_INSTANCE, UART_FLAG_TC) != SET) {};
    HAL_UART_Transmit(UART_INSTANCE, (uint8_t *)string, strlen(string), 0xFFFF);
#endif
}

/**
 * @brief   This is needed by printf().
 * @param[in]   file
 * @param[in]   *data
 * @param[in]   len
 *
 * @return  len
 */
int _write(int file, char *data, int len)
{
    YAA_UNUSED(file);
#if defined(USE_RTT) && (USE_RTT != 0)
    (void)rdnx_rtt_write(rtt_handle, data, len);
#else
    while (__HAL_UART_GET_FLAG(UART_INSTANCE, UART_FLAG_TC) != SET) {};
    HAL_UART_Transmit(UART_INSTANCE, (uint8_t *)data, len, 0xFFFF);
#endif
    return len;
}

// clang-format on
