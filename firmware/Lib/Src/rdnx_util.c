/**
 * @file    rdnx_util.c
 * @author  Software development team
 * @brief   Utility functions for bit manipulation and general helpers
 */

#include "rdnx_util.h"  // Make sure the header exists and declares this function

/**
 * @brief Find the index of the first set bit (LSB = 0) in a 32-bit word.
 *
 * @param word The 32-bit value to search.
 * @return Index of the first set bit [0..31]. Returns 32 if no bits are set.
 */
uint32_t rdnx_find_first_set_bit(uint32_t word)
{
    uint32_t first_set_bit = 0;

    while (word)
    {
        if (word & 0x1)
            return first_set_bit;
        word >>= 1;
        first_set_bit++;
    }

    return 32; // No bits set
}

/**
 * Shift the value and apply the specified mask.
 */
uint32_t rdnx_field_prep(uint32_t mask, uint32_t val)
{
    return (val << rdnx_find_first_set_bit(mask)) & mask;
}

/**
 * Get a field specified by a mask from a word.
 */
uint32_t rdnx_field_get(uint32_t mask, uint32_t word)
{
    return (word & mask) >> rdnx_find_first_set_bit(mask);
}
