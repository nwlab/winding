/**
 * @file yaa_event.c
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Scheduler includes. */
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

/* Core includes. */
#include <yaa_event.h>
#include <yaa_macro.h>
#include <yaa_queue.h>
#include <yaa_sal.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Type Definitions
 * ==========================================================================*/

/**
 * @brief Handler function registration for a single event type
 */
typedef struct yaa_event_registration
{
    uint32_t event_type;
    yaa_event_handler_t fn;
    void *context;
} yaa_event_registration_t;

/**
 * @brief One node in a singly linked-list of #yaa_event_registration_t
 * structures
 */
typedef struct yaa_event_registration_node
{
    struct yaa_event_registration_node *next;
    yaa_event_registration_t reg;
} yaa_event_registration_node_t;

/**
 * @brief A linked list of #yaa_event_registration_t structures
 */
typedef struct yaa_event_registration_list_t
{
    yaa_event_registration_node_t *head;
} yaa_event_registration_list_t;

/**
 * @brief An event queue's data
 */
typedef struct yaa_event_queue_t
{
    /** @brief Mutex to protect the contents of this structure */
    yaa_mutex_t mutex;

    /**
     * @brief Semaphore for event-get calls to block against when the queue is
     *        empty
     */
    SemaphoreHandle_t sem;

    /**
     * @brief The maximum value the semaphore may count up to.
     */
    uint32_t sem_amount;

    /**
     * @brief Semaphore for push timeout
     */
    yaa_sem_t push_sem;

    /**
     * @brief Hash table of event registrations
     *
     * This array forms a simple hash table.  The event type mod 16 is used to
     * identify one of 16 lists.  Each list contains all correspondng event
     * registrations.
     *
     * This system should be efficient if the event types are numbered
     * sequentially and the number of event types isn't large (e.g. 64 or
     * less).
     *
     * If we find that we need to register large numbers of event handlers,
     * then we should be able to replace this structure with minimal changes to
     * the rest of this library
     */
    yaa_event_registration_list_t registration[16];

    /**
     * @brief System queue used to hold queued events
     */
    yaa_queue_handle_t queue;
} yaa_event_queue_t;

/* ============================================================================
 * Internal utility functions
 * ==========================================================================*/

/* Determine whether we are in thread mode or handler mode. */
YAA_STATIC_INLINE int in_handler_mode(void)
{
    return __get_IPSR() != 0;
}

/**
 * @brief Lock an event queue's mutex
 */
YAA_STATIC_INLINE yaa_err_t event_queue_lock(yaa_event_queue_t *q, uint32_t timeout)
{
    return yaa_mutex_lock(q->mutex, timeout);
}

/**
 * @brief Unlock an event queue's mutex
 */
YAA_STATIC_INLINE yaa_err_t event_queue_unlock(yaa_event_queue_t *q)
{
    return yaa_mutex_unlock(q->mutex);
}

/**
 * @brief Take (possibly blocking on) an event queue's semaphore
 */
static yaa_err_t event_queue_sem_wait(yaa_event_queue_t *q, uint32_t timeout)
{
    BaseType_t ret;
    const TickType_t wait_ticks =
        (timeout == (uint32_t)YAA_TIMO_FOREVER) ? portMAX_DELAY : (timeout / portTICK_PERIOD_MS);

    ret = xSemaphoreTake(q->sem, wait_ticks);
    if (pdTRUE == ret)
    {
        return YAA_ERR_OK;
    }

    return YAA_ERR_TIMEOUT;
}

/**
 * @brief Give an event queue's semaphore, possibly unblocking other threads
 */
static void event_queue_sem_post(yaa_event_queue_t *q)
{
    (void)xSemaphoreGive(q->sem);
}

/**
 * @brief Give an event queue's semaphore, possibly unblocking other threads
 */
static void event_queue_sem_post_from_isr(yaa_event_queue_t *q)
{
    BaseType_t higher_priority_task_woken = pdFALSE;

    xSemaphoreGiveFromISR(q->sem, &higher_priority_task_woken);
}

