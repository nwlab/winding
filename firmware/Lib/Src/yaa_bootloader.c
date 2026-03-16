/**
 * @file yaa_event.c
 * @brief  Bootloader implementation
 *         This file contains the functions of the bootloader. The bootloader
 *         implementation uses the official HAL library of ST.*
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdio.h>

/* Library includes. */
#include "stm32f4xx_hal.h"

/* Core includes. */
#include <yaa_bootloader.h>
#include <yaa_flash.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

#define YAA_BOOTLOADER_VERSION_MAJOR 1
#define YAA_BOOTLOADER_VERSION_MINOR 1
#define YAA_BOOTLOADER_VERSION_PATCH 0
#define YAA_BOOTLOADER_VERSION_RC    (0x12)

#define YAA_BOOTLOADER_MAGIC (0x584e4452)

#define ADDR_IS_APP(addr) (((YAA_APP_ADDRESS) <= (addr)) && ((addr) < (YAA_END_ADDRESS)))

#ifndef __VECTOR_TABLE_SIZE
extern uint32_t _isr_vector_size[];
#define __VECTOR_TABLE_SIZE ((uint32_t)_isr_vector_size)
#endif

extern uint32_t __flash_start__;
extern uint32_t __flash_end__;

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

#ifdef HAL_CRC_MODULE_ENABLED
extern CRC_HandleTypeDef hcrc;
#endif

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

typedef union __attribute__((packed)) yaa_image_header
{
    struct
    {
        uint32_t image_magic;
        uint32_t image_version;
        uint32_t image_size;
        uint32_t crc;
        uint32_t flash_count;
    };
    uint32_t data[8];
} yaa_image_header_t;

typedef void (*pFunction)(void);

/* ============================================================================
 * Private Variable Declarations
 * ==========================================================================*/

static uintptr_t flash_ptr = YAA_APP_ADDRESS;

static yaa_bootloader_param_t bootloader_param = {
    .app_start = YAA_APP_ADDRESS,
    .app_size = 0, // Calculate based on the flash size
    .ram_size = YAA_RAM_SIZE,
};

/* ============================================================================
 * Private Function Declaration
 * ==========================================================================*/

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/**
 * @brief Initialize bootloader and flash
 */
uint8_t yaa_bootloader_init(const yaa_bootloader_param_t *param)
{
    uint8_t res = YAA_ERR_OK;
    if (param != NULL)
    {
        bootloader_param.app_start = param->app_start;
        bootloader_param.app_size = param->app_size;
        bootloader_param.ram_size = param->ram_size;
    }

    yaa_bootloader_update_app_size();

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    //__HAL_RCC_FLASH_CLK_ENABLE();

#ifdef HAL_CRC_MODULE_ENABLED
    CRC_HandleTypeDef CrcHandle;

    __HAL_RCC_CRC_CLK_ENABLE();
    CrcHandle.Instance = CRC;
    if (HAL_CRC_Init(&CrcHandle) != HAL_OK)
    {
        return YAA_ERR_FAIL;
    }
#endif

    /* Clear flash flags */
    HAL_FLASH_Unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_WRPERR | FLASH_SR_PGAERR | FLASH_SR_PGPERR | FLASH_SR_PGSERR);

    yaa_flash_lock();

    return res;
}

/**
 * @brief Erase flash for application.
 *        Start address is bootloader_param.app_start
 */
uint8_t yaa_bootloader_erase_flash(size_t size)
{
    uint32_t SectorError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;
    HAL_StatusTypeDef status = HAL_OK;

    yaa_flash_unlock();
    __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_EOP | FLASH_FLAG_OPERR | FLASH_FLAG_WRPERR | FLASH_FLAG_PGAERR |
                           FLASH_FLAG_PGPERR | FLASH_FLAG_PGSERR);

    if (status == HAL_OK)
    {
        EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
        EraseInitStruct.Sector = yaa_flash_get_sector(bootloader_param.app_start);
        EraseInitStruct.NbSectors = (yaa_flash_get_sector(bootloader_param.app_start + size) -
                                     yaa_flash_get_sector(bootloader_param.app_start)) +
                                    1;
        if (EraseInitStruct.NbSectors > FLASH_SECTOR_TOTAL)
        {
            EraseInitStruct.NbSectors = FLASH_SECTOR_TOTAL;
        }
        EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;
        status = HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError);
    }

    yaa_flash_lock();

    return (status == HAL_OK) ? YAA_BL_OK : YAA_BL_ERASE_ERROR;
}

