/**
 * @file yaa_wdg.h
 * @author Software development team
 * @brief >Watchdog APIs
 * @version 1.0
 * @date 2024-09-24
 */
#ifndef YAA_WDG_H
#define YAA_WDG_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint8_t yaa_wdg_init(uint32_t reload_time);

uint8_t yaa_wdg_start(void);

uint8_t yaa_wdg_refresh(void);

uint8_t yaa_wdg_get_reset(void);

#ifdef __cplusplus
}
#endif

#endif // YAA_WDG_H
