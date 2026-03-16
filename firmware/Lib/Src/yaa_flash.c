/**
 * @file yaa_flash.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Core includes. */
#include <yaa_event.h>
#include <yaa_flash.h>
#include <yaa_macro.h>
#include <yaa_queue.h>
#include <yaa_sal.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/* Function pointer for jumping to user application. */
typedef void (*fnc_ptr)(void);

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/**
 * @brief   This function erases the memory.
 * @param   address: First address to be erased.
 * @param   end_address: End address to be erased.
 * @return  status: Report about the success of the erasing.
 */
yaa_flash_status_t yaa_flash_erase(uint32_t address, uint32_t end_address)
{
    yaa_flash_unlock();

    yaa_flash_status_t status = YAA_FLASH_ERROR;
    FLASH_EraseInitTypeDef erase_init;
    uint32_t error = 0u;

    erase_init.TypeErase = FLASH_TYPEERASE_SECTORS;

    // Get the sector for the start address
    uint32_t start_sector = yaa_flash_get_sector(address);

    // Get the sector for the end address
    uint32_t end_sector = yaa_flash_get_sector(end_address);

    // Calculate the number of sectors
    // If the start and end addresses are within the same sector, only erase that sector
    if (start_sector == end_sector) {
        erase_init.NbSectors = 1;
    } else {
        erase_init.NbSectors = end_sector - start_sector + 1;
    }

    // Set the sector to start erasing
    erase_init.Sector = start_sector;

    // Set voltage range for flash operation
    erase_init.VoltageRange = FLASH_VOLTAGE_RANGE_3;

    // Perform the erase operation
    if (HAL_OK == HAL_FLASHEx_Erase(&erase_init, &error))
    {
        status = YAA_FLASH_OK;
    }

    yaa_flash_lock();

    return status;
}

/**
 * @brief   Read data from FLASH memory.
 * @param   address  Start address to read from.
 * @param   data     Destination buffer.
 * @param   length   Number of bytes to read.
 * @return  yaa_flash_status_t
 *          - YAA_FLASH_OK on success
 *          - YAA_FLASH_ERROR on invalid arguments
 */
yaa_flash_status_t yaa_flash_read(uint32_t address, uint8_t *data, uint32_t length)
{
    if (data == NULL || length == 0)
    {
        return YAA_FLASH_ERROR;
    }

    /* Direct memory read (internal flash is memory-mapped) */
    memcpy(data, (const void *)address, length);

    return YAA_FLASH_OK;
}

/**
 * @brief   This function flashes the memory.
 * @param   address: First address to be written to.
 * @param   *data:   Array of the data that we want to write.
 * @param   *length: Size of the array.
 * @return  status: Report about the success of the writing.
 */
yaa_flash_status_t yaa_flash_write(uint32_t address, uint32_t *data, uint32_t length)
{
    yaa_flash_status_t status = YAA_FLASH_OK;

    yaa_flash_unlock();

    /* Loop through the array. */
    for (uint32_t i = 0u; (i < length) && (YAA_FLASH_OK == status); i++)
    {
        /* The actual flashing. If there is an error, then report it. */
        if (HAL_OK != HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, data[i]))
        {
            status |= YAA_FLASH_ERROR_WRITE;
        }
        /* Read back the content of the memory. If it is wrong, then report
         * an error. */
        if (((data[i])) != (*(volatile uint32_t *)address))
        {
            status |= YAA_FLASH_ERROR_READBACK;
        }

        /* Shift the address by a word. */
        address += 4u;
    }

    yaa_flash_lock();

    return status;
}

void yaa_flash_unlock(void)
{
    HAL_FLASH_Unlock();
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGSERR) != RESET)
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGSERR);
    if (__HAL_FLASH_GET_FLAG(FLASH_FLAG_PGPERR) != RESET)
        __HAL_FLASH_CLEAR_FLAG(FLASH_FLAG_PGPERR);
}

void yaa_flash_lock(void)
{
    HAL_FLASH_Lock();
}

/**
 * @brief   Actually jumps to the user application.
 * @param   address Application address
 * @return  void
 */
