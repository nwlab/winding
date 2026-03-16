/**
 * @file yaa_mempool.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Scheduler includes. */
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

/* Core includes. */
#include <yaa_macro.h>
#include <yaa_mempool.h>
#include <yaa_queue.h>
#include <yaa_sal.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Macro Definitions
 * ==========================================================================*/

/**
 * The Memory pool is implemented with two approaches.
 * - Uses bitmask fields to mark memory object is free.
 * - Uses yaa_queue to store pointers to free objects.
 * Define the macro YAA_MEMPOOL_USE_QUEUE to enable the second approach.
 * Otherwise will be used the first.
 * Memory usage in bytes with 12 byte message size:
 * --------------------------------
 * | Blocks  | 5   | 100  | 1000  |
 * |---------|-----|------|-------|
 * | Bitmask | 480 | 1664 | 12544 |
 * | Queue   | 384 | 1920 | 16288 |
 * --------------------------------
 */
#define YAA_MEMPOOL_USE_QUEUE

/**
 * @brief This macro exposes the count of bits for word.
 */
#define BITS_PER_UINT32 (8 * sizeof(uint32_t))

/**
 * @brief This macro exposes the count of words for bit array.
 */
#define YAA_MEMPOOL_BIT_ARRAY_SIZE(max) (((max) + BITS_PER_UINT32 - 1) / BITS_PER_UINT32)

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

typedef struct yaa_mempool_struct
{
    /** @brief Memory pool buffer. */
    void *pool;
    /** @brief Indicates real block count */
    uint32_t block_count;
    /** @brief Indicates real block size, calculated with byte alignment */
    uint32_t block_size;
#ifdef YAA_MEMPOOL_USE_QUEUE
    /** @brief System queue used to hold pointers to free blocks. */
    yaa_queue_handle_t queue;
#else
    /** @brief Mutex to protect the contents of this structure. */
    yaa_mutex_t mutex;
    /** @brief Semaphore for event-get calls to block against when the memory
     * pool is empty. */
    yaa_sem_t sem;
    /** @brief Continues bit array. The size is depend of block_count. */
    uint32_t bits[1];
#endif
} yaa_mempool_struct_t;

/* ============================================================================
 * Private Function Definitions
 * ==========================================================================*/

#ifndef YAA_MEMPOOL_USE_QUEUE
/**
 * @brief Lock an event queue's mutex
 */
static yaa_err_t mempool_lock(yaa_mempool_struct_t *q, uint32_t timeout)
{
    return yaa_mutex_lock(q->mutex, timeout);
}

/**
 * @brief Unlock an event queue's mutex
 */
static yaa_err_t mempool_unlock(yaa_mempool_struct_t *q)
{
    return yaa_mutex_unlock(q->mutex);
}

/**
 * @brief Take (possibly blocking on) an event queue's semaphore
 */
static yaa_err_t mempool_sem_wait(yaa_mempool_struct_t *q, uint32_t timeout)
{
    return yaa_sem_take(q->sem, timeout);
}

/**
 * @brief Give an event queue's semaphore, possibly unblocking other threads
 */
YAA_STATIC_INLINE void mempool_sem_post(yaa_mempool_struct_t *q)
{
    (void)yaa_sem_give(q->sem);
}
#endif

/**
 * @brief Align the size with provided alignment value.
 */
YAA_STATIC_INLINE size_t yaa_size_round_up(size_t size, size_t aligment)
{
    size_t rounded_size;

    rounded_size = ((size + aligment - 1) / aligment) * aligment;

    return rounded_size;
}

/**
 * @brief Calculate pointer to block based on size and block index.
 */
YAA_STATIC_INLINE void *yaa_block_ptr(yaa_mempool_struct_t *ctrl, size_t sz, int block)
{
    return (uint8_t *)ctrl->pool + sz * block;
}

/**
 * @brief Calculate index of block from pointer based on size of block.
 */
YAA_STATIC_INLINE int yaa_block_num(yaa_mempool_struct_t *ctrl, void *block, int sz)
{
    return ((uint8_t *)block - (uint8_t *)ctrl->pool) / sz;
}

#ifndef YAA_MEMPOOL_USE_QUEUE
/* Places a 32 bit output pointer in word, and an integer bit index
 * within that word as the return value
 */
static int yaa_get_bit_ptr(yaa_mempool_struct_t *ctrl, int bn, uint32_t **word)
{
    uint32_t *bitarray = ctrl->bits;

    *word = &bitarray[bn / BITS_PER_UINT32];

    return bn & 0x1f;
}

static void yaa_set_alloc_bit(yaa_mempool_struct_t *ctrl, int bn)
{
    uint32_t *word;
    int bit = yaa_get_bit_ptr(ctrl, bn, &word);

    *word |= (1 << bit);
}

