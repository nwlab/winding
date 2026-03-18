/**
 * @file rdnx_queue.h
 * @author Software development team
 * @brief Queue APIs
 * @version 1.0
 * @date 2024-09-09
 */

#ifndef RDNX_QUEUE_H
#define RDNX_QUEUE_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief A handle to an initialized queue
 */
typedef struct rdnx_queue *rdnx_queue_handle_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Create a queue
 *
 * @param[out] queue           Pointer to memory which, on success, will
 *                             contain a handle to the newly-created queue
 * @param[in]  max_depth       The maximum number of items that may be
 *                             simultaneously placed on the queue.
 * @param[in]  queue_item_size The size of a single queue element
 *
 * @return 0 on success
 */
uint8_t rdnx_queue_create(rdnx_queue_handle_t *queue, uint32_t queue_depth, uint32_t queue_item_size);

/**
 * @brief Destroy queue
 *
 * After this call completes, the queue handle will be invalid.
 *
 * Attempting to destroy a queue while any thread is using it (including being
 * blocked on a queue operation) will result in undefined behavior.
 *
 * @param[in] queue Handle to the queue
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_destroy(rdnx_queue_handle_t queue);

/**
 * @brief Post an item to a queue
 *
 * The item is posted to the back of the queue.  This function is equivalent to
 * #rdnx_queue_post_item_to_back.
 *
 * If the queue is full, the thread blocks until another thread removes an item
 * from the queue using #rdnx_queue_get_item or #rdnx_queue_get_item_from_isr.
 *
 * The item's data is copied to the queue's memory.
 *
 * This function should not be used from interrupt handlers.  Interrupt code
 * should instead call #rdnx_queue_post_item_from_isr.
 *
 * @param[in] queue   Handle to the queue
 * @param[in] item    Pointer to the item which will be posted to the queue
 * @param[in] timeout Timeout parameter, in milliseconds.  If the call blocks,
 *                    and the timeout interval elapses before another thread
 *                    removes an item from the queue, the function will return
 *                    an error code.  In addition to time intervals, the
 *                    following special values may be used:
 *                    * #RDNX_TIMO_NOWAIT  Non-blocking.  The call will fail
 *                      immediately if the call would otherwise block.
 *                    * #RDNX_TIMO_FOREVER  Block indefinitely.  The call will
 *                      block until another thread removed an item from the
 *                      queue.
 *
 * @return 0 on success
 */
uint8_t rdnx_queue_post_item(rdnx_queue_handle_t queue, const void *item, uint32_t timeout);

