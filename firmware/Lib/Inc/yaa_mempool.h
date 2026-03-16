/**
 * @file yaa_mempool.h
 * @author Software development team
 * @brief Memory pool management APIs
 * @version 1.0
 * @date 2024-09-09
 */
#ifndef YAA_MEMPOOL_H
#define YAA_MEMPOOL_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Core includes. */
#include <yaa_types.h>

/* ============================================================================
 * Public Macro Definitions
 * ==========================================================================*/

/**
 * @brief The default memory alignment
 *
 * The default memory alignment is the size of the `int` type, which is
 * platform-dependent.
 */
#define YAA_MEMPOOL_ALIGNMENT_DEFAULT sizeof(int)

/**
 * @brief Compute the minimum amount of memory required by a memory pool
 *
 * The memory size computed is a function of the total number of objects, the
 * size of each object and the pool's memory alignment.
 *
 * @param[in] count The total number of objects in the pool
 * @param[in] size  The size of each object
 * @param[in] align The memory alignment
 *
 * @returns The minimum amount of memory, in bytes, required for a pool using
 *          the specified parameters
 */
#define YAA_MEMPOOL_MEM_SIZE_ALIGN(count, size, align) ((count) * ((((size) + (align) - 1) / (align)) * (align)))

/**
 * @brief Compute the minimum amount of memory required by a memory pool with
 * default memory alignment
 *
 * The memory size computed is a function of the total number of objects and
 * the size of each object.
 *
 * @param[in] count The total number of objects in the pool
 * @param[in] size  The size of each object
 *
 * @returns The minimum amount of memory, in bytes, required for a pool using
 *          the specified parameters
 */
#define YAA_MEMPOOL_MEM_SIZE(count, size) YAA_MEMPOOL_MEM_SIZE_ALIGN((count), (size), YAA_MEMPOOL_ALIGNMENT_DEFAULT)

/**
 * @brief Flag to request allocation of a pool from external memory
 *
 * See also yaa_mempool_param::flags
 */
#define YAA_MEMPOOL_FLAG_EXT_MEM BIT(0)

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief Handle to an initialized memory pool
 */
typedef void *yaa_mempool_t;

/* ============================================================================
 * Global Function Declarations
 * ==========================================================================*/

/**
 * @brief Create a memory pool
 *
 * @param[in]  block_count Maximum number of objects in the pool
 * @param[in]  block_size  Size of each object
 * @param[in]  param       Memory pool parameters.  `NULL` means default values
 *                         should be used for all parameters
 * @param[out] handle      Pointer to memory which, on success, will contain a
 *                         handle to the memory pool.
 *
 * @return #YAA_ERR_OK on success
 */
uint8_t yaa_mempool_create(uint32_t block_count, uint32_t block_size, yaa_mempool_t *handle);

/**
 * @brief Allocate an object from a memory pool
 *
 * @param[in] handle  Handle to the memory pool
 * @param[in] timeout Timeout parameter, in milliseconds.  If there are no free
 *                    objects in the pool, the call will block until either an
 *                    object becomes available or the timeout interval
 *                    elapses.  In addition to time intervals, the following
 *                    special values may be used:
 *                    * #YAA_TIMO_NOWAIT.  Non-blocking.  The call will fail
 *                      immediately if no objects are available.
 *                    * #YAA_TIMO_FOREVER.  Block indefinitely.  The call will
 *                      block until an object becomes available.
 *
 * @return A pointer to the allocated object on success or `NULL` on failure.
 */
void *yaa_mempool_alloc(yaa_mempool_t handle, int32_t timeout);

/**
 * @brief Return (free) an object to a memory pool
 *
 * @param[in] handle Handle to the memory pool
 * @param[in] block  Pointer to an object allocated from the pool
 *
 * @retval #YAA_ERR_OK       Success
 * @retval #YAA_ERR_NOTFOUND The object was not allocated from this pool
 */
uint8_t yaa_mempool_free(yaa_mempool_t handle, void *block);

/**
 * @brief Get the maximum number of objects belonging to a pool
 *
 * This includes both objects that have been allocated and objects that remain
 * in the pool.
 *
 * @param[in] handle Handle to the memory pool
 *
 * @return The maximum number of objects belonging to the pool
 */
uint32_t yaa_mempool_get_capacity(yaa_mempool_t handle);

/**
 * @brief Get a pool's object size
 *
 * @param[in] handle Handle to the memory pool
 *
 * @return The size of a single object in the pool
 */
uint32_t yaa_mempool_get_block_size(yaa_mempool_t handle);

/**
 * @brief Get the number of objects that have been allocated from a pool
 *
 * This is the number of outstanding objects belonging to the pool.  That is,
 * the count of objects that have been allocated and not yet freed.
 *
 * @param[in] handle Handle to the memory pool
 *
 * @return The number of objects allocated from the pool
 */
uint32_t yaa_mempool_get_count(yaa_mempool_t handle);

/**
 * @brief Get the number of objects that may be allocated from a pool
 *
 * This is the number of objects that may be allocated.  That is, the total
 * pool size, less the number of objects that have been allocated.
 *
 * @param[in] handle Handle to the memory pool
 *
 * @return The number of objects that may be allocated
 */
uint32_t yaa_mempool_get_space(yaa_mempool_t handle);

/**
 * @brief Destroy (free) a memory pool
 *
 * Destroying a memory pool frees any memory that was dynamically allocated
 * when the pool was created.
 *
 * Before a pool may be destroyed, all objects allocated from the pool must be
 * freed back to the pool.
 *
 * @param[in] handle Handle to the memory pool
 *
 * @retval #YAA_ERR_OK   Success
 * @retval #YAA_ERR_BUSY The pool could not be freed because one or more of
 *                        its objects have been allocated and not freed.
 */
uint8_t yaa_mempool_destroy(yaa_mempool_t handle);

#endif // YAA_MEMPOOL_H