static void yaa_clear_alloc_bit(yaa_mempool_struct_t *ctrl, int bn)
{
    uint32_t *word;
    int bit = yaa_get_bit_ptr(ctrl, bn, &word);

    *word &= ~(1 << bit);
}

static inline bool yaa_alloc_bit_is_set(yaa_mempool_struct_t *ctrl, int bn)
{
    uint32_t *word;
    int bit = yaa_get_bit_ptr(ctrl, bn, &word);

    return (*word >> bit) & 1;
}

static int yaa_find_first_clear(const uint32_t *bitmap, uint32_t bits)
{
    uint32_t words = YAA_MEMPOOL_BIT_ARRAY_SIZE(bits);
    uint32_t cnt;
    uint32_t neg_bitmap;

    /*
     * By bitwise negating the bitmap, we are actually implementing
     * ffc (find first clear) using ffs (find first set).
     */
    for (cnt = 0U; cnt < words; cnt++)
    {
        neg_bitmap = ~bitmap[cnt];
        if (neg_bitmap == 0) /* all full */
        {
            continue;
        }
        else if (neg_bitmap == ~0UL) /* first bit */
        {
            return cnt * BITS_PER_UINT32;
        }
        else
        {
            return cnt * BITS_PER_UINT32 + ffs(neg_bitmap) - 1;
        }
    }
    return -1;
}

static int yaa_find_first_set(const uint32_t *bitmap, uint32_t bits)
{
    uint32_t words = YAA_MEMPOOL_BIT_ARRAY_SIZE(bits);
    uint32_t cnt;

    for (cnt = 0U; cnt < words; cnt++)
    {
        if (bitmap[cnt] == 0) /* all empty */
        {
            continue;
        }
        else if (bitmap[cnt] == ~0UL) /* first bit */
        {
            return cnt * BITS_PER_UINT32;
        }
        else
        {
            return cnt * BITS_PER_UINT32 + ffs(bitmap[cnt]) - 1;
        }
    }
    return -1;
}

static void yaa_bitarray_clear(uint32_t *bitmap, uint32_t bits)
{
    uint32_t words = YAA_MEMPOOL_BIT_ARRAY_SIZE(bits);
    uint32_t cnt;
    for (cnt = 0U; cnt < words; cnt++)
    {
        bitmap[cnt] = 0;
    }
}
#endif

/**
 * Allocate or set memory for messages with provided parameters.
 */
static yaa_err_t yaa_mempool_pool_create(yaa_mempool_struct_t *ctrl)
{
    const size_t mp_size = ctrl->block_count * ctrl->block_size;

    ctrl->pool = yaa_alloc(mp_size);

    if (ctrl->pool == NULL)
    {
        return YAA_ERR_NOMEM;
    }

    return YAA_ERR_OK;
}

/**
 * Mark all messages as free in the pool
 */
static void yaa_mempool_pool_init(yaa_mempool_struct_t *ctrl)
{
#ifndef YAA_MEMPOOL_USE_QUEUE
    yaa_bitarray_clear(ctrl->bits, ctrl->block_count);
#else
    for (int i = 0; i < (int)ctrl->block_count; i++)
    {
        void *block = yaa_block_ptr(ctrl, ctrl->block_size, i);
        yaa_err_t err = yaa_queue_post_item(ctrl->queue, &block, 0);
        if (err != YAA_ERR_OK)
        {
            return;
        }
    }
#endif
}

/**
 * Release resources for dynamic allocation
 */
static void yaa_mempool_pool_destroy(yaa_mempool_struct_t *ctrl)
{
    if (ctrl->pool != NULL)
    {
        yaa_free(ctrl->pool);
    }
}

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

/**
 * Create and Initialize a Memory Pool object.
 */
