/**
 * @file rdnx_sal.h
 * @author Software development team
 * @brief Software application layer APIs
 * @version 1.0
 * @date 2024-09-09
 */
#ifndef RDNX_SAL_H
#define RDNX_SAL_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h>
#include <stddef.h> // for size_t
#include <stdint.h>

/* Core includes. */
#include <rdnx_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/* NOTE: If you are using CMSIS, the registers can also be
   accessed through CoreDebug->DHCSR & CoreDebug_DHCSR_C_DEBUGEN_Msk */
#define HALT_IF_DEBUGGING()                                \
    do                                                     \
    {                                                      \
        if ((*(volatile uint32_t *)0xE000EDF0) & (1 << 0)) \
        {                                                  \
            __asm("bkpt 1");                               \
        }                                                  \
    } while (0)

/* Define min and max priority for platform */
#define RDNX_THREAD_IDLE_PRIO (0)
#define RDNX_THREAD_MAX_PRIO  (11)

/* Define size for UID buffer, see #rdnx_device_uuid */
#define RDNX_DEVICE_UUID_SIZE (12)

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Thread priorities
 *
 * Lower numbers correspond to lower priorities
 */
typedef enum rdnx_thread_prio_t
{
    RDNX_THREAD_PRIO_0 = RDNX_THREAD_IDLE_PRIO,       // Priority 0 - lowest priority
    RDNX_THREAD_PRIO_1 = (RDNX_THREAD_IDLE_PRIO + 1), // Priority 1
    RDNX_THREAD_PRIO_2 = (RDNX_THREAD_IDLE_PRIO + 2), // Priority 2
    RDNX_THREAD_PRIO_3 = (RDNX_THREAD_IDLE_PRIO + 3), // Priority 3
    RDNX_THREAD_PRIO_4 = (RDNX_THREAD_IDLE_PRIO + 4), // Priority 4
    RDNX_THREAD_PRIO_5 = (RDNX_THREAD_IDLE_PRIO + 5), // Priority 5
    RDNX_THREAD_PRIO_6 = (RDNX_THREAD_IDLE_PRIO + 6), // Priority 6
    RDNX_THREAD_PRIO_7 = (RDNX_THREAD_IDLE_PRIO + 7), // Priority 7
    RDNX_THREAD_PRIO_8 = (RDNX_THREAD_IDLE_PRIO + 8), // Priority 8
    RDNX_THREAD_PRIO_9 = (RDNX_THREAD_IDLE_PRIO + 9), // Priority 9
    RDNX_THREAD_PRIO_10 = (RDNX_THREAD_MAX_PRIO - 1), // Priority 10 - highest priority

    /** Maximum priority */
    RDNX_THREAD_PRIO_MAX = RDNX_THREAD_PRIO_10,

    // Semantic priority mappings

    /** @brief Idle thread priority */
    RDNX_THREAD_PRIO_IDLE = RDNX_THREAD_PRIO_0,

    /** @brief Lowest non-idle priority */
    RDNX_THREAD_PRIO_LOWEST = RDNX_THREAD_PRIO_1,

    /** @brief Minimum priority for a system thread */
    RDNX_THREAD_PRIO_SYSTEM_MIN = RDNX_THREAD_PRIO_2,

    /** @brief Low priority for an application thread */
    RDNX_THREAD_PRIO_LOW = RDNX_THREAD_PRIO_3,

    /** @brief Normal priority for an application thread */
    RDNX_THREAD_PRIO_NORMAL = RDNX_THREAD_PRIO_5,

    /** @brief High priority for an application thread */
    RDNX_THREAD_PRIO_HIGH = RDNX_THREAD_PRIO_7,

    /** @brief Maximum priority for a system thread */
    RDNX_THREAD_PRIO_SYSTEM_MAX = RDNX_THREAD_PRIO_8,

    /** @brief Highest non-critical priority */
    RDNX_THREAD_PRIO_HIGHEST = RDNX_THREAD_PRIO_9,

    /** @brief Timing-critical thread priority */
    RDNX_THREAD_PRIO_TIME_CRITICAL = RDNX_THREAD_PRIO_10,

    // Indicative priorities used by system services

    /** @brief Priority for system networking service threads */
    RDNX_THREAD_PRIO_NETWORK = RDNX_THREAD_PRIO_6,

    /** @brief Priority for system GUI threads */
    RDNX_THREAD_PRIO_GUI = RDNX_THREAD_PRIO_7
} rdnx_thread_prio_t;

/**
 * @brief Mutex handle
 */
typedef struct rdnx_mutex_struct *rdnx_mutex_t;

/**
 * @brief Counting semaphore
 */
typedef struct rdnx_semaphore_struct *rdnx_sem_t;

