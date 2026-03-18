/**
 * @file rdnx_bksram.h
 * @author Software development team
 * @brief Backup SRAM APIs
 * @version 1.0
 * @date 2024-10-02
 */

#ifndef RDNX_BKSRAM_H
#define RDNX_BKSRAM_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Core includes. */
#include <rdnx_types.h>

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

/**
 * @brief  Initializes backup SRAM peripheral
 * @note   This function initializes and enable backup SRAM domain.
 *
 * @note   With this settings you have access to save/get from locations
 * where SRAM is.
 * @param  None
 * @retval #RDNX_ERR_OK on success
 */
rdnx_err_t rdnx_bkpsram_init(void);

/**
 * @brief  Enable write access to the backup registers.  Backup interface
 * must be initialized for subsequent register writes to work.
 * @see rdnx_bkpsram_init()
 */
void rdnx_bkpsram_enable_writes(void);

/**
 * @brief  Disable write access to the backup registers.
 */
void rdnx_bkpsram_disable_writes(void);

/**
 * @brief  Gets memory size for internal backup SRAM
 * @param  None
 * @retval Memory size in bytes
 */
uint32_t rdnx_bkpsram_get_size(void);

/**
 * @brief  Gets memory address for internal backup SRAM
 * @param  None
 * @retval Pointer to backup memory area
 */
void *rdnx_bkpsram_get_address(void);

#ifdef __cplusplus
}
#endif

#endif // RDNX_BKSRAM_H