uint8_t yaa_mempool_create(uint32_t block_count, uint32_t block_size, yaa_mempool_t *handle)
{
    yaa_err_t ret;
    yaa_mempool_struct_t *ctrl = NULL;

    if (block_count == 0 || block_size == 0 || handle == 0)
    {
        return YAA_ERR_BADARG;
    }

    // Calculate size of control block
    size_t ctrl_size = sizeof(yaa_mempool_struct_t);
#ifndef YAA_MEMPOOL_USE_QUEUE
    ctrl_size += YAA_MEMPOOL_BIT_ARRAY_SIZE(block_count) * sizeof(uint32_t);
#endif

    ctrl = (yaa_mempool_struct_t *)yaa_alloc(ctrl_size);

    if (ctrl == NULL)
    {
        return YAA_ERR_NOMEM;
    }

    memset(ctrl, 0, ctrl_size);

    const size_t alignment = YAA_MEMPOOL_ALIGNMENT_DEFAULT;
    ctrl->block_size = yaa_size_round_up(block_size, alignment);
    ctrl->block_count = block_count;

#ifdef YAA_MEMPOOL_USE_QUEUE
    ret = yaa_queue_create(&ctrl->queue, ctrl->block_count, sizeof(void *));
    if (ret != YAA_ERR_OK)
    {
        yaa_free(ctrl);
        return ret;
    }
#else
    ret = yaa_mutex_create(&ctrl->mutex);
    if (ret != YAA_ERR_OK)
    {
        return ret;
    }

    ret = yaa_sem_create(&ctrl->sem, 0, 1);
    if (ret != YAA_ERR_OK)
    {
        return ret;
    }

#endif
    ret = yaa_mempool_pool_create(ctrl);
    if (ret != YAA_ERR_OK)
    {
#ifdef YAA_MEMPOOL_USE_QUEUE
        yaa_queue_destroy(ctrl->queue);
#else
        yaa_sem_destroy(ctrl->sem);
        yaa_mutex_destroy(ctrl->mutex);
#endif
        yaa_free((uint8_t *)ctrl);
        return ret;
    }

    yaa_mempool_pool_init(ctrl);

    *handle = (yaa_mempool_t)ctrl;

    return YAA_ERR_OK;
}

/**
 * Allocate a memory block from a Memory Pool.
 */
void *yaa_mempool_alloc(yaa_mempool_t handle, int32_t timeout)
{
    yaa_mempool_struct_t *ctrl = (yaa_mempool_struct_t *)handle;

    if (ctrl == NULL)
    {
        return NULL;
    }

#ifdef YAA_MEMPOOL_USE_QUEUE
    void *ptr = NULL;
    if (yaa_is_isr())
    {
        if (yaa_queue_get_item_from_isr(ctrl->queue, &ptr) == YAA_ERR_OK)
        {
            return ptr;
        }
    }
    else
    {
        if (yaa_queue_get_item(ctrl->queue, &ptr, timeout) == YAA_ERR_OK)
        {
            return ptr;
        }
    }
#else
    int32_t end = timeout + yaa_systemtime();

    mempool_lock(ctrl, YAA_TIMO_FOREVER);
    while (true)
    {
        int allocation_index = yaa_find_first_clear(ctrl->bits, ctrl->block_count);
        if (allocation_index >= 0 && allocation_index < (int)ctrl->block_count)
        {
            void *ptr = yaa_block_ptr(ctrl, ctrl->block_size, allocation_index);
            yaa_set_alloc_bit(ctrl, allocation_index);
            mempool_unlock(ctrl);
            return ptr;
        }
        if (timeout == YAA_TIMO_NOWAIT)
        {
            /* don't wait for a free block to become available */
            mempool_unlock(ctrl);
            return NULL;
        }
        mempool_unlock(ctrl);
        /* wait for a free block or timeout */
        if (mempool_sem_wait(ctrl, timeout) == YAA_ERR_TIMEOUT)
        {
            return NULL;
        }
        mempool_lock(ctrl, YAA_TIMO_FOREVER);
        if (timeout != (int32_t)YAA_TIMO_FOREVER)
        {
            timeout = end - yaa_systemtime();
            if (timeout <= 0)
            {
                break;
            }
        }
    }

    mempool_unlock(ctrl);

#endif

    return NULL;
}

/**
 * Return an allocated memory block back to a Memory Pool.
 */
uint8_t yaa_mempool_free(yaa_mempool_t handle, void *block)
{
    yaa_mempool_struct_t *ctrl = (yaa_mempool_struct_t *)handle;

    if (ctrl == NULL || block == NULL)
    {
        return YAA_ERR_BADARG;
    }
#ifdef YAA_MEMPOOL_USE_QUEUE
    int block_num = yaa_block_num(ctrl, block, ctrl->block_size);

    if (block_num >= 0 && block_num < (int)ctrl->block_count)
    {
        yaa_err_t err = YAA_ERR_OK;
        if (yaa_is_isr())
        {
            err = yaa_queue_post_item_from_isr(ctrl->queue, &block);
        }
        else
        {
            err = yaa_queue_post_item(ctrl->queue, &block, YAA_TIMO_NOWAIT);
        }

        if (err != YAA_ERR_OK)
        {
            return err;
        }
    }
    else
    {
        return YAA_ERR_NOTFOUND;
    }
#else
    mempool_lock(ctrl, YAA_TIMO_FOREVER);

    int block_num = yaa_block_num(ctrl, block, ctrl->block_size);

    if (block_num >= 0 && block_num < (int)ctrl->block_count)
    {
        /* Detect common double-free occurrences */
        if (!yaa_alloc_bit_is_set(ctrl, block_num))
        {
            mempool_unlock(ctrl);
            return YAA_ERR_FAIL;
        }

        /* Put it back */
        yaa_clear_alloc_bit(ctrl, block_num);
        /* Notify, there is a free block */
        mempool_sem_post(ctrl);
    }
    else
    {
        mempool_unlock(ctrl);
        return YAA_ERR_NOTFOUND;
    }

    mempool_unlock(ctrl);
#endif
    return YAA_ERR_OK;
}