/**
 * @brief Erase flash
 */
uint8_t yaa_bootloader_erase_flash_all(void)
{
    uint32_t size = YAA_END_ADDRESS - bootloader_param.app_start;
    return yaa_bootloader_erase_flash(size);
}

/**
 * @brief Flash Begin
 */
void yaa_bootloader_flash_begin(void)
{
    /* Reset flash destination address */
    flash_ptr = bootloader_param.app_start;

    /* Unlock flash */
    yaa_flash_unlock();
}

/**
 * @brief Program 32bit data into flash
 */
uint8_t yaa_bootloader_flash_next_32(uint32_t data, uint32_t *index)
{

    if (!ADDR_IS_APP(flash_ptr))
    {
        return YAA_BL_WRITE_ERROR;
    }

    __HAL_FLASH_CLEAR_FLAG(
          FLASH_FLAG_EOP    |
          FLASH_FLAG_OPERR  |
          FLASH_FLAG_WRPERR |
          FLASH_FLAG_PGAERR |
          FLASH_FLAG_PGPERR |
          FLASH_FLAG_PGSERR);

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, flash_ptr, (uint64_t)data) == HAL_OK)
    {
        /* Check the written value */
        if (*(uint32_t *)flash_ptr != data)
        {
            /* Flash content doesn't match source content */
            return YAA_BL_WRITE_ERROR;
        }
        /* Increment Flash destination address */
        flash_ptr += 4;
        *index = (flash_ptr - bootloader_param.app_start);
    }
    else
    {
        /* Error occurred while writing data into Flash */
        return YAA_BL_WRITE_ERROR;
    }

    return YAA_BL_OK;
}

/**
 * @brief Program 64bit data into flash
 */
uint8_t yaa_bootloader_flash_next(uint64_t data)
{
    if (!ADDR_IS_APP(flash_ptr))
    {
        return YAA_BL_WRITE_ERROR;
    }

    if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_DOUBLEWORD, flash_ptr, data) == HAL_OK)
    {
        /* Check the written value */
        if (*(uint64_t *)flash_ptr != data)
        {
            /* Flash content doesn't match source content */
            return YAA_BL_WRITE_ERROR;
        }
        /* Increment Flash destination address */
        flash_ptr += 8;
    }
    else
    {
        /* Error occurred while writing data into Flash */
        return YAA_BL_WRITE_ERROR;
    }

    return YAA_BL_OK;
}

/**
 * @brief Finish flash programming
 */
void yaa_bootloader_flash_end(void)
{
    /* Lock flash */
    yaa_flash_lock();
}

/**
 * @brief Get flash size in Kbytes
 */
size_t yaa_bootloader_flash_size(void)
{
    uint16_t flash_size_kb = *(const uint16_t *)FLASHSIZE_BASE;
    return (size_t)flash_size_kb;
}

/**
 * @brief Verify that the MCU flash size matches the firmware linker configuration.
 */
void yaa_bootloader_check_flash_size(void)
{
    uint16_t real_flash_kb = *(const uint16_t *)FLASHSIZE_BASE;

    size_t linker_flash_bytes =
        (size_t)&__flash_end__ - (size_t)&__flash_start__;

    size_t linker_flash_kb = linker_flash_bytes / 1024;

    if (real_flash_kb != linker_flash_kb)
    {
        printf("\n\r[WARNING] Flash size mismatch!\n\r");
        printf("  Linker:  %u KB\n\r", (unsigned)linker_flash_kb);
        printf("  Device:  %u KB\n\r\n\r", real_flash_kb);
    }
}

/**
 * @brief Configure flash write protection
 */
uint8_t yaa_bootloader_config_protection(uint32_t protection)
{
    FLASH_OBProgramInitTypeDef OBStruct = { 0 };
    HAL_StatusTypeDef status = HAL_ERROR;

    status = HAL_FLASH_Unlock();
    status |= HAL_FLASH_OB_Unlock();

    /* Bank 1 */

    OBStruct.OptionType = OPTIONBYTE_RDP;

    if (protection & YAA_BL_PROTECTION_RDP)
    {
        /* Enable RDP protection */
        OBStruct.RDPLevel = OB_RDP_LEVEL_1;
    }
    else
    {
        /* Remove RDP protection */
        OBStruct.RDPLevel = OB_RDP_LEVEL_0;
    }
    status |= HAL_FLASHEx_OBProgram(&OBStruct);

    if (status == HAL_OK)
    {
        /* Loading Flash Option Bytes - this generates a system reset. */
        status |= HAL_FLASH_OB_Launch();
    }

    status |= HAL_FLASH_OB_Lock();
    yaa_flash_lock();

    return (status == HAL_OK) ? YAA_BL_OK : YAA_BL_OBP_ERROR;
}

