/**
 * @file rdnx_queue.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stddef.h>
#include <string.h>

/* Scheduler includes. */
#include <FreeRTOS.h>
#include <queue.h>

/* Core includes. */
#include "rdnx_queue.h"
#include "rdnx_sal.h"
#include "rdnx_types.h"

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

typedef struct rdnx_queue
{
    QueueHandle_t os_queue;
} rdnx_queue_t;

/* ============================================================================
 * Private Function Definitions
 * ==========================================================================*/

static inline QueueHandle_t queue_handle_from_rdnx(rdnx_queue_handle_t s)
{
    return s->os_queue;
}

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint8_t rdnx_queue_create(rdnx_queue_handle_t *queue, uint32_t queue_depth, uint32_t queue_item_size)
{

    uint8_t err = RDNX_ERR_OK;
    rdnx_queue_t *q = NULL;

    if (queue == NULL || queue_depth == 0 || queue_item_size == 0)
    {
        err = RDNX_ERR_BADARG;
    }
    else if ((q = rdnx_alloc(sizeof(*q))) == NULL)
    {
        err = RDNX_ERR_NOMEM;
    }
    else
    {
        memset(q, 0, sizeof(*q));
        q->os_queue = xQueueCreate(queue_depth, queue_item_size);

        if (q->os_queue != NULL)
        {
            *queue = (rdnx_queue_handle_t)q;
            err = RDNX_ERR_OK;
        }
        else
        {
            rdnx_free(q);
            err = RDNX_ERR_NORESOURCE;
        }
    }

    return err;
}

uint8_t rdnx_queue_destroy(rdnx_queue_handle_t queue)
{
    uint8_t err = RDNX_ERR_FAIL;

    if (queue == NULL)
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        vQueueDelete(queue_handle_from_rdnx(queue));

        rdnx_free(queue);
        err = RDNX_ERR_OK;
    }

    return err;
}

uint8_t rdnx_queue_post_item(rdnx_queue_handle_t queue, const void *item, uint32_t timeout)
{
    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)RDNX_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueueSend(queue_handle_from_rdnx(queue), ((void *)item), wait_ticks) != pdPASS)
        {
            err = RDNX_ERR_FAIL;
        }
    }

    return err;
}

uint8_t rdnx_queue_post_item_from_isr(rdnx_queue_handle_t queue, const void *item)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        if (xQueueSendFromISR(queue_handle_from_rdnx(queue), ((void *)item), &xHigherPriorityTaskWoken) != pdTRUE)
        {
            err = RDNX_ERR_FAIL;
        }
    }
    return err;
}

uint8_t rdnx_queue_post_item_to_back(rdnx_queue_handle_t queue, const void *item, uint32_t timeout)
{

    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)RDNX_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueueSendToBack(queue_handle_from_rdnx(queue), ((void *)item), wait_ticks) != pdPASS)
        {
            err = RDNX_ERR_FAIL;
        }
    }

    return err;
}

uint8_t rdnx_queue_post_item_to_front(rdnx_queue_handle_t queue, const void *item, uint32_t timeout)
{
    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)RDNX_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueueSendToFront(queue_handle_from_rdnx(queue), ((void *)item), wait_ticks) != pdPASS)
        {
            err = RDNX_ERR_FAIL;
        }
    }
    return err;
}

uint8_t rdnx_queue_post_item_to_back_from_isr(rdnx_queue_handle_t queue, const void *item)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        if (xQueueSendToBackFromISR(queue_handle_from_rdnx(queue), ((void *)item), &xHigherPriorityTaskWoken) !=
            pdTRUE)
        {
            err = RDNX_ERR_FAIL;
        }
    }

    return err;
}

uint8_t rdnx_queue_post_item_to_front_from_isr(rdnx_queue_handle_t queue, const void *item)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        if (xQueueSendToFrontFromISR(queue_handle_from_rdnx(queue), ((void *)item), &xHigherPriorityTaskWoken) !=
            pdTRUE)
        {
            err = RDNX_ERR_FAIL;
        }
    }

    return err;
}

uint8_t rdnx_queue_get_item(rdnx_queue_handle_t queue, void *ptrData, uint32_t timeout)
{
    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (ptrData == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)RDNX_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueueReceive(queue_handle_from_rdnx(queue), ptrData, wait_ticks) != pdPASS)
        {
            err = RDNX_ERR_FAIL;
        }
    }

    return err;
}

uint8_t rdnx_queue_get_item_from_isr(rdnx_queue_handle_t queue, void *ptrData)
{
    uint8_t err = RDNX_ERR_OK;
    BaseType_t xTaskWokenByReceive = pdFALSE;

    if ((queue == NULL) || (ptrData == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        if (xQueueReceiveFromISR(queue_handle_from_rdnx(queue), ptrData, &xTaskWokenByReceive) != pdTRUE)
        {
            err = RDNX_ERR_FAIL;
        }
    }

    return err;
}

uint8_t rdnx_queue_peek_item(rdnx_queue_handle_t queue, void *ptrData, uint32_t timeout)
{
    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (ptrData == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)RDNX_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueuePeek(queue_handle_from_rdnx(queue), ptrData, wait_ticks) != pdPASS)
        {
            err = RDNX_ERR_FAIL;
        }
    }

    return err;
}

uint8_t rdnx_queue_peek_item_from_isr(rdnx_queue_handle_t queue, void *ptrData)
{
    uint8_t err = RDNX_ERR_OK;

    if ((queue == NULL) || (ptrData == NULL))
    {
        err = RDNX_ERR_BADARG;
    }
    else
    {
        if (xQueuePeekFromISR(queue_handle_from_rdnx(queue), ptrData) != pdPASS)
        {
            err = RDNX_ERR_FAIL;
        }
    }

    return err;
}

uint32_t rdnx_queue_get_queue_size(rdnx_queue_handle_t queue)
{
    uint32_t res = 0;

    if (queue != NULL)
    {
        res = uxQueueMessagesWaiting(queue_handle_from_rdnx(queue));
    }

    return res;
}

uint32_t rdnx_queue_get_free_space(rdnx_queue_handle_t queue)
{
    uint32_t res = 0;

    if (queue != NULL)
    {
        res = uxQueueSpacesAvailable(queue_handle_from_rdnx(queue));
    }

    return res;
}
