/**
 * @file yaa_event.h
 * @author Software development team
 * @brief Event queue APIs
 * @version 1.0
 * @date 2024-09-09
 */
#ifndef YAA_EVENT_H
#define YAA_EVENT_H

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>

/* Core includes. */
#include <yaa_queue.h>
#include <yaa_types.h>

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * @file yaa_event.h
 * @brief Event queue
 *
 * This module implements an event queue which may be used to drive any
 * event-driven application.  The implementation is thread-safe and may be used
 * in single- or multi-threaded applications.  Multiple queues may be created,
 * each with an independent set of registered event-handler functions.
 *
 * Every event consists of the following values:
 * * An event type (an integer)
 * * Two integer arguments
 * * A pointer argument
 *
 * Event types and the semantics for the arguments are defined by the
 * application.  Applications post events to an event queue for later
 * processing.
 *
 * Application software creates one or more event queues, as required to
 * implement the application's functionality.  The application registers event
 * handler callback functions with each queue, in order to process events
 * posted to that queue.  Each event type may have at most one handler function
 * registered at a time on each queue.  Additionally, each registration has a
 * `context` value that is opaquely passed to the event handler function.
 *
 * The application (typically as a part of a dedicated event-handling thread)
 * removes events from a queue and dispatches them to the queue's registered
 * handler functions.  After each event is handled, it is deleted, to prevent
 * memory leaks.
 *
 * Applications may create and destroy event queues as needed.  Each queue
 * maintains a registry of handler functions.  The code is thread-safe so any
 * thread may post, get and dispatch events to any queue.
 *
 * For multi-threaded applications, where an event queue can run in its own
 * dedicated thread, that thread typically runs a simple loop to get and
 * dispatch events:
 * ~~~~~~~~{.c}
 *     yaa_event_queue_handle_t queue;
 *     ...
 *     while (1)
 *     {
 *         uint8_t  err;
 *         yaa_event_t event;
 *
 *         err = yaa_event_get(queue, &event, YAA_TIMO_FOREVER);
 *         if (err == YAA_ERR_OK)
 *         {
 *             yaa_event_dispatch(queue, &event);
 *         }
 *     }
 * ~~~~~~~~
 * This pattern, however, can not be used for a single-threaded application or
 * from a thread that needs to process system events alongside application
 * events, because it blocks whenever the queue is empty.  For these
 * situations, a slightly more complicated pattern may be used:
 * ~~~~~~~~{.c}
 *     yaa_event_queue_handle_t queue;
 *     ...
 *     while (1)
 *     {
 *         if ((yaa_event_get_queue_size(queue) == 0) &&
 *             (system_queue_size == 0))
 *         {
 *             block_on_system_queue();
 *         }
 *
 *         if (system_queue_size > 0)
 *         {
 *             sev = get_system_event();
 *             dispatch_system_event();
 *             continue;
 *         }
 *
 *         if (yaa_event_get_queue_size(queue) > 0)
 *         {
 *             yaa_err_t   err;
 *             yaa_event_t event;
 *
 *             err = yaa_event_get(queue, &event, YAA_TIMO_FOREVER);
 *             if (err == YAA_ERR_OK)
 *             {
 *                 yaa_event_dispatch(queue, &event);
 *             }
 *         }
 *     }
 * ~~~~~~~~
 * This pattern allows processing of both system and application events.  It
 * gives priority to system events because they are frequently the type of
 * events that must be processed quickly (e.g. Bluetooth protocol events).
 *
 * The above pattern also assumes that when both queues are empty, the event
 * that unblocks the application will arrive on the system event queue.  If
 * that is not necessarily the case, then other design patterns may be used.
 */

/* ============================================================================
 * Public Type Declarations
 * ==========================================================================*/

