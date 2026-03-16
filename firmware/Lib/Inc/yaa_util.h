/**
 * @file yaa_util.h
 * @author Software development team
 * @brief Utility functions for bit manipulation and general helpers
 * @version 1.0
 * @date 2026-02-11
 */
#ifndef YAA_UTIL_H
#define YAA_UTIL_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/* Check if bit set */
static inline int yaa_test_bit(int pos, const volatile void * addr)
{
	return (((const int *)addr)[pos / 32] >> pos) & 1UL;
}

uint32_t yaa_find_first_set_bit(uint32_t word);

/* Shift the value and apply the specified mask. */
uint32_t yaa_field_prep(uint32_t mask, uint32_t val);
/* Get a field specified by a mask from a word. */
uint32_t yaa_field_get(uint32_t mask, uint32_t word);

#endif /* YAA_UTIL_H */