/**
 * @brief Check if application fits into user flash
 */
uint8_t yaa_bootloader_check_size(size_t appsize)
{
    return ((FLASH_BASE + YAA_FLASH_SIZE - bootloader_param.app_start) >= appsize) ? YAA_BL_OK : YAA_BL_SIZE_ERROR;
}

/**
 * @brief Verify checksum of bootloader
 */
uint32_t yaa_bootloader_get_boot_checksum(void)
{
    uint32_t calculatedCrc = 0;
#ifdef HAL_CRC_MODULE_ENABLED
    calculatedCrc = HAL_CRC_Calculate(&hcrc, (uint32_t *)YAA_BL_ADDRESS, YAA_BL_SIZE);
#endif
    return calculatedCrc;
}

/**
 * @brief Verify checksum of application located in flash
 */
uint32_t yaa_bootloader_get_checksum(void)
{
    uint32_t calculatedCrc = 0;
#ifdef HAL_CRC_MODULE_ENABLED
    calculatedCrc =
        HAL_CRC_Calculate(&hcrc, (uint32_t *)bootloader_param.app_start, bootloader_param.app_size / sizeof(uint32_t));

    __HAL_RCC_CRC_FORCE_RESET();
    __HAL_RCC_CRC_RELEASE_RESET();
#endif
    return calculatedCrc;
}

/**
 * @brief Get signed binary checksum of application located in flash
 */
uint32_t yaa_bootloader_get_app_checksum(void)
{
    uint32_t calculatedCrc = 0;
    uint32_t app_end = (*(__IO uint32_t *)(bootloader_param.app_start + __VECTOR_TABLE_SIZE));

    if (ADDR_IS_APP(app_end))
    {
        calculatedCrc = *(__IO uint32_t *)(app_end);
    }

    return calculatedCrc;
}

/**
 * @brief get signed application size
 */
size_t yaa_bootloader_get_app_size(void)
{
    return bootloader_param.app_size;
}

/**
 * @brief Update application size based on the flashed firmware
 */
size_t yaa_bootloader_update_app_size(void)
{
    uintptr_t app_end = (*(__IO uint32_t *)(bootloader_param.app_start + __VECTOR_TABLE_SIZE));
    if (ADDR_IS_APP(app_end))
    {
        bootloader_param.app_size = app_end - bootloader_param.app_start;
    }
    else
    {
        bootloader_param.app_size = YAA_APP_SIZE;
    }
    return bootloader_param.app_size;
}

/**
 * @brief Erase area for checksum
 */
uint8_t yaa_bootloader_erase_info(void)
{
    uint32_t SectorError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;

    yaa_flash_unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Sector = yaa_flash_get_sector(YAA_INFO_ADDRESS);
    EraseInitStruct.NbSectors = 1;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    // Erase flash sector
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
    {
        yaa_flash_lock();
        return YAA_BL_ERASE_ERROR;
    }

    yaa_flash_lock();

    return YAA_BL_OK;
}

/**
 * @brief Save checksum to flash
 */
uint8_t yaa_bootloader_save_info(void)
{
    yaa_image_header_t *flash_header = (yaa_image_header_t *)YAA_INFO_ADDRESS;
    yaa_image_header_t header = { 0 };
    uint32_t SectorError = 0;
    FLASH_EraseInitTypeDef EraseInitStruct;

    header.image_magic = YAA_BOOTLOADER_MAGIC;
    header.image_size = yaa_bootloader_update_app_size();
    header.crc = yaa_bootloader_get_checksum();
    header.image_version = yaa_bootloader_get_version();
    if (flash_header->image_magic == YAA_BOOTLOADER_MAGIC)
    {
        header.flash_count = (flash_header->flash_count + 1);
    }
    else
    {
        header.flash_count = 1;
    }

    yaa_flash_unlock();

    EraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
    EraseInitStruct.Sector = yaa_flash_get_sector(YAA_INFO_ADDRESS);
    EraseInitStruct.NbSectors = 1;
    EraseInitStruct.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    // Erase flash sector
    if (HAL_FLASHEx_Erase(&EraseInitStruct, &SectorError) != HAL_OK)
    {
        yaa_flash_lock();
        return YAA_BL_ERASE_ERROR;
    }

    // Write info
    for (uint16_t i = 0; i < sizeof(header) / sizeof(uint32_t); i++)
    {
        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, YAA_INFO_ADDRESS + (i * sizeof(uint32_t)),
                              (uint64_t)header.data[i]) != HAL_OK)
        {
            yaa_flash_lock();
            return YAA_BL_ERASE_ERROR;
        }
    }

    yaa_flash_lock();

    return YAA_BL_OK;
}