/**
 * @brief Get an event handler registration from an event queue
 *
 * This function assumes the caller has already locked the queue
 *
 * @param q          The event queue to look in
 * @param event_type The event type to look up
 * @return           The registration, if found.  NULL if not found.
 */
static yaa_event_registration_t *event_queue_get_registration(yaa_event_queue_t *q, uint32_t event_type)
{
    /*
     * Get a hash from the event type.  Then walk the corresponding list to
     * find the registration.  Return what we find.
     */
    int hash = event_type & 0x0F;
    yaa_event_registration_node_t *node = q->registration[hash].head;
    while (node)
    {
        if (node->reg.event_type == event_type)
        {
            // Found the node.  Return its registration
            return &node->reg;
        }

        node = node->next;
    }

    return NULL;
}

/**
 * @brief Remove an event handler registration from an event queue
 *
 * This function assumes the caller has already locked the queue
 *
 * @param q          The event queue to look in
 * @param event_type The event type to remove
 */
static void event_queue_delete_registration(yaa_event_queue_t *q, uint32_t event_type)
{
    /*
     * Get a hash from the event type.  Then walk the corresponding list to
     * find the registration.  Delete what we find.
     */
    int hash = event_type & 0x0F;
    yaa_event_registration_list_t *bucket = q->registration + hash;
    yaa_event_registration_node_t *node = bucket->head;
    yaa_event_registration_node_t *previous = NULL;

    while (node)
    {
        yaa_event_registration_node_t *next = node->next;

        if (node->reg.event_type == event_type)
        {
            // Found the node.  Delete it, but keep on walking the list in
            // order to handle the degenerate case of an event type appearing
            // more than once

            if (node == bucket->head)
            {
                bucket->head = next;
            }
            else
            {
                previous->next = next;
            }

            yaa_free(node);
        }
        else
        {
            previous = node;
        }

        node = next;
    }
}

/**
 * @brief Store an event handler registration in the event queue
 *
 * If the registration does not exist, one will be created.  If the
 * registration exists, it will be replaced.  If the function pointer, `fn` is
 * NULL, the registration will be removed
 *
 * This function assumes the caller has already locked the queue
 *
 * @param q          The event queue
 * @param event_type The event type
 * @param fn         The handler function, or NULL to deregister the handler
 * @param context    The handler's context pointer
 * @return           #YAA_ERR_OK on success
 */
static yaa_err_t event_queue_set_registration(yaa_event_queue_t *q, uint32_t event_type, yaa_event_handler_t fn,
                                               void *context)
{

    if (!fn)
    {
        event_queue_delete_registration(q, event_type);
        return YAA_ERR_OK;
    }

    /* Look up any existing registration.  Replace the contents, if found*/
    yaa_event_registration_t *reg = event_queue_get_registration(q, event_type);
    if (reg)
    {
        reg->fn = fn;
        reg->context = context;

        event_queue_unlock(q);
        return YAA_ERR_OK;
    }

    /*
     * Create and populate a new registration node
     */
    yaa_event_registration_node_t *node =
        (yaa_event_registration_node_t *)yaa_alloc(sizeof(yaa_event_registration_node_t));
    if (!node)
    {
        event_queue_unlock(q);
        return YAA_ERR_NOMEM;
    }
    node->reg.event_type = event_type;
    node->reg.fn = fn;
    node->reg.context = context;

    /*
     * Get a hash from the event type.  Then add the node to the corresponding
     * list.
     */
    int hash = event_type & 0x0F;
    yaa_event_registration_list_t *bucket = q->registration + hash;
    node->next = bucket->head;
    bucket->head = node;

    return YAA_ERR_OK;
}

/**************************************************************************
 *
 * Public functions
 */

