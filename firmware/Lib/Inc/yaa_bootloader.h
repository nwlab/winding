/**
 * @file yaa_bootloader.h
 * @author Software development team
 * @brief Bootloader header
 *        This file defines the bootloader configuration parameters, function prototypes, and other required macros
 *        and definitions needed to initialize and manage the bootloader process for the system.
 * @version 1.0
 * @date 2024-09-30
 */

#ifndef YAA_BOOTLOADER_H
#define YAA_BOOTLOADER_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

// clang-format off
/*
    Flash memory organisation

    0x807FFFF -> +-------------+
                 |             | \
                 |             |  \
                 |             |   |
                 |             |   |
                 | 0x8010000   |   |
                 |    to       |   |
                 | 0x807FFFF   |   |- Contain the application software
                 |   448 KB    |   |
                 | application |   |
                 |             |   |
                 |             |   |
                 |             |  /
                 |             | /
    0x8010000 -> +-------------+
                 | 0x8008000   | \
                 |    to       |  \
                 | 0x800FFFF   |   |- Reserved for NVRAM
                 |   32 KB     |  /   2 sector of 16Kb
                 | data        | /
    0x8008000 -> +-------------+
                 |             | \
                 | 0x8000000   |  \
                 |    to       |   |
                 | 0x8007FFF   |   |- Contain bootloader software
                 |   32 KB     |   |  2 sectors of 16Kb
                 | bootloader  |  /
                 |             | /
    0x8000000 -> +-------------+
*/
// clang-format on

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/* Bootloader Configuration */
/**
 * @brief Automatically set vector table location before launching the application.
 * @note Set to 1 to enable automatic vector table location setting.
 */
#define SET_VECTOR_TABLE 1

/**
 * @brief Clear reset flags when bootloader is invoked.
 * @note If enabled: bootloader clears reset flags. (This occurs only when OBL RST
 *       flag is active.) If disabled: bootloader does not clear reset flags, not
 *       even when OBL RST is active.
 */
#define CLEAR_RESET_FLAGS 1

/* Bootloader address definitions */
/**
 * @brief Start address of bootloader in flash memory.
 */
#define YAA_BL_ADDRESS (uint32_t)0x08000000

/**
 * @brief Size of the bootloader section (32KB).
 */
#define YAA_BL_SIZE (uint32_t)0x00008000

/* Application space address definitions */
/**
 * @brief Start address of the application space in flash.
 * @note Default address is 0x08010000 unless overridden.
 */
#ifndef YAA_APP_ADDRESS
#define YAA_APP_ADDRESS (uint32_t)0x08010000
#endif

/**
 * @brief End address of the application space.
 * @note Computed based on bootloader size.
 */
#define YAA_END_ADDRESS (YAA_BL_ADDRESS + (yaa_bootloader_flash_size() * 1024) - 1)

/**
 * @brief Start address for application NVRAM.
 * @note Default address is 0x08008000 unless overridden.
 */
#ifndef YAA_INFO_ADDRESS
#define YAA_INFO_ADDRESS (uint32_t)0x08008000
#endif

/**
 * @brief Address of the system memory (ST Bootloader).
 */
#define YAA_SYSMEM_ADDRESS (uint32_t)0x1FFFD800

/* MCU RAM size and flash size */
/**
 * @brief Total RAM size of the MCU used for checking whether the flash contains a valid application.
 */
#define YAA_RAM_SIZE   (uint32_t)0x0020000 /* 128 kB */

/**
 * @brief Total flash size available in the MCU.
 */
#define YAA_FLASH_SIZE (uint32_t)((FLASH_END) - (FLASH_BASE))

/**
 * @brief Size of the application in DWORD (32 bits or 4 bytes).
 */
#define YAA_APP_SIZE (uint32_t)(((YAA_END_ADDRESS - YAA_APP_ADDRESS) + 1))

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Enumeration for bootloader error codes.
 *        This list contains various error types the bootloader can return.
 */
enum
{
    YAA_BL_OK = 0, /**< Bootloader operation successful */
    YAA_BL_NO_APP, /**< No application found in flash */
    YAA_BL_SIZE_ERROR, /**< Invalid application size */
    YAA_BL_CHKS_ERROR, /**< Checksum error */
    YAA_BL_ERASE_ERROR, /**< Flash erase error */
    YAA_BL_WRITE_ERROR, /**< Flash write error */
    YAA_BL_OBP_ERROR /**< Operation block protection error */
};

/**
 * @brief Enumeration for flash protection types.
 *        These flags define various protection mechanisms for flash sectors.
 */
enum
{
    YAA_BL_PROTECTION_NONE = 0, /**< No protection */
    YAA_BL_PROTECTION_WRP = 0x1, /**< Write protection */
    YAA_BL_PROTECTION_RDP = 0x2, /**< Read-out protection */
    YAA_BL_PROTECTION_PCROP = 0x4, /**< Peripheral control read-out protection */
};

/**
 * @brief Structure containing bootloader configuration parameters.
 */
