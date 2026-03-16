/**
 * @file yaa_bksram.h
 * @author Software development team
 * @brief Backup SRAM APIs
 * @version 1.0
 * @date 2024-10-02
 */

#ifndef YAA_BKSRAM_H
#define YAA_BKSRAM_H

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
 * @retval #YAA_ERR_OK on success
 */
yaa_err_t yaa_bkpsram_init(void);

/**
 * @brief  Enable write access to the backup registers.  Backup interface
 * must be initialized for subsequent register writes to work.
 * @see yaa_bkpsram_init()
 */
void yaa_bkpsram_enable_writes(void);

/**
 * @brief  Disable write access to the backup registers.
 */
void yaa_bkpsram_disable_writes(void);

/**
 * @brief  Gets memory size for internal backup SRAM
 * @param  None
 * @retval Memory size in bytes
 */
uint32_t yaa_bkpsram_get_size(void);

/**
 * @brief  Gets memory address for internal backup SRAM
 * @param  None
 * @retval Pointer to backup memory area
 */
void *yaa_bkpsram_get_address(void);

#ifdef __cplusplus
}
#endif

#endif // YAA_BKSRAM_H
