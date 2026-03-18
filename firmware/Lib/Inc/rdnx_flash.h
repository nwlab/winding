/**
 * @file rdnx_flash.h
 * @author Software development team
 * @brief Flash APIs
 * @version 1.0
 * @date 2024-09-09
 */

#ifndef RDNX_FLASH_H
#define RDNX_FLASH_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

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
 * @brief Status report for the functions.
 */
typedef enum rdnx_flash_status
{
    RDNX_FLASH_OK = 0x00u,             /**< The action was successful. */
    RDNX_FLASH_ERROR_SIZE = 0x01u,     /**< The binary is too big. */
    RDNX_FLASH_ERROR_WRITE = 0x02u,    /**< Writing failed. */
    RDNX_FLASH_ERROR_READBACK = 0x04u, /**< Writing was successful, but the
                                          content of the memory is wrong. */
    RDNX_FLASH_ERROR = 0xFFu           /**< Generic error. */
} rdnx_flash_status_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief  Locks the FLASH Program Erase Controller.
 * @param  None
 * @retval None
 */
void rdnx_flash_lock(void);

/**
 * @brief  Unlocks the FLASH Program Erase Controller.
 * @param  None
 * @retval None
 */
void rdnx_flash_unlock(void);

/**
 * @brief  Erases a FLASH page from specified address.
 * @param  address: The start address to be erased.
 * @param  end_address: The end address to be erased.
 * @retval FLASH Status: The returned value can be: RDNX_FLASH_OK,
 * RDNX_FLASH_ERROR.
 */
rdnx_flash_status_t rdnx_flash_erase(uint32_t address, uint32_t end_address);

/**
 * @brief  Reads data from FLASH memory.
 *
 * @param  address: Start address to read from.
 * @param  data: Pointer to destination buffer.
 * @param  length: Number of bytes to read.
 *
 * @retval FLASH Status:
 *         - RDNX_FLASH_OK on success
 *         - RDNX_FLASH_ERROR on failure
 */
rdnx_flash_status_t rdnx_flash_read(uint32_t address, uint8_t *data, uint32_t length);

/**
 * @brief  Programs a data at a specified address.
 * @param  address: specifies the address to be programmed.
 * @param  data: specifies the data to be programmed.
 * @param  length: specifies the length of data.
 * @retval FLASH Status: The returned value can be: RDNX_FLASH_OK,
 *   RDNX_FLASH_ERROR, RDNX_FLASH_ERROR_SIZE or RDNX_FLASH_ERROR_WRITE.
 */
rdnx_flash_status_t rdnx_flash_write(uint32_t address, uint32_t *data, uint32_t length);

/**
 * @brief  Get a sector for a specified address.
 * @param  address: specifies the address of the flash.
 * @retval Sector number for the requested address.
 */
uint32_t rdnx_flash_get_sector(uint32_t address);

/**
 * @brief Jump execution to the specific address
 */
void rdnx_flash_jump_to_app(uint32_t address);

#ifdef __cplusplus
}
#endif

#endif // RDNX_FLASH_H