/**
 * @brief An event
 *
 * This structure holds all of the data that belongs to a single event
 *
 * Each event consists of a type and three arguments
 * * `event_type` is an integer, defined by the application.  It is used by the
 *   event queue library to locate and call registered handler functions.
 *
 *   **Important:** Applications **MUST NOT** use zero as an event type because
 *   it has a special meaning.  See #yaa_event_register_handler
 * * `iarg1` is the first 32-bit integer argument
 * * `iarg2` is the second 32-bit integer argument
 * * `parg` is an opaque pointer argument
 *
 * In most situations, it is possible to pack all necessary data directly into
 * the three arguments.  Where this is not practical, the event-generating code
 * may allocate memory, store the event data in that memory, and pass a pointer
 * to that memory via the `parg` argument.  The event handler can dereference
 * the pointer to access the data and then free the memory to prevent memory
 * leaks.
 */
typedef struct yaa_event_t
{
    uint32_t event_type; /**< @brief Application-defined event type */
    uint32_t iarg1;      /**< @brief First integer argument */
    uint32_t iarg2;      /**< @brief Second integer argument */
    void *parg;          /**< @brief Pointer argument */
} yaa_event_t;

/**
 * @brief A handle to an event queue
 *
 * An event queue handle is a pointer to opaque data that contains all of an
 * event queue's data.  All access is via yaa_event APIs
 */
typedef struct yaa_event_queue_t *yaa_event_queue_handle_t;

/**
 * @brief An event handler function prototype
 *
 * All event handler functions must be of this type
 *
 * @param event_type The type of the event being dispatched.  By passing this
 *                   to the handler function, it permits applications to
 *                   register a single function for multiple event types.
 * @param iarg1      The first integer argument
 * @param iarg2      The second integer argument
 * @param parg       The pointer argument
 * @param context    An opaque value provided by the application as a part of
 *                   registering the event handler
 */
typedef void (*yaa_event_handler_t)(uint32_t event_type, uint32_t iarg1, uint32_t iarg2, void *parg, void *context);

/* ============================================================================
 * Global Function Declarations (prototypes)
 * ==========================================================================*/

/**
 * @brief Create an event queue
 *
 * @param queue       On success, the handle pointed to will be populated with
 *                    a handle to the created event queue
 * @param max_events  The maximum number of events that may be queued at any
 *                    time.  The minimum value is 1.
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_event_queue_create(yaa_event_queue_handle_t *queue, uint32_t max_events);

/**
 * @brief Destroy an event queue
 *
 * Registrations and queued events will be destroyed.  No thread may be using
 * the queue at the time it is destroyed or undefined behavior (resulting from
 * accessing freed memory) will result.
 *
 * It is the responsibility of the application to free any
 * dynamically-allocated memory that is pointed to by an event registration
 * (via the context pointer) or an event's pointer argument.
 *
 * @param queue Handle to the event queue
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_event_queue_destroy(yaa_event_queue_handle_t queue);

/**
 * @brief Register a handler function for an event type on an event queue
 *
 * Each event type may be registered only once per queue.  If an application
 * tries to register two handlers for a particular event type on a single
 * queue, the second registration will overwrite the first.  An event type may
 * be un-registered by registering a NULL pointer as the handler function
 *
 * @param queue            Handle to the event queue
 * @param event_type       The type of event corresponding to this
 *                         registration.
 *                         <BR><BR>
 *                         Event type 0 is a "default" handler. A function
 *                         registered for event type 0 will be called for any
 *                         event type that does not have its own registered
 *                         handler function
 * @param handler_function The function that will be called when the
 *                         corresponding event is dispatched
 * @param context          An opaque value.  It is passed to the handler
 *                         function whenever the event is dispatched
 * @return                 #YAA_ERR_OK on success
 */
yaa_err_t yaa_event_register_handler(yaa_event_queue_handle_t queue, uint32_t event_type,
                                       yaa_event_handler_t handler_function, void *context);