void yaa_flash_jump_to_app(uint32_t address)
{
    /* Function pointer to the address of the user application. */
    fnc_ptr jump_to_app;

    jump_to_app = (fnc_ptr)(*(volatile uint32_t *)(address + 4u));

    // HAL_DeInit();
    RCC->APB1RSTR = 0xFFFFFFFFU;
    RCC->APB1RSTR = 0x00;
    RCC->APB2RSTR = 0xFFFFFFFFU;
    RCC->APB2RSTR = 0x00;

    // SysTick DeInit
    SysTick->CTRL = 0;
    SysTick->VAL = 0;
    SysTick->LOAD = 0;

    __disable_irq();

    // NVIC DeInit
    __set_BASEPRI(0);
    __set_CONTROL(0);
    NVIC->ICER[0] = 0xFFFFFFFF;
    NVIC->ICPR[0] = 0xFFFFFFFF;
    NVIC->ICER[1] = 0xFFFFFFFF;
    NVIC->ICPR[1] = 0xFFFFFFFF;
    NVIC->ICER[2] = 0xFFFFFFFF;
    NVIC->ICPR[2] = 0xFFFFFFFF;

    __enable_irq();

    /* Change the main and local  stack pointer. */
    __set_MSP(*(volatile uint32_t *)address);
    SCB->VTOR = *(volatile uint32_t *)address;

    jump_to_app();
}

/**
 * @brief Define the SECTORS according to the reference manual
 * STM32F407VG have:-
 *  Sector 0 to Sector 3 each 16KB
 *  Sector 4 as 64KB
 *  Sector 5 to Sector 11 each 128KB
 */

uint32_t yaa_flash_get_sector(uint32_t address)
{
    uint32_t sector = 0;

    if ((address <= 0x08003FFF) && (address >= 0x08000000))
    {
        sector = FLASH_SECTOR_0;
    }
    else if ((address <= 0x08007FFF) && (address >= 0x08004000))
    {
        sector = FLASH_SECTOR_1;
    }
    else if ((address <= 0x0800BFFF) && (address >= 0x08008000))
    {
        sector = FLASH_SECTOR_2;
    }
    else if ((address <= 0x0800FFFF) && (address >= 0x0800C000))
    {
        sector = FLASH_SECTOR_3;
    }
    else if ((address <= 0x0801FFFF) && (address >= 0x08010000))
    {
        sector = FLASH_SECTOR_4;
    }
    else if ((address <= 0x0803FFFF) && (address >= 0x08020000))
    {
        sector = FLASH_SECTOR_5;
    }
    else if ((address <= 0x0805FFFF) && (address >= 0x08040000))
    {
        sector = FLASH_SECTOR_6;
    }
    else if ((address <= 0x0807FFFF) && (address >= 0x08060000))
    {
        sector = FLASH_SECTOR_7;
    }
    else if ((address <= 0x0809FFFF) && (address >= 0x08080000))
    {
        sector = FLASH_SECTOR_8;
    }
    else if ((address <= 0x080BFFFF) && (address >= 0x080A0000))
    {
        sector = FLASH_SECTOR_9;
    }
    else if ((address <= 0x080DFFFF) && (address >= 0x080C0000))
    {
        sector = FLASH_SECTOR_10;
    }
    else if ((address <= 0x080FFFFF) && (address >= 0x080E0000))
    {
        sector = FLASH_SECTOR_11;
    }
#if defined(STM32F427xx) || defined(STM32F437xx) || defined(STM32F429xx) || defined(STM32F439xx) || \
    defined(STM32F469xx) || defined(STM32F479xx)
    else if ((address <= 0x08103FFF) && (address >= 0x08100000))
    {
        sector = FLASH_SECTOR_12;
    }
    else if ((address <= 0x08107FFF) && (address >= 0x08104000))
    {
        sector = FLASH_SECTOR_13;
    }
    else if ((address <= 0x0810BFFF) && (address >= 0x08108000))
    {
        sector = FLASH_SECTOR_14;
    }
    else if ((address <= 0x0810FFFF) && (address >= 0x0810C000))
    {
        sector = FLASH_SECTOR_15;
    }
    else if ((address <= 0x0811FFFF) && (address >= 0x08110000))
    {
        sector = FLASH_SECTOR_16;
    }
    else if ((address <= 0x0813FFFF) && (address >= 0x08120000))
    {
        sector = FLASH_SECTOR_17;
    }
    else if ((address <= 0x0815FFFF) && (address >= 0x08140000))
    {
        sector = FLASH_SECTOR_18;
    }
    else if ((address <= 0x0817FFFF) && (address >= 0x08160000))
    {
        sector = FLASH_SECTOR_19;
    }
    else if ((address <= 0x0819FFFF) && (address >= 0x08180000))
    {
        sector = FLASH_SECTOR_20;
    }
    else if ((address <= 0x081BFFFF) && (address >= 0x081A0000))
    {
        sector = FLASH_SECTOR_21;
    }
    else if ((address <= 0x081DFFFF) && (address >= 0x081C0000))
    {
        sector = FLASH_SECTOR_22;
    }
    else if (address <= 0x081FFFFF) && (address >= 0x081E0000))
        {
            sector = FLASH_SECTOR_23;
        }
#endif
    return sector;
}
