/**
 * @file rdnx_bkpsram.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Core includes. */
#include <rdnx_macro.h>
#include <rdnx_types.h>

/* Check compatibility */
#if defined(STM32F401xx) || defined(STM32F411xE)
#error "Not available on F401/F411 devices"
#endif

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/* ============================================================================
 * Private Variable Declarations
 * ==========================================================================*/

#if defined(BKPSRAM_BASE)
#define BKPSRAM_LENGTH (0x00001000)
#else
#define BKPSRAM_LENGTH (0x00001000)
static uint8_t BKPSRAM_BASE[BKPSRAM_LENGTH];
#endif

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

rdnx_err_t rdnx_bkpsram_init(void)
{
    HAL_PWR_EnableBkUpAccess();

    /* Enable PWR clock */
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;

    /* Enable backup SRAM Clock */
    __HAL_RCC_BKPSRAM_CLK_ENABLE();

    /* Enable write access to Backup domain */
    PWR->CR |= PWR_CR_DBP;

    /* Enable the Backup SRAM low power Regulator */
    /* This will allow data to stay when using VBat mode */
    PWR->CSR |= PWR_CSR_BRE;

    /* Wait for backup regulator to be ready  */
    if (HAL_PWREx_EnableBkUpReg() == HAL_TIMEOUT)
    {
        return RDNX_ERR_TIMEOUT;
    }

    return RDNX_ERR_OK;
}

void rdnx_bkpsram_enable_writes(void)
{
    /* Enable write access to Backup domain */
    SET_BIT(PWR->CR, PWR_CR_DBP);
}

void rdnx_bkpsram_disable_writes(void)
{
    /* Disable write access to Backup domain */
    CLEAR_BIT(PWR->CR, PWR_CR_DBP);
}

uint32_t rdnx_bkpsram_get_size(void)
{
    return BKPSRAM_LENGTH;
}

void *rdnx_bkpsram_get_address(void)
{
    return (void *)BKPSRAM_BASE;
}
