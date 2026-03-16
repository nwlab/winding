/**
 * @file yaa_macro.h
 * @author Software development team
 * @brief Global macro definitions
 * @version 1.0
 * @date 2024-09-09
 */
#ifndef YAA_MACRO_H
#define YAA_MACRO_H

#include <assert.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/**
 * @brief Prevents compiler warning when function parameter or variable is not
 * used
 */
#define YAA_UNUSED(x) ((void)(x))

#define YAA_UNUSED_FUNC __attribute__((unused))

/**
 * @brief Cast a value to a target type via ptrdiff_t.
 *
 * This macro performs an intermediate cast to @c ptrdiff_t before casting
 * to the final target type. It is primarily intended for safe and explicit
 * conversions between pointers and integer types, where a direct cast may
 * trigger compiler warnings or be non-portable.
 *
 * @param target_type Destination type of the cast.
 * @param val         Value to be cast (pointer or integer).
 *
 * @note Use with care: this macro assumes that @c ptrdiff_t is wide enough
 *       to hold the value being converted.
 */
#ifndef YAA_CAST
#define YAA_CAST(target_type, val) ((target_type)((ptrdiff_t)val))
#endif

/**
 * @brief Make string literal from preprocessor definition
 */
#define YAA_STRINGIFY(...)          YAA_STRINGIFY_VARIADIC(__VA_ARGS__)
#define YAA_STRINGIFY_VARIADIC(...) #__VA_ARGS__

/**
 * @brief inline keyword for GNU Compiler
 */
#define YAA_INLINE        inline
#define YAA_STATIC_INLINE static inline

/**
 * @macro YAA_STATIC_ASSERT()
 * @brief Compile time check
 *
 * Breaks compilation process if condition is not true
 *
 * @param EXPR - condition that expects to be true
 */
#ifdef __cplusplus
#ifndef _Static_assert
#define _Static_assert static_assert
#endif
#endif
#define YAA_STATIC_ASSERT(EXPR) _Static_assert((EXPR), "(" #EXPR ") failed")

/** @brief Weak attribute */
#define YAA_WEAK __attribute__((weak))

/** @brief Packed attribute */
#define YAA_PACKED __attribute__((__packed__))

/**
 * @brief Get number of macro's arguments
 */
// clang-format off
#define YAA_LIB_PP_NARG(...) \
         YAA_LIB_PP_NARG_(__VA_ARGS__,YAA_LIB_PP_RSEQ_N())
#define YAA_LIB_PP_NARG_(...) \
         YAA_LIB_PP_ARG_N(__VA_ARGS__)
#define YAA_LIB_PP_ARG_N( \
          _1, _2, _3, _4, _5, _6, _7, _8, _9,_10, \
         _11,_12,_13,_14,_15,_16,_17,_18,_19,_20, \
         _21,_22,_23,_24,_25,_26,_27,_28,_29,_30, \
         _31,_32,_33,_34,_35,_36,_37,_38,_39,_40, \
         _41,_42,_43,_44,_45,_46,_47,_48,_49,_50, \
         _51,_52,_53,_54,_55,_56,_57,_58,_59,_60, \
         _61,_62,_63,N,...) N
#define YAA_LIB_PP_RSEQ_N() \
         63,62,61,60,                   \
         59,58,57,56,55,54,53,52,51,50, \
         49,48,47,46,45,44,43,42,41,40, \
         39,38,37,36,35,34,33,32,31,30, \
         29,28,27,26,25,24,23,22,21,20, \
         19,18,17,16,15,14,13,12,11,10, \
         9,8,7,6,5,4,3,2,1
// clang-format on

/**
 * @brief Runtime check
 *
 * Breaks execution if condition is not true.
 * Macro is functional in debug build only; in release build it is defined
 * as a no-op.
 *
 * @param 1st condition that expects to be true
 * @param 2nd and so on - optional printf()-like format and parameters
 *                        message will be printed before execution is stopped
 */
#ifndef configASSERT
#define configASSERT assert
#endif

#define YAA_ASSERT(...)         YAA_ASSERT_HID(YAA_LIB_PP_NARG(__VA_ARGS__), __VA_ARGS__)
#define YAA_ASSERT_HID(...)     YAA_ASSERT_HID1(__VA_ARGS__)
#define YAA_ASSERT_HID1(n, ...) YAA_ASSERT_##n(__VA_ARGS__)
#define YAA_ASSERT_1(expr)      configASSERT(expr)
#define YAA_ASSERT_2(...)       YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_3(...)       YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_4(...)       YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_5(...)       YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_6(...)       YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_7(...)       YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_8(...)       YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_9(...)       YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_10(...)      YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_11(...)      YAA_ASSERT_n(__VA_ARGS__)
#define YAA_ASSERT_12(...)      YAA_ASSERT_n(__VA_ARGS__)

#ifndef NDEBUG
extern void hardfault_print(const char *string);
#define YAA_ASSERT_n(expr, fmt, ...)                                                  \
    do                                                                                 \
    {                                                                                  \
        if (!(expr))                                                                   \
        {                                                                              \
            hardfault_print("ASSERT "                                                  \
                            "[" __FILE__ ":" YAA_STRINGIFY(__LINE__) "] " fmt "\n\r", \
                            ##__VA_ARGS__);                                            \
            configASSERT(false);                                                       \
        }                                                                              \
    } while (0)
#else
#define YAA_ASSERT_n(...) (void)0
#endif

/* Macro to use CCM (Core Coupled Memory) in STM32F4 */
#define CCM_RAM __attribute__((section(".ccmram")))

#define YAA_MIN(a, b) (((a) < (b)) ? (a) : (b))
#define YAA_MAX(a, b) (((a) > (b)) ? (a) : (b))

/** @brief Fallthrough attribute */
#define YAA_FALLTHROUGH __attribute__((fallthrough))

#define YAA_ALIGN(size, align) ((((size) + (align) - 1) / (align)) * (align))

#define YAA_COUNTOF(a) (sizeof(a) / sizeof((a)[0]))

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

#define YAA_CONTAINER_OF(ptr, type, name) ((type *)((char *)(ptr) - offsetof(type, name)))

#define YAA_BIT(x) (1 << (x))

#define YAA_BITS_PER_LONG (32)

// clang-format off
#define YAA_GENMASK(h, l)              \
    ( ((uint32_t)(~0U) << (l)) &        \
      ((uint32_t)(~0U) >> (31 - (h))) )
// clang-format on

#ifdef __cplusplus
}
#endif

#endif