/**
 * @brief Function pointer type for external printf-like functions.
 *
 * This defines a pointer to a function that behaves like printf, taking
 * a format string and optional arguments, and returning the number of
 * characters printed.
 *
 * @param fmt   Format string, similar to printf.
 * @param ...   Variable arguments corresponding to the format specifiers.
 * @return      Number of characters printed, same as standard printf.
 */
typedef int (*rdnx_printf_t)(const char *fmt, ...);

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Memory allocator
 *
 * @param size Memory size, in bytes.
 * @return Pointer to the allocated memory, or NULL if the request fails.
 */
void *rdnx_alloc(size_t size);

/**
 * @brief Memory deallocator
 *
 * @param ptr - Pointer to a memory block previously allocated by a call
 * 		          rdnx_alloc.
 * @return None.
 */
void rdnx_free(void *ptr);

/**
 * @brief Get system time, in millisecond.
 *
 * Get the number of system clock ticks since system startup.
 *
 * @return System tick count, in millisecond.
 */
uint32_t rdnx_systemtime(void);

/**
 * @brief Has a specified interval elapsed since a given time
 *
 * @param[in] start A system timestamp, in milliseconds, as returned by
 *                  #rdnx_systemtime
 * @param[in] pause An interval, in milliseconds
 *
 * @retval true  The interval has elapsed.  The current system time is larger
 *               than the provided start time plus the interval.
 * @retval false The interval has not yet elapsed.  The current system time
 *               is smaller than the provided start time plus the interval.
 */
bool rdnx_istimespent(uint32_t start, uint32_t pause);

/**
 * @brief Put the current thread to sleep for a period of time, in milliseconds
 *
 * This function is typically implemented using the operating system's thread
 * scheduler, blocking the current thread until the time elapses.
 *
 * @param[in] delay_ms Amount of time to sleep, in milliseconds
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_mdelay(uint32_t delay_ms);

/**
 * @brief Put the platform to sleep for a period of time, in microseconds
 *
 * This function is typically implemented using chip-specific functionality,
 * which may block the entire CPU core, not just the current thread.
 *
 * @param[in] delay_us Amount of time to sleep, in microseconds
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_udelay(uint32_t delay_us);

/**
 * @brief      Obtain the system reset cause
 * @param      None
 * @return     The system reset cause
 */
rdnx_boot_reason_t rdnx_boot_reason_get(void);

/**
 * @brief Check if in isr context (true), or task context (false)
 */
bool rdnx_is_isr(void);

/**
 * @brief Reboot a device
 */
uint8_t rdnx_reboot(void);

/**
 * @brief Get device UUID if supported.
 *        The UID is a 96-bit value.
 *
 * @param[in] uuid Pointer to array with size 12 byte, see
 * #RDNX_DEVICE_UUID_SIZE
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_device_uuid(uint8_t *uuid);

/**
 * @brief Create a (non-recursive) mutex object
 *
 * @param[out] mutex Pointer to memory which, on success, will contain a handle
 *                   to the mutex
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_mutex_create(rdnx_mutex_t *mutex);

/**
 * @brief Destroy a mutex object
 *
 * **Warning:** Do not destroy a mutex that has threads blocked on it
 *
 * @param[in] mutex Handle to the mutex
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_mutex_destroy(rdnx_mutex_t mutex);

/**
 * @brief Lock the mutex
 *
 * If the mutex is already locked, this call will block until another thread
 * unlocks the mutex with a call to #rdnx_mutex_unlock.
 *
 * **IMPORTANT:** If the current thread has the mutex locked and the mutex is
 * not recursive, the thread will block until the timeout interval elapses
 * (which will be a deadlock if #RDNX_TIMO_FOREVER is used as the interval).
 *
 * @param[in] mutex   Handle to the mutex
 * @param[in] wait_ms Timeout parameter, in milliseconds.  If the call blocks,
 *                    and the timeout interval elapses before another thread
 *                    unlocks the mutex, the function will return
 *                    #RDNX_ERR_TIMEOUT.  In addition to time intervals, the
 *                    following special values may be used:
 *                    * #RDNX_TIMO_NOWAIT.  Non-blocking.  The call will fail
 *                      immediately if the call would otherwise block.  This is
 *                      sometimes known as "try-lock" behavior.
 *                    * #RDNX_TIMO_FOREVER.  Block indefinitely.  The call will
 *                      block until another thread unlocks the mutex.
 *
 * @retval #RDNX_ERR_OK      The mutex was successfully locked
 * @retval #RDNX_ERR_TIMEOUT The mutex is locked by another thread and the
 *                           timeout interval elapsed before it was released.
 */
uint8_t rdnx_mutex_lock(rdnx_mutex_t mutex, uint32_t wait_ms);

