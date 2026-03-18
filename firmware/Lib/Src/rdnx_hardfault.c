/**
 * @file rdnx_hardfault.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdarg.h>
#include <stdio.h>

/* Library includes. */
#include <stm32f4xx.h>

/* Core includes. */
#include <rdnx_sal.h>

// clang-format off

/* ============================================================================
 * Public Function Declaration
 * ==========================================================================*/

extern void hardfault_print(const char *string);

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

// Disable optimizations for this function so "frame" argument
// does not get optimized away
__attribute__((optimize("O0")))
void hard_fault_handler_c(unsigned int *hardfault_args)
{
    unsigned int stacked_r0;
    unsigned int stacked_r1;
    unsigned int stacked_r2;
    unsigned int stacked_r3;
    unsigned int stacked_r12;
    unsigned int stacked_lr;
    unsigned int stacked_pc;
    unsigned int stacked_psr;

    char logString[60];

    stacked_r0 = ((unsigned long)hardfault_args[0]);
    stacked_r1 = ((unsigned long)hardfault_args[1]);
    stacked_r2 = ((unsigned long)hardfault_args[2]);
    stacked_r3 = ((unsigned long)hardfault_args[3]);

    stacked_r12 = ((unsigned long)hardfault_args[4]);
    stacked_lr  = ((unsigned long)hardfault_args[5]);
    stacked_pc  = ((unsigned long)hardfault_args[6]);
    stacked_psr = ((unsigned long)hardfault_args[7]);

    sprintf(logString, "\n>>>>>>>>>>>>>>[");
    hardfault_print(logString);
    switch (__get_IPSR())
    {
    case 3:
        sprintf(logString, "Hard Fault");
        hardfault_print(logString);
        break;

    case 4:
        sprintf(logString, "Memory Manage");
        hardfault_print(logString);
        break;

    case 5:
        sprintf(logString, "Bus Fault");
        hardfault_print(logString);
        break;

    case 6:
        sprintf(logString, "Usage Fault");
        hardfault_print(logString);
        break;

    default:
        sprintf(logString, "Unknown Fault %ld", __get_IPSR());
        hardfault_print(logString);
        break;
    }
    sprintf(logString, ",corrupt,dump registers]>>>>>>>>>>>>>>>>>>\n\r");
    hardfault_print(logString);

    sprintf(logString, "R0 = 0x%08x\r\n", stacked_r0);
    hardfault_print(logString);
    sprintf(logString, "R1 = 0x%08x\r\n", stacked_r1);
    hardfault_print(logString);
    sprintf(logString, "R2 = 0x%08x\r\n", stacked_r2);
    hardfault_print(logString);
    sprintf(logString, "R3 = 0x%08x\r\n", stacked_r3);
    hardfault_print(logString);
    sprintf(logString, "R12 = 0x%08x\r\n", stacked_r12);
    hardfault_print(logString);
    sprintf(logString, "LR [R14] = 0x%08x  subroutine call return address\r\n", stacked_lr);
    hardfault_print(logString);
    sprintf(logString, "PC [R15] = 0x%08X  program counter\r\n", stacked_pc);
    hardfault_print(logString);
    sprintf(logString, "PSR = 0x%08X\r\n", stacked_psr);
    hardfault_print(logString);
    sprintf(logString, "BFAR = 0x%08lx\r\n", (*((volatile unsigned long *)(0xE000ED38))));
    hardfault_print(logString);
    sprintf(logString, "CFSR = 0x%08lx\r\n", (*((volatile unsigned long *)(0xE000ED28))));
    hardfault_print(logString);
    sprintf(logString, "HFSR = 0x%08lx\r\n", (*((volatile unsigned long *)(0xE000ED2C))));
    hardfault_print(logString);
    sprintf(logString, "DFSR = 0x%08lx\r\n", (*((volatile unsigned long *)(0xE000ED30))));
    hardfault_print(logString);
    sprintf(logString, "AFSR = 0x%08lx\r\n", (*((volatile unsigned long *)(0xE000ED3C))));
    hardfault_print(logString);

    while (1)
    {
        // If and only if a debugger is attached, execute a breakpoint
        // instruction so we can take a look at what triggered the fault
        HALT_IF_DEBUGGING();
    }
}

/* ============================================================================
 * Global Function Definitions for testing hardfaults
 * ==========================================================================*/

int illegal_instruction_execution(void)
{
    int (*bad_instruction)(void) = (int (*)(void))0xE0000000;
    return bad_instruction();
}

uint32_t read_from_bad_address(void)
{
    return *(volatile uint32_t *)0xbadcafe;
}

void bad_addr_double_word_write(void)
{
    volatile uint64_t *buf = (volatile uint64_t *)0x30000000;
    *buf = 0x1122334455667788;
}

void unaligned_double_word_read(void)
{
    extern void *g_unaligned_buffer;
    uint64_t *buf = g_unaligned_buffer;
    *buf = 0x1122334455667788;
}

// clang-format on
