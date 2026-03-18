/**
 * @file rdnx_alloc.c
 * @author Software development team
 * @brief Wrapper alloc
 * @version 1.0
 * @date 2026-03-18
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h>

/* Core includes. */
#include <rdnx_macro.h>

/* ============================================================================
 * Public Function Declaration
 * ==========================================================================*/

extern void *rdnx_alloc(size_t size);
extern void rdnx_free(void *ptr);

 /* ============================================================================
 * Wrappers to replace library memory allocation functions
 * ==========================================================================*/

void *__wrap__malloc_r(void *reent, size_t nbytes)
{
    RDNX_UNUSED(reent);
    return rdnx_alloc(nbytes);
}

void __wrap__free_r(void *reent, void *ptr)
{
    RDNX_UNUSED(reent);
    if (ptr)
    {
        rdnx_free(ptr);
    }
}