/**
 * @brief Post an event to a queue
 *
 * Events are always posted to the end of the queue, ensuring that they are
 * always dispatched in the order they are posted.
 *
 * If a thread is blocked getting an event from the queue, it will be unblocked
 * after the event is posted.
 *
 * If the queue is full, this call will block until either space becomes
 * available or the amount of time specified by the `timeout` parameter has
 * elapsed.
 *
 * @param queue      Handle to the event queue
 * @param event_type The type of event to post
 * @param iarg1      The event's first integer argument
 * @param iarg2      The event's second integer argument
 * @param parg       The event's pointer argument
 * @param timeout    If the queue is full, the call will block for this much
 *                   time (in milliseconds) waiting for a space in the queue to
 *                   become available.  If the timeout interval elapses before
 *                   space becomes available, event posting will fail.
 *                   <BR /><BR />
 *                   The following special values may also be used:
 *                   * #YAA_TIMO_NOWAIT means the call will not block, but
 *                     will fail immediately if the queue is full
 *                   * #YAA_TIMO_FOREVER (not recommended) means the call will
 *                     block indefinitely, until the queue has space for the
 *                     event
 * @retval #YAA_ERR_OK      Success
 * @retval #YAA_ERR_TIMEOUT The queue is full and space did not become
 *                           available before the `timeout` interval elapsed
 */
yaa_err_t yaa_event_post(yaa_event_queue_handle_t queue, uint32_t event_type, uint32_t iarg1, uint32_t iarg2,
                           void *parg, uint32_t timeout);

/**
 * @brief Post an event to a queue from an interrupt handler
 *
 * Events are always posted to the end of the queue, ensuring that they are
 * always dispatched in the order they are posted.
 *
 * If a thread is blocked getting an event from the queue, it will be unblocked
 * after the event is posted.
 *
 * If the queue is full, this call will immediately return an error.
 *
 * @param queue      Handle to the event queue
 * @param event_type The type of event to post
 * @param iarg1      The event's first integer argument
 * @param iarg2      The event's second integer argument
 * @param parg       The event's pointer argument
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_event_post_from_isr(yaa_event_queue_handle_t queue, uint32_t event_type, uint32_t iarg1,
                                    uint32_t iarg2, void *parg);

/**
 * @brief Get an event from a queue.
 *
 * If the queue is empty, the call may either return #YAA_ERR_TIMEOUT or may
 * block, depending on the value of the `timeout` parameter.
 *
 * @param queue   Handle to the event queue
 * @param event   Pointer to an event object that, on success, will hold the
 *                event data removed from the queue
 * @param timeout If the queue is empty, the call will block for this much
 *                time (in milliseconds) waiting for an event to be posted.
 *                If the timeout interval elapses before an event is posted,
 *                #YAA_ERR_TIMEOUT will be returned.
 *                <BR /><BR />
 *                The following special values may also be used:
 *                * #YAA_TIMO_NOWAIT means the call will not block, but
 *                  will immediately return #YAA_ERR_TIMEOUT if the queue is
 *                  empty
 *                * #YAA_TIMO_FOREVER means the call will block
 *                  indefinitely, until an event has been posted
 * @retval #YAA_ERR_OK      Success.  `event` points to valid event data
 * @retval #YAA_ERR_TIMEOUT The queue is empty and an event was not posted
 *                           before the `timeout` interval elapsed
 */
yaa_err_t yaa_event_get(yaa_event_queue_handle_t queue, yaa_event_t *event, uint32_t timeout);

/**
 * @brief Get the total number of events in a queue
 *
 * @param queue Handle to the event queue
 * @return The number of events currently in the queue.  Zero is returned if
 *         `queue` is `NULL`.
 */
uint32_t yaa_event_get_queue_size(yaa_event_queue_handle_t queue);

/**
 * @brief Get the number of the number of events that can be posted before the
 * queue is full
 *
 * @param queue Handle to the event queue
 * @return The number of items up to the full queue.  Zero is returned if
 *         `queue` is `NULL`.
 */
uint32_t yaa_event_get_queue_free_space(yaa_event_queue_handle_t queue);

/**
 * @brief Dispatch an event to its registered handler
 *
 * This function will look up the registered handler function for the event on
 * its queue and then call it.
 *
 * Undefined behavior will result if the queue used to dispatch the event is
 * different from the queue the event was taken from.
 *
 * @param queue Handle to the event queue
 * @param event Pointer to the event to dispatch
 * @return #YAA_ERR_OK on success
 */
yaa_err_t yaa_event_dispatch(yaa_event_queue_handle_t queue, yaa_event_t *event);

#ifdef __cplusplus
}
#endif

#endif // YAA_EVENT_H
