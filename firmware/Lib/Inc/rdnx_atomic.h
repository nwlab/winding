/**
 * @file
 * @author Software development team
 * @brief C11-like atomic API
 * @version 0.1
 * @date 2026-03-18
 */

#ifndef RDNX_ATOMIC_H
#define RDNX_ATOMIC_H

#include <stdatomic.h>

/**
 * @brief Initialization.
 */

#define RDNX_ATOMIC_VAR_INIT(value) ATOMIC_VAR_INIT(value)

/**
 * @brief Order and consistency.
 *
 * The memory_order_* constants that denote the barrier behaviour of the
 * atomic operations.
 */

typedef enum memory_order rdnx_memory_order;

/**
 * @brief Fences.
 */

#define rdnx_atomic_thread_fence(order) atomic_thread_fence(order)
#define rdnx_atomic_signal_fence(order) atomic_signal_fence(order)

/**
 * @brief Lock-free property.
 */

#define rdnx_atomic_is_lock_free(X) atomic_is_lock_free(X)

/**
 * @brief Atomic integer types.
 */

typedef atomic_bool rdnx_atomic_bool;
typedef atomic_char rdnx_atomic_char;
typedef atomic_schar rdnx_atomic_schar;
typedef atomic_uchar rdnx_atomic_uchar;
typedef atomic_short rdnx_atomic_short;
typedef atomic_ushort rdnx_atomic_ushort;
typedef atomic_int rdnx_atomic_int;
typedef atomic_uint rdnx_atomic_uint;
typedef atomic_long rdnx_atomic_long;
typedef atomic_ulong rdnx_atomic_ulong;
typedef atomic_llong rdnx_atomic_llong;
typedef atomic_ullong rdnx_atomic_ullong;
typedef atomic_wchar_t rdnx_atomic_wchar_t;
typedef atomic_int_least8_t rdnx_atomic_int_least8_t;
typedef atomic_uint_least8_t rdnx_atomic_uint_least8_t;
typedef atomic_int_least16_t rdnx_atomic_int_least16_t;
typedef atomic_uint_least16_t rdnx_atomic_uint_least15_t;
typedef atomic_int_least32_t rdnx_atomic_int_least32_t;
typedef atomic_uint_least32_t rdnx_atomic_uint_least32_t;
typedef atomic_int_least64_t rdnx_atomic_int_least64_t;
typedef atomic_uint_least64_t rdnx_atomic_uint_least64_t;
typedef atomic_int_fast8_t rdnx_atomic_int_fast8_t;
typedef atomic_uint_fast8_t rdnx_atomic_uint_fast8_t;
typedef atomic_int_fast16_t rdnx_atomic_int_fast16_t;
typedef atomic_uint_fast16_t rdnx_atomic_uint_fast16_t;
typedef atomic_int_fast32_t rdnx_atomic_int_fast32_t;
typedef atomic_uint_fast32_t rdnx_atomic_uint_fast32_t;
typedef atomic_int_fast64_t rdnx_atomic_int_fast64_t;
typedef atomic_uint_fast64_t rdnx_atomic_uint_fast64_t;
typedef atomic_intptr_t rdnx_atomic_intptr_t;
typedef atomic_uintptr_t rdnx_atomic_uintptr_t;
typedef atomic_size_t rdnx_atomic_size_t;
typedef atomic_ptrdiff_t rdnx_atomic_ptrdiff_t;
typedef atomic_intmax_t rdnx_atomic_intmax_t;
typedef atomic_uintmax_t rdnx_atomic_uintmax_t;

/**
 * @brief Operations on atomic types.
 */

#define rdnx_atomic_compare_exchange_strong_explicit(object, expected,          \
                                                    desired, success, failure)  \
    atomic_compare_exchange_strong_explicit(object, expected,                   \
                                            desired, success, failure)

#define rdnx_atomic_compare_exchange_weak_explicit(object, expected,            \
                                                    desired, success, failure)  \
    atomic_compare_exchange_weak_explicit(object, expected,                     \
                                            desired, success, failure)

#define rdnx_atomic_exchange_explicit(object, desired, order)   \
    atomic_exchange_explicit(object, desired, order)

#define rdnx_atomic_fetch_add_explicit(object, operand, order)  \
    atomic_fetch_add_explicit(object, operand, order)

#define rdnx_atomic_fetch_and_explicit(object, operand, order)  \
    atomic_fetch_and_explicit(object, operand, order)

#define rdnx_atomic_fetch_or_explicit(object, operand, order)   \
    atomic_fetch_or_explicit(object, operand, order)

#define rdnx_atomic_fetch_sub_explicit(object, operand, order)  \
    atomic_fetch_sub_explicit(object, operand, order)

#define rdnx_atomic_fetch_xor_explicit(object, operand, order)  \
    atomic_fetch_xor_explicit(object, operand, order)

#define rdnx_atomic_load_explicit(object, order)    \
    atomic_load_explicit(object, order)

#define rdnx_atomic_store_explicit(object, desired, order)      \
    atomic_store_explicit(object, desired, order)

#define rdnx_atomic_compare_exchange_strong(object, expected, desired)  \
    atomic_compare_exchange_strong(object, expected, desired)

#define rdnx_atomic_compare_exchange_weak(object, expected, desired)    \
    atomic_compare_exchange_weak(object, expected, desired)

#define rdnx_atomic_exchange(object, desired)       \
    atomic_exchange(object, desired)

#define rdnx_atomic_fetch_add(object, operand)      \
    atomic_fetch_add(object, operand)

#define rdnx_atomic_fetch_and(object, operand)      \
    atomic_fetch_and(object, operand)

#define rdnx_atomic_fetch_or(object, operand)       \
    atomic_fetch_or(object, operand)

#define rdnx_atomic_fetch_sub(object, operand)      \
    atomic_fetch_sub(object, operand)

#define rdnx_atomic_fetch_xor(object, operand)      \
    atomic_fetch_xor(object, operand)

#define rdnx_atomic_load(object) atomic_load(object)

#define rdnx_atomic_store(object, desired) atomic_store(object, desired)

/**
 * @brief Atomic flag type and operations.
 */

typedef atomic_flag rdnx_atomic_flag;

#define RDNX_ATOMIC_FLAG_INIT ATOMIC_FLAG_INIT

#define rdnx_atomic_flag_test_and_set_explicit(flag_obj, order) \
    atomic_flag_test_and_set_explicit(flag_obj, order)

#define rdnx_atomic_flag_clear_explicit(flag_obj, order) \
    atomic_flag_clear_explicit(flag_obj, order)

#define rdnx_atomic_flag_test_and_set(flag_obj) \
    atomic_flag_test_and_set(flag_obj)

#define rdnx_atomic_flag_clear(flag_obj) \
    atomic_flag_clear(flag_obj)

#endif // RDNX_ATOMIC_H