/**
 * @brief Post an item to a queue from interrupt context
 *
 * This is functionally equivalent to #rdnx_queue_post_item, but may be called
 * from an interrupt handler context.
 *
 * The item is posted to the back of the queue.  This function is equivalent to
 * #rdnx_queue_post_item_to_back_from_isr.
 *
 * If the queue is full, an error is returned.
 *
 * The item's data is copied to the queue's memory.
 *
 * @param[in] queue   Handle to the queue
 * @param[in] item    Pointer to the item which will be posted to the queue
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_post_item_from_isr(rdnx_queue_handle_t queue, const void *item);

/**
 * @brief Post an item to the back of a queue
 *
 * The item is posted to the back of the queue, implementing FIFO (first-in,
 * first-out) behavior.  This function is equivalent to #rdnx_queue_post_item.
 *
 * If the queue is full, the thread blocks until another thread removes an item
 * from the queue using #rdnx_queue_get_item or #rdnx_queue_get_item_from_isr.
 *
 * The item's data is copied to the queue's memory.
 *
 * This function should not be used from interrupt handlers.  Interrupt code
 * should instead call #rdnx_queue_post_item_to_back_from_isr.
 *
 * @param[in] queue   Handle to the queue
 * @param[in] item    Pointer to the item which will be posted to the queue
 * @param[in] timeout Timeout parameter, in milliseconds.  If the call blocks,
 *                    and the timeout interval elapses before another thread
 *                    removes an item from the queue, the function will return
 *                    an error code.  In addition to time intervals, the
 *                    following special values may be used:
 *                    * #RDNX_TIMO_NOWAIT  Non-blocking.  The call will fail
 *                      immediately if the call would otherwise block.
 *                    * #RDNX_TIMO_FOREVER  Block indefinitely.  The call will
 *                      block until another thread removed an item from the
 *                      queue.
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_post_item_to_back(rdnx_queue_handle_t queue, const void *item, uint32_t timeout);

/**
 * @brief Post an item to the front of a queue
 *
 * The item is posted to the front of the queue, implementing LIFO (last-in,
 * first-out) behavior.
 *
 * If the queue is full, the thread blocks until another thread removes an item
 * from the queue using #rdnx_queue_get_item or #rdnx_queue_get_item_from_isr.
 *
 * The item's data is copied to the queue's memory.
 *
 * This function should not be used from interrupt handlers.  Interrupt code
 * should instead call #rdnx_queue_post_item_to_front_from_isr.
 *
 * @param[in] queue   Handle to the queue
 * @param[in] item    Pointer to the item which will be posted to the queue
 * @param[in] timeout Timeout parameter, in milliseconds.  If the call blocks,
 *                    and the timeout interval elapses before another thread
 *                    removes an item from the queue, the function will return
 *                    an error code.  In addition to time intervals, the
 *                    following special values may be used:
 *                    * #RDNX_TIMO_NOWAIT  Non-blocking.  The call will fail
 *                      immediately if the call would otherwise block.
 *                    * #RDNX_TIMO_FOREVER  Block indefinitely.  The call will
 *                      block until another thread removed an item from the
 *                      queue.
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_post_item_to_front(rdnx_queue_handle_t queue, const void *item, uint32_t timeout);

/**
 * @brief Post an item to the back of a queue from interrupt context
 *
 * This is functionally equivalent to #rdnx_queue_post_item_to_back, but may be
 * called from an interrupt handler context.
 *
 * The item is posted to the back of the queue, implementing FIFO (first-in,
 * first-out) behavior.  This function is equivalent to
 * #rdnx_queue_post_item_from_isr.
 *
 * If the queue is full, an error is returned.
 *
 * The item's data is copied to the queue's memory.
 *
 * @param[in] queue Handle to the queue
 * @param[in] item  Pointer to the item which will be posted to the queue
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_post_item_to_back_from_isr(rdnx_queue_handle_t queue, const void *item);

/**
 * @brief Post an item to the back of a queue from interrupt context
 *
 * This is functionally equivalent to #rdnx_queue_post_item_to_front, but may
 * be called from an interrupt handler context.
 *
 * The item is posted to the front of the queue, implementing LIFO (last-in,
 * first-out) behavior.
 *
 * If the queue is full, an error is returned.
 *
 * The item's data is copied to the queue's memory.
 *
 * @param[in] queue Handle to the queue
 * @param[in] item  Pointer to the item which will be posted to the queue
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_post_item_to_front_from_isr(rdnx_queue_handle_t queue, const void *item);

/**
 * @brief Get an item from a queue
 *
 * The item is removed from the front of the queue.
 *
 * If the queue is empty, the thread blocks until another thread posts an item
 * to the queue.
 *
 * The item's data is copied to the application-provided memory.
 *
 * This function should not be used from interrupt handlers.  Interrupt code
 * should instead call #rdnx_queue_get_item_from_isr.
 *
 * @param[in]  queue   Handle to the queue
 * @param[out] item    Pointer to memory which, in success, will contain the
 *                     removed item's data.  The size of this memory must be at
 *                     least as large as the queue's item size.  See also
 *                     #rdnx_queue_create.
 * @param[in]  timeout Timeout parameter, in milliseconds.  If the call blocks,
 *                     and the timeout interval elapses before another thread
 *                     posts an item to the queue, the function will return
 *                     an error code.  In addition to time intervals, the
 *                     following special values may be used:
 *                     * #RDNX_TIMO_NOWAIT  Non-blocking.  The call will fail
 *                       immediately if the call would otherwise block.
 *                     * #RDNX_TIMO_FOREVER Block indefinitely.  The call
 *                       will block until another thread posts an item to the
 *                       queue.
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_get_item(rdnx_queue_handle_t queue, void *item, uint32_t timeout);

/**
 * @brief Get an item from a queue from interrupt context.
 *
 * This is functionally equivalent to #rdnx_queue_get_item, but may be called
 * from an interrupt handler context.
 *
 * The item is removed from the front of the queue.
 *
 * If the queue is empty, an error is returned.
 *
 * The item's data is copied to the application-provided memory.
 *
 * @param[in]  queue   Handle to the queue
 * @param[out] item    Pointer to memory which, in success, will contain the
 *                     removed item's data.  The size of this memory must be at
 *                     least as large as the queue's item size.  See also
 *                     #rdnx_queue_create.
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_get_item_from_isr(rdnx_queue_handle_t queue, void *item);

/**
 * @brief Get an item from a queue without dequeuing it
 *
 * The item at the front of the queue is returned to the application without
 * removing it from the queue.
 *
 * If the queue is empty, the thread blocks until another thread posts an item
 * to the queue.
 *
 * The item's data is copied to the application-provided memory.
 *
 * This function should not be used from interrupt handlers.  Interrupt code
 * should instead call #rdnx_queue_peek_item_from_isr.
 *
 * @param[in]  queue   Handle to the queue
 * @param[out] item    Pointer to memory which, in success, will contain the
 *                     item's data.  The size of this memory must be at least
 *                     as large as the queue's item size.  See also
 *                     #rdnx_queue_create.
 * @param[in]  timeout Timeout parameter, in milliseconds.  If the call blocks,
 *                     and the timeout interval elapses before another thread
 *                     posts an item to the queue, the function will return
 *                     an error code.  In addition to time intervals, the
 *                     following special values may be used:
 *                     * #RDNX_TIMO_NOWAIT  Non-blocking.  The call will fail
 *                       immediately if the call would otherwise block.
 *                     * #RDNX_TIMO_FOREVER  Block indefinitely.  The call
 *                       will block until another thread posts an item to the
 *                       queue.
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_peek_item(rdnx_queue_handle_t queue, void *item, uint32_t timeout);

/**
 * @brief Get an item from a queue from interrupt context without dequeuing it
 *
 * This is functionally equivalent to #rdnx_queue_peek_item, but may be called
 * from an interrupt handler context.
 *
 * The item at the front of the queue is returned to the application without
 * removing it from the queue.
 *
 * If the queue is empty, an error is returned.
 *
 * The item's data is copied to the application-provided memory.
 *
 * @param[in]  queue   Handle to the queue
 * @param[out] item    Pointer to memory which, in success, will contain the
 *                     item's data.  The size of this memory must be at least
 *                     as large as the queue's item size.  See also
 *                     #rdnx_queue_create.
 *
 * @return #RDNX_ERR_OK on success
 */
uint8_t rdnx_queue_peek_item_from_isr(rdnx_queue_handle_t queue, void *item);

/**
 * @brief Get the number of items currently in a queue
 *
 * @param[in] queue Handle to the queue
 *
 * @return The number of items currently in the queue
 */
uint32_t rdnx_queue_get_queue_size(rdnx_queue_handle_t queue);

/**
 * @brief Get the number of items which may be posted to a queue
 *
 * This is the difference between queue's depth (see #rdnx_queue_create) and
 * the number of items currently in the queue (see
 * #rdnx_queue_get_queue_size).
 *
 * @param[in] queue Handle to the queue
 *
 * @return The number of items which may be posted to the queue
 */
uint32_t rdnx_queue_get_free_space(rdnx_queue_handle_t queue);

#ifdef __cplusplus
}
#endif

#endif // RDNX_QUEUE_H