yaa_err_t yaa_event_queue_create(yaa_event_queue_handle_t *queue, uint32_t sem_amount)
{
    yaa_err_t err;

    /* Validate parameters */
    if (queue == NULL)
    {
        return YAA_ERR_BADARG;
    }

    /* Allocate the event queue */
    yaa_event_queue_t *q = (yaa_event_queue_t *)yaa_alloc(sizeof(yaa_event_queue_t));
    if (q == NULL)
    {
        return YAA_ERR_NOMEM;
    }
    memset(q, 0, sizeof(*q));

    /* Create the mutex to protect the event queue's internal data */
    err = yaa_mutex_create(&q->mutex);
    if (err != YAA_ERR_OK)
    {
        yaa_free(q);
        return err;
    }

    /* Create the semaphore for blocking on when pushing to a full queue */
    err = yaa_sem_create(&q->push_sem, 0, sem_amount);
    if (err != YAA_ERR_OK)
    {
        yaa_mutex_destroy(q->mutex);
        yaa_free(q);
        return err;
    }

    /* Create the semaphore for blocking on when getting from an empty queue */
    q->sem = xSemaphoreCreateCounting(sem_amount, 0);

    q->sem_amount = sem_amount;

    /* Create the system queue for holding the actual event data */
    if ((err = yaa_queue_create(&q->queue, sem_amount, sizeof(yaa_event_t))) != YAA_ERR_OK)
    {
        yaa_sem_destroy(q->push_sem);
        vSemaphoreDelete(q->sem);
        yaa_mutex_destroy(q->mutex);
        yaa_free(q);
        return err;
    }

    *queue = q;
    return YAA_ERR_OK;
}

yaa_err_t yaa_event_queue_destroy(yaa_event_queue_handle_t queue)
{
    if (queue == NULL)
    {
        // It's not an error to destroy NULL
        return YAA_ERR_OK;
    }

    /*
     * The event_queue_lock() it is an internal mutex for the event queue.
     * This mutex is protecting the event queue from multiply access, so it can
     * not be blocked forever and we don`t need to separate timeout for it.
     */
    event_queue_lock(queue, YAA_TIMO_FOREVER);

    yaa_queue_destroy(queue->queue);

    // Purge the queue of all registrations
    int hash;
    for (hash = 0; hash < 16; ++hash)
    {
        yaa_event_registration_list_t *bucket = &queue->registration[hash];
        while (bucket->head)
        {
            yaa_event_registration_node_t *node = bucket->head;
            bucket->head = node->next;
            yaa_free(node);
        }
    }

    vSemaphoreDelete(queue->sem);
    yaa_sem_destroy(queue->push_sem);
    event_queue_unlock(queue);
    yaa_mutex_destroy(queue->mutex);
    yaa_free(queue);

    return YAA_ERR_OK;
}

yaa_err_t yaa_event_register_handler(yaa_event_queue_handle_t queue, uint32_t event_type,
                                       yaa_event_handler_t handler_function, void *context)
{
    if (queue == NULL)
    {
        return YAA_ERR_BADARG;
    }

    yaa_err_t err = YAA_ERR_OK;

    err = event_queue_lock(queue, YAA_TIMO_FOREVER);
    if (err == YAA_ERR_OK)
    {
        err = event_queue_set_registration(queue, event_type, handler_function, context);
    }
    event_queue_unlock(queue);

    return err;
}