typedef struct yaa_bootloader_param
{
    uintptr_t app_start; /**< Start address of the application in flash */
    size_t app_size; /**< Size of the application */
    size_t ram_size; /**< Size of the RAM */
} yaa_bootloader_param_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Initializes the bootloader with provided parameters.
 *
 * @param param Pointer to a structure containing bootloader configuration parameters.
 * @return Status code of the initialization process.
 */
uint8_t yaa_bootloader_init(const yaa_bootloader_param_t *param);

/**
 * @brief Erases a section of flash memory.
 *
 * @param size The size of the flash section to be erased.
 * @return Status code indicating the result of the operation.
 */
uint8_t yaa_bootloader_erase_flash(size_t size);

/**
 * @brief Erases all flash memory.
 *
 * @return Status code indicating the result of the operation.
 */
uint8_t yaa_bootloader_erase_flash_all(void);

/**
 * @brief Begins the flash writing process.
 * This function must be called before writing any data to flash.
 */
void yaa_bootloader_flash_begin(void);

/**
 * @brief Writes a 32-bit word to flash memory.
 *
 * @param data The 32-bit data to be written.
 * @param index Pointer to the index of the next flash write position.
 * @return Status code indicating the result of the operation.
 */
uint8_t yaa_bootloader_flash_next_32(uint32_t data, uint32_t *index);

/**
 * @brief Writes a 64-bit data to flash memory.
 *
 * @param data The 64-bit data to be written.
 * @return Status code indicating the result of the operation.
 */
uint8_t yaa_bootloader_flash_next(uint64_t data);

/**
 * @brief Ends the flash writing process.
 * This function must be called after all data has been written to flash.
 */
void yaa_bootloader_flash_end(void);

/**
 * @brief Returns the size of the flash memory.
 *
 * @return The size of the flash memory in bytes.
 */
size_t yaa_bootloader_flash_size(void);

/**
 * @brief Verify that the MCU flash size matches the firmware linker configuration.
 *
 * This function reads the actual flash size from the MCU system memory
 * (FLASHSIZE register) and compares it with the flash size defined by
 * the linker script.
 *
 * If a mismatch is detected, a warning is printed. Optionally, the system
 * may be halted or reset depending on the implementation.
 *
 * This check helps prevent running firmware built for a different
 * microcontroller flash configuration.
 *
 * @note Should be called early during bootloader initialization.
 *
 * @warning If the firmware is built for a larger flash than physically
 * available on the device, executing beyond valid flash memory may
 * cause undefined behavior or HardFault.
 */
void yaa_bootloader_check_flash_size(void);

/**
 * @brief Configures flash protection settings.
 *
 * @param protection The desired protection settings.
 * @return Status code indicating the result of the operation.
 */
uint8_t yaa_bootloader_config_protection(uint32_t protection);

/**
 * @brief Verifies that the size of the application is correct.
 *
 * @param appsize The size of the application to verify.
 * @return Status code indicating the result of the size check.
 */
uint8_t yaa_bootloader_check_size(size_t appsize);

/**
 * @brief Returns the bootloader checksum.
 *
 * @return The checksum value of the bootloader code.
 */
uint32_t yaa_bootloader_get_boot_checksum(void);

/**
 * @brief Returns the general checksum of the system.
 *
 * @return The general checksum of the system.
 */
uint32_t yaa_bootloader_get_checksum(void);

/**
 * @brief Returns the checksum of the application.
 *
 * @return The checksum of the application stored in flash.
 */
uint32_t yaa_bootloader_get_app_checksum(void);

/**
 * @brief Returns the size of the application.
 *
 * @return The size of the application in bytes.
 */
size_t yaa_bootloader_get_app_size(void);

/**
 * @brief Updates and returns the new application size after update.
 *
 * @return The updated application size.
 */
size_t yaa_bootloader_update_app_size(void);

/**
 * @brief Erases the application info section from flash memory.
 *
 * @return Status code indicating the result of the operation.
 */
uint8_t yaa_bootloader_erase_info(void);

/**
 * @brief Saves application info to flash memory.
 *
 * @return Status code indicating the result of the operation.
 */
uint8_t yaa_bootloader_save_info(void);

/**
 * @brief Checks if an application is present in the flash memory.
 *
 * @return Status code indicating whether an application is found.
 */
uint8_t yaa_bootloader_check_for_application(void);

/**
 * @brief Jumps to the application in flash memory.
 */
void yaa_bootloader_jump_to_app(void);

/**
 * @brief Jumps to the system memory (ST Bootloader).
 */
void yaa_bootloader_jump_to_sysmem(void);

/**
 * @brief Returns the version of the bootloader.
 *
 * @return The bootloader version.
 */
uint32_t yaa_bootloader_get_version(void);

/**
 * @brief Returns the number of times the bootloader has been used to flash the system.
 *
 * @return The count of flash operations.
 */
uint32_t yaa_bootloader_get_flashed_count(void);

#ifdef __cplusplus
}
#endif

#endif // YAA_BOOTLOADER_H
