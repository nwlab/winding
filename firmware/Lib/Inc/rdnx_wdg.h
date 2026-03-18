/**
 * @file rdnx_wdg.h
 * @author Software development team
 * @brief >Watchdog APIs
 * @version 1.0
 * @date 2024-09-24
 */
#ifndef RDNX_WDG_H
#define RDNX_WDG_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

uint8_t rdnx_wdg_init(uint32_t reload_time);

uint8_t rdnx_wdg_start(void);

uint8_t rdnx_wdg_refresh(void);

uint8_t rdnx_wdg_get_reset(void);

#ifdef __cplusplus
}
#endif

#endif // RDNX_WDG_H
