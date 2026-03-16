/**
 * @file yaa_queue.c
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
#include "yaa_queue.h"
#include "yaa_sal.h"
#include "yaa_types.h"

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

typedef struct yaa_queue
{
    QueueHandle_t os_queue;
} yaa_queue_t;

/* ============================================================================
 * Private Function Definitions
 * ==========================================================================*/

static inline QueueHandle_t queue_handle_from_yaa(yaa_queue_handle_t s)
{
    return s->os_queue;
}

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

uint8_t yaa_queue_create(yaa_queue_handle_t *queue, uint32_t queue_depth, uint32_t queue_item_size)
{

    uint8_t err = YAA_ERR_OK;
    yaa_queue_t *q = NULL;

    if (queue == NULL || queue_depth == 0 || queue_item_size == 0)
    {
        err = YAA_ERR_BADARG;
    }
    else if ((q = yaa_alloc(sizeof(*q))) == NULL)
    {
        err = YAA_ERR_NOMEM;
    }
    else
    {
        memset(q, 0, sizeof(*q));
        q->os_queue = xQueueCreate(queue_depth, queue_item_size);

        if (q->os_queue != NULL)
        {
            *queue = (yaa_queue_handle_t)q;
            err = YAA_ERR_OK;
        }
        else
        {
            yaa_free(q);
            err = YAA_ERR_NORESOURCE;
        }
    }

    return err;
}

uint8_t yaa_queue_destroy(yaa_queue_handle_t queue)
{
    uint8_t err = YAA_ERR_FAIL;

    if (queue == NULL)
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        vQueueDelete(queue_handle_from_yaa(queue));

        yaa_free(queue);
        err = YAA_ERR_OK;
    }

    return err;
}

uint8_t yaa_queue_post_item(yaa_queue_handle_t queue, const void *item, uint32_t timeout)
{
    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)YAA_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueueSend(queue_handle_from_yaa(queue), ((void *)item), wait_ticks) != pdPASS)
        {
            err = YAA_ERR_FAIL;
        }
    }

    return err;
}

uint8_t yaa_queue_post_item_from_isr(yaa_queue_handle_t queue, const void *item)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        if (xQueueSendFromISR(queue_handle_from_yaa(queue), ((void *)item), &xHigherPriorityTaskWoken) != pdTRUE)
        {
            err = YAA_ERR_FAIL;
        }
    }
    return err;
}

uint8_t yaa_queue_post_item_to_back(yaa_queue_handle_t queue, const void *item, uint32_t timeout)
{

    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)YAA_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueueSendToBack(queue_handle_from_yaa(queue), ((void *)item), wait_ticks) != pdPASS)
        {
            err = YAA_ERR_FAIL;
        }
    }

    return err;
}

uint8_t yaa_queue_post_item_to_front(yaa_queue_handle_t queue, const void *item, uint32_t timeout)
{
    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)YAA_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueueSendToFront(queue_handle_from_yaa(queue), ((void *)item), wait_ticks) != pdPASS)
        {
            err = YAA_ERR_FAIL;
        }
    }
    return err;
}

uint8_t yaa_queue_post_item_to_back_from_isr(yaa_queue_handle_t queue, const void *item)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        if (xQueueSendToBackFromISR(queue_handle_from_yaa(queue), ((void *)item), &xHigherPriorityTaskWoken) !=
            pdTRUE)
        {
            err = YAA_ERR_FAIL;
        }
    }

    return err;
}

uint8_t yaa_queue_post_item_to_front_from_isr(yaa_queue_handle_t queue, const void *item)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (item == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        if (xQueueSendToFrontFromISR(queue_handle_from_yaa(queue), ((void *)item), &xHigherPriorityTaskWoken) !=
            pdTRUE)
        {
            err = YAA_ERR_FAIL;
        }
    }

    return err;
}

uint8_t yaa_queue_get_item(yaa_queue_handle_t queue, void *ptrData, uint32_t timeout)
{
    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (ptrData == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)YAA_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueueReceive(queue_handle_from_yaa(queue), ptrData, wait_ticks) != pdPASS)
        {
            err = YAA_ERR_FAIL;
        }
    }

    return err;
}

uint8_t yaa_queue_get_item_from_isr(yaa_queue_handle_t queue, void *ptrData)
{
    uint8_t err = YAA_ERR_OK;
    BaseType_t xTaskWokenByReceive = pdFALSE;

    if ((queue == NULL) || (ptrData == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        if (xQueueReceiveFromISR(queue_handle_from_yaa(queue), ptrData, &xTaskWokenByReceive) != pdTRUE)
        {
            err = YAA_ERR_FAIL;
        }
    }

    return err;
}

uint8_t yaa_queue_peek_item(yaa_queue_handle_t queue, void *ptrData, uint32_t timeout)
{
    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (ptrData == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        const TickType_t wait_ticks =
            (timeout == (uint32_t)YAA_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

        if (xQueuePeek(queue_handle_from_yaa(queue), ptrData, wait_ticks) != pdPASS)
        {
            err = YAA_ERR_FAIL;
        }
    }

    return err;
}

uint8_t yaa_queue_peek_item_from_isr(yaa_queue_handle_t queue, void *ptrData)
{
    uint8_t err = YAA_ERR_OK;

    if ((queue == NULL) || (ptrData == NULL))
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        if (xQueuePeekFromISR(queue_handle_from_yaa(queue), ptrData) != pdPASS)
        {
            err = YAA_ERR_FAIL;
        }
    }

    return err;
}

uint32_t yaa_queue_get_queue_size(yaa_queue_handle_t queue)
{
    uint32_t res = 0;

    if (queue != NULL)
    {
        res = uxQueueMessagesWaiting(queue_handle_from_yaa(queue));
    }

    return res;
}

uint32_t yaa_queue_get_free_space(yaa_queue_handle_t queue)
{
    uint32_t res = 0;

    if (queue != NULL)
    {
        res = uxQueueSpacesAvailable(queue_handle_from_yaa(queue));
    }

    return res;
}
