/**
 * @file yaa_ring.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h>
#include <string.h>

/* Core includes. */
#include "yaa_ring.h"
#include "yaa_types.h"

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

#pragma GCC push_options
#pragma GCC optimize("O3")

yaa_err_t yaa_ring_init(yaa_ring_t *ring, uint32_t max_elements, size_t element_size, void *storage)
{
    if ((ring != NULL) && (storage != NULL))
    {
        (void)memset(storage, 0, element_size * max_elements);
        ring->element_size = element_size;
        ring->max_elements = max_elements;
        ring->first = (uintptr_t)NULL;
        ring->next = (uintptr_t)storage;
        ring->storage = storage;
        return YAA_ERR_OK;
    }
    else
    {
        return YAA_ERR_FAIL;
    }
}

volatile void *yaa_ring_get_next_enqueue(yaa_ring_t *ring)
{
    return (ring != NULL) ? (void *)(ring->next) : NULL;
}

uint8_t yaa_ring_advance_next(yaa_ring_t *ring)
{
    if (ring->first != ring->next)
    {
        uintptr_t storage_end = (uintptr_t)(ring->storage) + (ring->element_size * ring->max_elements);
        ring->first = (ring->first == 0U) ? ring->next : ring->first;
        ring->next = ((ring->next + ring->element_size) == storage_end) ? (uintptr_t)ring->storage
                                                                        : (ring->next + ring->element_size);
        return YAA_ERR_OK;
    }
    else
    {
        return YAA_ERR_NORESOURCE;
    }
}

uint8_t yaa_ring_enqueue(yaa_ring_t *ring, void *element)
{
    if (ring->first != ring->next)
    {
        (void)memmove((void *)ring->next, (void *)element, ring->element_size);
        return yaa_ring_advance_next(ring);
    }
    return YAA_ERR_NORESOURCE;
}

uint8_t yaa_ring_dequeue(yaa_ring_t *ring, void *element)
{
    void *x = yaa_ring_dequeue_fast(ring);
    if (x != NULL)
    {
        if (element)
        {
            (void)memmove((void *)element, x, ring->element_size);
        }
        return YAA_ERR_OK;
    }
    else
    {
        return YAA_ERR_NORESOURCE;
    }
}

void *yaa_ring_dequeue_fast(yaa_ring_t *ring)
{
    if (ring == NULL)
    {
        return NULL;
    }

    if (ring->first != 0U)
    {
        void *ret_ptr = (void *)ring->first;

        uintptr_t storage_end = (uintptr_t)ring->storage + (ring->element_size * ring->max_elements);
        uintptr_t next_first = ring->first + ring->element_size;

        ring->first = (next_first == storage_end) ? (uintptr_t)ring->storage : next_first;
        ring->first = (ring->first == ring->next) ? 0U : ring->first;

        return ret_ptr;
    }

    return NULL;
}

uint8_t yaa_ring_size(yaa_ring_t *ring, size_t *_size)
{
    *_size = ring->first == 0U
                 ? 0U
                 : (ring->first < ring->next ? (ring->next - ring->first) / ring->element_size
                                             : ring->max_elements - ((ring->first - ring->next) / ring->element_size));

    return YAA_ERR_OK;
}

#pragma GCC pop_options