/**
 * @brief Check for application in user flash
 */
uint8_t yaa_bootloader_check_for_application(void)
{
    /* We first check if the first 4 bytes starting from APP_START_ADDRESS
       contain the MSP (end of SRAM).*/
    if ((((*(__IO uint32_t *)bootloader_param.app_start) - bootloader_param.ram_size) == 0x20000000))
    {
        /* Check magic word on the end of binary */
        uintptr_t app_end = (*(__IO uint32_t *)(bootloader_param.app_start + __VECTOR_TABLE_SIZE));
        if (!ADDR_IS_APP(app_end))
        {
            return YAA_BL_NO_APP;
        }

        uint32_t magic = *(__IO uint32_t *)(app_end - sizeof(uint32_t));
        if (magic != YAA_BOOTLOADER_MAGIC)
        {
            return YAA_BL_NO_APP;
        }

#ifdef HAL_CRC_MODULE_ENABLED
        uint32_t crc = *(__IO uint32_t *)(app_end);
        /* Binary without CRC*/
        if (crc == 0xFFFFFFFF)
        {
            return YAA_BL_OK;
        }
        /* Check CRC */
        if (crc != yaa_bootloader_get_checksum())
        {
            return YAA_BL_CHKS_ERROR;
        }
#endif // HAL_CRC_MODULE_ENABLED
        return YAA_BL_OK;
    }
    else
    {
        return YAA_BL_NO_APP;
    }
}

/**
 * @brief Jump to application
 */
void yaa_bootloader_jump_to_app(void)
{
    uint32_t JumpAddress = *(__IO uint32_t *)(bootloader_param.app_start + 4);
    pFunction Jump = (pFunction)JumpAddress;

    HAL_RCC_DeInit();
    HAL_DeInit();

    // HAL_NVIC_DisableIRQ();

    // Disables SysTick timer and its related interrupt
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    // Disable all interrupts related to clock
    RCC->CIR = 0x00000000;
    __disable_irq();

#if (SET_VECTOR_TABLE)
    __DMB(); // ARM says to use a DMB instruction before relocating VTOR */
    SCB->VTOR = bootloader_param.app_start;
    __DSB(); // ARM says to use a DSB instruction just after relocating VTOR */
#endif

    __set_MSP(*(__IO uint32_t *)bootloader_param.app_start);
    // We start the execution from he Reset_Handler of the main firmware
    Jump();

    while (1)
    {
    }; // Never coming here
}

/**
 * Jump to System Memory (ST Bootloader)
 */
void yaa_bootloader_jump_to_sysmem(void)
{
    uint32_t JumpAddress = *(__IO uint32_t *)(YAA_SYSMEM_ADDRESS + 4);
    pFunction Jump = (pFunction)JumpAddress;

    HAL_RCC_DeInit();
    HAL_DeInit();

    // Disables SysTick timer and its related interrupt
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;

    __HAL_RCC_SYSCFG_CLK_ENABLE();
    __HAL_SYSCFG_REMAPMEMORY_SYSTEMFLASH();

    __set_MSP(*(__IO uint32_t *)YAA_SYSMEM_ADDRESS);
    Jump();

    while (1)
    {
    }; // Never coming here
}

uint32_t yaa_bootloader_get_version(void)
{
    return ((YAA_BOOTLOADER_VERSION_MAJOR << 24) | (YAA_BOOTLOADER_VERSION_MINOR << 16) |
            (YAA_BOOTLOADER_VERSION_PATCH << 8) | (YAA_BOOTLOADER_VERSION_RC));
}

uint32_t yaa_bootloader_get_flashed_count(void)
{
    yaa_image_header_t *flash_header = (yaa_image_header_t *)YAA_INFO_ADDRESS;
    if (flash_header->image_magic == YAA_BOOTLOADER_MAGIC)
    {
        return flash_header->flash_count;
    }
    else
    {
        return 0;
    }
}
