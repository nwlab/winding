/**
 * @file yaa_ring.h
 * @author Software development team
 * @brief Ring storage APIs
 * @version 1.0
 * @date 2024-10-06
 */

#ifndef YAA_RING_H
#define YAA_RING_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <inttypes.h>
#include <stdint.h>

/* Core includes. */
#include <yaa_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

typedef struct yaa_ring
{
    volatile void *storage;
    volatile uintptr_t first;
    volatile uintptr_t next;
    volatile size_t element_size;
    volatile uint32_t max_elements;
} yaa_ring_t;

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

yaa_err_t yaa_ring_init(yaa_ring_t *ring, uint32_t max_elements, size_t element_size, void *storage);
uint8_t yaa_ring_enqueue(yaa_ring_t *ring, void *element);
uint8_t yaa_ring_dequeue(yaa_ring_t *ring, void *element);
uint8_t yaa_ring_size(yaa_ring_t *ring, size_t *size);
uint8_t yaa_ring_advance_next(yaa_ring_t *ring);
volatile void *yaa_ring_get_next_enqueue(yaa_ring_t *ring);
void *yaa_ring_dequeue_fast(yaa_ring_t *ring);

#ifdef __cplusplus
}
#endif

#endif // YAA_RING_H