yaa_err_t yaa_event_post(yaa_event_queue_handle_t queue, uint32_t event_type, uint32_t iarg1, uint32_t iarg2,
                           void *parg, uint32_t timeout)
{
    yaa_err_t err;

    if (queue == NULL)
    {
        return YAA_ERR_BADARG;
    }

    event_queue_lock(queue, YAA_TIMO_FOREVER);

    /*
     * If the queue is full, block until space becomes available or until
     * the timeout elapses.  Be sure we aren't holding the mutex while
     * blocking or we'll prevent other threads from dequeueing events,
     * causing a deadlock.
     */

    /*
     * We only have to be sure that we're not holding the event queue's lock
     * when we make the call.  But that should be fine.  We can copy the
     * necessary variables (queue->queue) to local variables before releasing
     * the lock, then reacquire it before posting its semaphore.
     */
    if (yaa_event_get_queue_size(queue) >= queue->sem_amount)
    {
        event_queue_unlock(queue);
        if (yaa_sem_take(queue->push_sem, timeout) != YAA_ERR_OK)
        {
            return YAA_ERR_FAIL;
        }
        event_queue_lock(queue, YAA_TIMO_FOREVER);
    }

    yaa_event_t ev = { 0 };

    ev.event_type = event_type;
    ev.iarg1 = iarg1;
    ev.iarg2 = iarg2;
    ev.parg = parg;

    err = yaa_queue_post_item(queue->queue, &ev, timeout);
    if (err == YAA_ERR_OK)
    {
        // Notify any blocking thread that there's an event
        event_queue_sem_post(queue);
    }

    event_queue_unlock(queue);
    return err;
}

yaa_err_t yaa_event_post_from_isr(yaa_event_queue_handle_t queue, uint32_t event_type, uint32_t iarg1,
                                    uint32_t iarg2, void *parg)
{
    yaa_err_t err;

    if (queue == NULL)
    {
        err = YAA_ERR_BADARG;
    }
    else
    {
        yaa_event_t ev = { 0 };

        ev.event_type = event_type;
        ev.iarg1 = iarg1;
        ev.iarg2 = iarg2;
        ev.parg = parg;

        err = yaa_queue_post_item_from_isr(queue->queue, &ev);
        if (err == YAA_ERR_OK)
        {
            // Notify any blocking thread that there's an event
            event_queue_sem_post_from_isr(queue);
        }
    }
    return err;
}

yaa_err_t yaa_event_get(yaa_event_queue_handle_t queue, yaa_event_t *event, uint32_t timeout)
{
    yaa_err_t err = YAA_ERR_BADARG;

    if (queue == NULL)
    {
        return err;
    }

    if (event_queue_sem_wait(queue, timeout) == YAA_ERR_OK)
    {
        (void)yaa_sem_give(queue->push_sem);
        err = yaa_queue_get_item(queue->queue, event, timeout);
        return err;
    }

    return YAA_ERR_TIMEOUT;
}

uint32_t yaa_event_get_queue_size(yaa_event_queue_handle_t queue)
{
    if (queue == NULL)
    {
        return 0;
    }

    return yaa_queue_get_queue_size(queue->queue);
}

uint32_t yaa_event_get_queue_free_space(yaa_event_queue_handle_t queue)
{
    if (queue == NULL)
    {
        return 0;
    }

    return yaa_queue_get_free_space(queue->queue);
}

yaa_err_t yaa_event_dispatch(yaa_event_queue_handle_t queue, yaa_event_t *event)
{
    if ((queue == NULL) || (event == NULL))
    {
        return YAA_ERR_BADARG;
    }

    /*
     * Look up the registration.  First look for an exact match.  If we don't
     * find one, then look for 0 (the default handler)
     */
    event_queue_lock(queue, YAA_TIMO_FOREVER);

    const yaa_event_registration_t *reg;
    reg = event_queue_get_registration(queue, event->event_type);
    if (!reg)
    {
        reg = event_queue_get_registration(queue, 0);
    }

    /*
     * Copy the registration data to a local copy before releasing the mutex.
     * Otherwise there's the possibility of a race condition where another
     * thread could change it while we're accessing it and that would be bad.
     */
    yaa_event_registration_t r;
    if (reg)
    {
        r = *reg;
    }

    /*
     * It is critical to release the mutex before calling the handler
     * function.
     *
     * We don't know how long the handler function will take to complete and we
     * don't want to block other threads during this time.
     *
     * Furthermore, the handler function may try to access the event library
     * (e.g. post new events), and we definitely don't want that to result in a
     * deadlock.
     */
    event_queue_unlock(queue);

    if (reg)
    {
        r.fn(event->event_type, event->iarg1, event->iarg2, event->parg, r.context);
    }

    return YAA_ERR_OK;
}
