/**
 * @file rdnx_ring.h
 * @author Software development team
 * @brief Ring storage APIs
 * @version 1.0
 * @date 2024-10-06
 */

#ifndef RDNX_RING_H
#define RDNX_RING_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <inttypes.h>
#include <stdint.h>

/* Core includes. */
#include <rdnx_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

typedef struct rdnx_ring
{
    volatile void *storage;
    volatile uintptr_t first;
    volatile uintptr_t next;
    volatile size_t element_size;
    volatile uint32_t max_elements;
} rdnx_ring_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

rdnx_err_t rdnx_ring_init(rdnx_ring_t *ring, uint32_t max_elements, size_t element_size, void *storage);
uint8_t rdnx_ring_enqueue(rdnx_ring_t *ring, void *element);
uint8_t rdnx_ring_dequeue(rdnx_ring_t *ring, void *element);
uint8_t rdnx_ring_size(rdnx_ring_t *ring, size_t *size);
uint8_t rdnx_ring_advance_next(rdnx_ring_t *ring);
volatile void *rdnx_ring_get_next_enqueue(rdnx_ring_t *ring);
void *rdnx_ring_dequeue_fast(rdnx_ring_t *ring);

#ifdef __cplusplus
}
#endif

#endif // RDNX_RING_H
