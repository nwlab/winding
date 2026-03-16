/**
 * @file    ds2430.h
 * @author  Software development team
 * @brief   Driver for DS2430 device, 256 bit EEPROM
 * @version 1.0
 * @date    2024-10-30
 */

#ifndef DS2430_H
#define DS2430_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Core includes. */
#include <onewire/ds2482.h>
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

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

yaa_err_t ds2430_writeMemory(ds2482_handle_t handle, const uint8_t *source, uint8_t length, uint8_t position);

yaa_err_t ds2430_readMemory(ds2482_handle_t handle, uint8_t *destination, uint16_t length, uint16_t position);

#ifdef __cplusplus
}
#endif

#endif // DS2430_H