/**
 * @brief Unlock the mutex
 *
 * If the mutex is recursive and it has been locked multiple times, the count
 * of held locks will be decremented.
 *
 * If the mutex is non-recursive or if the count decrements to zero, and
 * another thread is blocked on the mutex, it will be unblocked by this call.
 * If multiple threads are blocked on the mutex, exactly one will be unblocked.
 *
 * @param[in] mutex Handle to the mutex
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_mutex_unlock(rdnx_mutex_t mutex);

/**
 * @brief Create a semaphore object
 *
 * @param[out] sem           Pointer to memory which, on success, will contain
 *                           a handle to the semaphore.
 * @param[in]  initial_count The initial value of the semaphore.  Must be less
 *                           than *max_count*
 * @param[in]  max_count     The maximum value the semaphore may count up to.
 *                           If *max_count* is 1, a binary (non-counting)
 *                           semaphore will be used internally (if supported by
 *                           the underlying operating system).  Zero an illegal
 *                           value.
 *
 * @retval #RDNX_ERR_OK     on success
 * @retval #RDNX_ERR_BADARG if *max_count* is zero or *initial_count* is
 *                          greater than *max_count*
 */
uint8_t rdnx_sem_create(rdnx_sem_t *sem, uint32_t initial_count, uint32_t max_count);

/**
 * @brief Destroy a semaphore object
 *
 * **Warning:** Do not destroy a semaphore that has threads blocked on it
 *
 * @param[in] sem Handle to the semaphore
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_sem_destroy(rdnx_sem_t sem);

/**
 * @brief Take a semaphore
 *
 * If the semaphore's count is non-zero, it is decremented.
 *
 * If the count is zero, the thread blocks until the another thread gives the
 * semaphore using #rdnx_sem_give.
 *
 * @param[in] sem     Handle to the semaphore
 * @param[in] wait_ms Timeout parameter, in milliseconds.  If the call blocks,
 *                    and the timeout interval elapses before another thread
 *                    gives the semaphore, the function will return
 *                    #RDNX_ERR_TIMEOUT.  In addition to time intervals, the
 *                    following special values may be used:
 *                    * #RDNX_TIMO_NOWAIT.  Non-blocking.  The call will fail
 *                      immediately if the call would otherwise block.
 *                    * #RDNX_TIMO_FOREVER.  Block indefinitely.  The call will
 *                      block until another thread gives the semaphore.
 *
 * @retval #RDNX_ERR_OK      The semaphore was successfully taken
 * @retval #RDNX_ERR_TIMEOUT The semaphore's count is 0 and the timeout
 *                           interval elapsed before any other thread could
 *                           increment it.
 */
uint8_t rdnx_sem_take(rdnx_sem_t sem, uint32_t wait_ms);

/**
 * @brief Give a semaphore
 *
 * If another thread is blocked on the semaphore, it is unblocked.  If multiple
 * threads are blocked on the semaphore, exactly one of them is unblocked.
 *
 * If no threads are blocked on the semaphore, its count is incremented.
 *
 * @param[in] sem Handle to the semaphore
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_sem_give(rdnx_sem_t sem);

/**
 * @brief Get the current count of a semaphore
 *
 * @param[in]  sem   Handle to the semaphore
 * @param[out] count Pointer to memory which, on success, will contain the
 *                   semaphore's current count
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_sem_getcount(rdnx_sem_t sem, uint32_t *count);

/**
 * @brief Reset a semaphore's count to zero
 *
 * @param[in] sem Handle to the semaphore
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_sem_reset(rdnx_sem_t sem);

/**
 * @brief Get the current free heap memory.
 *
 * This function returns the amount of free heap memory available in bytes.
 * It is typically used to monitor dynamic memory usage in embedded systems
 * that utilize an RTOS or direct memory management.
 *
 * @return size_t The free heap size in bytes.
 */
size_t rdnx_get_free_heap(void);

/**
 * @brief Get the current free RAM memory (including stack and static regions).
 *
 * This function returns the amount of free RAM available in the system, which includes
 * both the heap and static memory regions, such as the stack. It is useful for
 * checking overall system memory health, including unused stack space.
 *
 * @return size_t The free RAM size in bytes.
 */
size_t rdnx_get_free_ram(void);

/**
 * @brief Print FreeRTOS task status information.
 *
 * This function gathers and prints a snapshot of all tasks in the system,
 * including task name, state, priority, stack usage, and runtime statistics.
 * The output is directed through a user-provided printf-like function.
 *
 * @param printf_func Pointer to a printf-compatible function used to output
 *                    the task information. Can be standard printf, UART printf,
 *                    or any other custom logging function.
 *
 * @note The function allocates memory for task status snapshots internally
 *       and frees it before returning. Ensure that rdnx_alloc and rdnx_free
 *       are implemented correctly for your platform.
 *
 * @warning The runtime statistics are scaled based on total run time and
 *          may be rounded down to the nearest integer.
 */
void rdnx_cmd_ps(rdnx_printf_t printf_func);

#ifdef __cplusplus
}
#endif

#endif // RDNX_SAL_H
