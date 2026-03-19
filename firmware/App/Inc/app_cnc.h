/**
 * @file app_cnc.h
 * @author Software development team
 * @brief Application CNC implementation
 * @version 1.0
 * @date 2024-09-12
 */

#ifndef APP_CNC_H
#define APP_CNC_H

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


/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

uint8_t app_cnc_init(void);

uint8_t app_cnc_loop(void);

#ifdef __cplusplus
}
#endif

#endif // APP_CNC_H
