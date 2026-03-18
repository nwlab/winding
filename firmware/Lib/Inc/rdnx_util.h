/**
 * @file rdnx_util.h
 * @author Software development team
 * @brief Utility functions for bit manipulation and general helpers
 * @version 1.0
 * @date 2026-02-11
 */
#ifndef RDNX_UTIL_H
#define RDNX_UTIL_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/**
 * @brief Generate an integer mask from a logical bit number
 */
#ifndef BIT
#define BIT(n) (1UL << (n))
#endif

/**
 * @brief Compute the number of elements in an array
 *
 * This macro only works for arrays that are declared as arrays with a fixed
 * size.  It does not work on pointers to arrays or arrays declared with an
 * undetermined number of elements.
 */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

/**
 * @brief Calculate the address of the base of a structure, given its type and
 *        an address of a field within the structure.
 *
 * Usage:
 * ~~~~~~{.c}
 * base_addr = CONTAINER_OF(internal_addr, base_type, field_name);
 * ~~~~~~
 *
 * where:
 * * `base_addr` is the address of the containing struct you're seeking
 * * `internal_addr` is the address of a field within that struct
 * * `base_type` is the type of the containing struct
 * * `field_name` is that name of the field in the containing struct
 *
 * For example, suppose you have this struct:
 * ~~~~~~{.c}
 * typedef struct foo
 * {
 *     struct bar    x;
 *     struct baz    y;
 *     struct foobie z;
 * } foo;
 * ~~~~~~
 *
 * If you have a pointer to field `z` of this struct and you need a pointer to
 * the `struct foo` that contains it, the following code fragment will work:
 * ~~~~~~{.c}
 * foo *base_object;
 * struct foobie *ptr;
 *
 * base_object = CONTAINER_OF(ptr, foo, z);
 * ~~~~~~
 *
 * This idea for this macro comes from the Windows NT DDK.  It was modified to
 * use `offsetof()` in order to be portable to all ANSI-C platforms.
 */
#ifndef CONTAINER_OF
#define CONTAINER_OF(address, type, field) \
                    ((type *)((char *)(address) - offsetof(type, field)))
#endif

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/* Check if bit set */
static inline int rdnx_test_bit(int pos, const volatile void * addr)
{
	return (((const int *)addr)[pos / 32] >> pos) & 1UL;
}

uint32_t rdnx_find_first_set_bit(uint32_t word);

/* Shift the value and apply the specified mask. */
uint32_t rdnx_field_prep(uint32_t mask, uint32_t val);
/* Get a field specified by a mask from a word. */
uint32_t rdnx_field_get(uint32_t mask, uint32_t word);

#endif /* RDNX_UTIL_H */