/**
 * Get maximum number of memory blocks in a Memory Pool.
 */
uint32_t yaa_mempool_get_capacity(yaa_mempool_t handle)
{
    yaa_mempool_struct_t *ctrl = (yaa_mempool_struct_t *)handle;

    if (ctrl == NULL)
    {
        return 0;
    }
    else
    {
#ifdef YAA_MEMPOOL_USE_QUEUE
        return yaa_queue_get_queue_size(ctrl->queue);
#else
        uint32_t num = 0;
        mempool_lock(ctrl, YAA_TIMO_FOREVER);
        num = ctrl->block_count;
        mempool_unlock(ctrl);
        return num;
#endif
    }
}

/**
 * Get memory block size in a Memory Pool.
 */
uint32_t yaa_mempool_get_block_size(yaa_mempool_t handle)
{
    yaa_mempool_struct_t *ctrl = (yaa_mempool_struct_t *)handle;

    if (ctrl == NULL)
    {
        return 0;
    }
    else
    {
        uint32_t size = 0;
#ifdef YAA_MEMPOOL_USE_QUEUE
        size = ctrl->block_size;
#else
        mempool_lock(ctrl, YAA_TIMO_FOREVER);
        size = ctrl->block_size;
        mempool_unlock(ctrl);
#endif
        return size;
    }
}

/**
 * Get number of memory blocks used in a Memory Pool.
 */
uint32_t yaa_mempool_get_count(yaa_mempool_t handle)
{
    yaa_mempool_struct_t *ctrl = (yaa_mempool_struct_t *)handle;

    if (ctrl == NULL)
    {
        return 0;
    }
    else
    {
#ifdef YAA_MEMPOOL_USE_QUEUE
        return yaa_queue_get_free_space(ctrl->queue);
#else
        uint32_t num = 0;
        mempool_lock(ctrl, YAA_TIMO_FOREVER);
        for (uint32_t i = 0; i < ctrl->block_count; i++)
        {
            if (yaa_alloc_bit_is_set(ctrl, i))
            {
                num++;
            }
        }
        mempool_unlock(ctrl);
        return num;
#endif
    }
}

/**
 * Get number of memory blocks available in a Memory Pool.
 */
uint32_t yaa_mempool_get_space(yaa_mempool_t handle)
{
    yaa_mempool_struct_t *ctrl = (yaa_mempool_struct_t *)handle;

    if (ctrl == NULL)
    {
        return 0;
    }
    else
    {
#ifdef YAA_MEMPOOL_USE_QUEUE
        return (ctrl->block_count - yaa_queue_get_free_space(ctrl->queue));
#else
        uint32_t num = 0;
        mempool_lock(ctrl, YAA_TIMO_FOREVER);
        for (uint32_t i = 0; i < ctrl->block_count; i++)
        {
            if (!yaa_alloc_bit_is_set(ctrl, i))
            {
                num++;
            }
        }
        mempool_unlock(ctrl);
        return num;
#endif
    }
}

/**
 * Delete a Memory Pool object.
 */
uint8_t yaa_mempool_destroy(yaa_mempool_t handle)
{
    yaa_mempool_struct_t *ctrl = (yaa_mempool_struct_t *)handle;

    if (ctrl == NULL)
    {
        return YAA_ERR_BADARG;
    }
    else
    {
#ifdef YAA_MEMPOOL_USE_QUEUE
        if (yaa_queue_get_free_space(ctrl->queue) != 0)
        {
            return YAA_ERR_BUSY;
        }
#else
        mempool_lock(ctrl, YAA_TIMO_FOREVER);

        // Check all objects are free
        if (yaa_find_first_set(ctrl->bits, ctrl->block_count) != -1)
        {
            mempool_unlock(ctrl);
            return YAA_ERR_BUSY;
        }
#endif
        yaa_mempool_pool_destroy(ctrl);
#ifdef YAA_MEMPOOL_USE_QUEUE
        yaa_queue_destroy(ctrl->queue);
#else
        yaa_mutex_destroy(ctrl->mutex);
        yaa_sem_destroy(ctrl->sem);
#endif
        yaa_free((uint8_t *)ctrl);

        return YAA_ERR_OK;
    }
}
