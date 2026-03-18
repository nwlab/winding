/**
 * @file rdnx_sal.c
 * @author Software development team
 * @brief Software application layer implementation for STM32 platform
 * @version 1.0
 * @date 2024-09-12
 */

/* ============================================================================
 * Include Files
 * ==========================================================================*/

/* Standard includes. */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* Library includes. */
#include <stm32f4xx_hal.h>

/* Scheduler includes. */
#include <FreeRTOS.h>
#include <queue.h>
#include <semphr.h>

/* Core includes. */
#include <rdnx_macro.h>
#include <rdnx_sal.h>
#include <rdnx_types.h>

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

typedef struct rdnx_mutex_struct
{
    SemaphoreHandle_t mutex;
#if (configUSE_RECURSIVE_MUTEXES == 1)
    bool recursive;
#endif
} rdnx_mutex_struct_t;

typedef struct rdnx_semaphore_struct
{
    SemaphoreHandle_t semaphore;
} rdnx_semaphore_struct_t;

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

void *rdnx_alloc(size_t size)
{
    return pvPortMalloc(size);
}

void rdnx_free(void *ptr)
{
    vPortFree(ptr);
}

uint8_t rdnx_mdelay(uint32_t delay_ms)
{
    const TickType_t xDelay = delay_ms / portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
    return RDNX_ERR_OK;
}

/* Determine whether we are in thread mode or handler mode. */
RDNX_STATIC_INLINE int in_handler_mode(void)
{
    return __get_IPSR() != 0;
}

uint8_t rdnx_mutex_create(rdnx_mutex_t *mutex_handle)
{
#if (configUSE_MUTEXES == 1)
    if (NULL == mutex_handle)
    {
        return RDNX_ERR_BADARG;
    }

    rdnx_mutex_struct_t *mtx = (rdnx_mutex_struct_t *)rdnx_alloc(sizeof(rdnx_mutex_struct_t));
    if (NULL == mtx)
    {
        return RDNX_ERR_NOMEM;
    }

    mtx->mutex = xSemaphoreCreateMutex();
#if (configUSE_RECURSIVE_MUTEXES == 1)
    mtx->recursive = false;
#endif

    if (NULL == mtx->mutex)
    {
        rdnx_free(mtx);
        return RDNX_ERR_NORESOURCE;
    }

    *mutex_handle = mtx;
    return RDNX_ERR_OK;
#else
    return RDNX_ERR_NORESOURCE;
#endif // configUSE_MUTEXES == 1
}

uint8_t rdnx_mutex_destroy(rdnx_mutex_t mutex_handle)
{
    if (NULL == mutex_handle)
    {
        return RDNX_ERR_BADARG;
    }

    vSemaphoreDelete(mutex_handle->mutex);
    rdnx_free(mutex_handle);
    return RDNX_ERR_OK;
}

uint8_t rdnx_mutex_lock(rdnx_mutex_t mutex_handle, uint32_t wait_ms)
{
    portBASE_TYPE taskWoken = pdFALSE;

    if (NULL == mutex_handle)
    {
        return RDNX_ERR_BADARG;
    }

    const TickType_t wait_ticks =
        (wait_ms == (uint32_t)RDNX_TIMO_FOREVER) ? portMAX_DELAY : (wait_ms / portTICK_PERIOD_MS);

    BaseType_t ret;

#if (configUSE_RECURSIVE_MUTEXES == 1)
    if (mutex_handle->recursive)
    {
        ret = xSemaphoreTakeRecursive(mutex_handle->mutex, wait_ticks);
    }
    else
#endif
    {
        if (in_handler_mode())
        {
            ret = xSemaphoreTakeFromISR(mutex_handle->mutex, &taskWoken);
            portEND_SWITCHING_ISR(taskWoken);
        }
        else
        {
            ret = xSemaphoreTake(mutex_handle->mutex, wait_ticks);
        }
    }

    if (pdTRUE == ret)
    {
        return RDNX_ERR_OK;
    }

    return RDNX_ERR_TIMEOUT;
}
uint8_t rdnx_mutex_unlock(rdnx_mutex_t mutex_handle)
{
    portBASE_TYPE taskWoken = pdFALSE;

    if (NULL == mutex_handle)
    {
        return RDNX_ERR_BADARG;
    }

    BaseType_t ret;

#if (configUSE_RECURSIVE_MUTEXES == 1)
    if (mutex_handle->recursive)
    {
        ret = xSemaphoreGiveRecursive(mutex_handle->mutex);
    }
    else
#endif
    {
        if (in_handler_mode())
        {
            ret = xSemaphoreGiveFromISR(mutex_handle->mutex, &taskWoken);
            portEND_SWITCHING_ISR(taskWoken);
        }
        else
        {
            ret = xSemaphoreGive(mutex_handle->mutex);
        }
    }

    if (pdTRUE == ret)
    {
        return RDNX_ERR_OK;
    }

    return RDNX_ERR_FAIL;
}

uint8_t rdnx_sem_create(rdnx_sem_t *semaphore_handle, uint32_t initial_count, uint32_t max_count)
{
    if ((NULL == semaphore_handle) || (max_count == 0) || (initial_count > max_count))
    {
        return RDNX_ERR_BADARG;
    }

    SemaphoreHandle_t sem;

    /*
     * If the user requests a max count of 1, then create a binary semaphore,
     * which should be better optimized for that situation
     */
    if (max_count == 1)
    {
        sem = xSemaphoreCreateBinary();
        if ((sem != NULL) && (initial_count > 0))
        {
            xSemaphoreGive(sem);
        }
    }
    else
    {
        sem = xSemaphoreCreateCounting(max_count, initial_count);
    }

    if (NULL == sem)
    {
        return RDNX_ERR_NORESOURCE;
    }

    *semaphore_handle = (rdnx_sem_t)sem;
    return RDNX_ERR_OK;
}

uint8_t rdnx_sem_destroy(rdnx_sem_t semaphore_handle)
{
    if (NULL == semaphore_handle)
    {
        return RDNX_ERR_BADARG;
    }

    vSemaphoreDelete((SemaphoreHandle_t)semaphore_handle);
    return RDNX_ERR_OK;
}

uint8_t rdnx_sem_take(rdnx_sem_t semaphore_handle, uint32_t wait_ms)
{
    if (NULL == semaphore_handle)
    {
        return RDNX_ERR_BADARG;
    }

    const TickType_t wait_ticks =
        (wait_ms == (uint32_t)RDNX_TIMO_FOREVER) ? portMAX_DELAY : (wait_ms / portTICK_PERIOD_MS);

    if (in_handler_mode())
    {
        BaseType_t higher_priority_task_woken = pdFALSE;

        if (pdTRUE == xSemaphoreTakeFromISR((SemaphoreHandle_t)semaphore_handle, &higher_priority_task_woken))

        {
            /*
             * higher_priority_task_woken is set to pdTRUE if giving the
             * semaphore woke up a blocked task with higher priority than the
             * one that was running when the interrupt occurred.
             *
             * portYIELD_FROM_ISR will check this flag and tell the scheduler
             * to yield to that task when the interrupt returns.  This improves
             * performance when cooperative schedulers are used.  It should
             * have no effect on preemptive schedulers.
             */
            portYIELD_FROM_ISR(higher_priority_task_woken);

            return RDNX_ERR_OK;
        }
        return RDNX_ERR_FAIL;
    }
    else
    {
        if (pdTRUE == xSemaphoreTake((SemaphoreHandle_t)semaphore_handle, wait_ticks))
        {
            return RDNX_ERR_OK;
        }
    }

    return RDNX_ERR_TIMEOUT;
}

uint8_t rdnx_sem_give(rdnx_sem_t semaphore_handle)
{
    if (NULL == semaphore_handle)
    {
        return RDNX_ERR_BADARG;
    }

    if (in_handler_mode())
    {
        BaseType_t higher_priority_task_woken = pdFALSE;

        if (pdTRUE == xSemaphoreGiveFromISR((SemaphoreHandle_t)semaphore_handle, &higher_priority_task_woken))
        {
            /*
             * higher_priority_task_woken is set to pdTRUE if giving the
             * semaphore woke up a blocked task with higher priority than the
             * one that was running when the interrupt occurred.
             *
             * portYIELD_FROM_ISR will check this flag and tell the scheduler
             * to yield to that task when the interrupt returns.  This improves
             * performance when cooperative schedulers are used.  It should
             * have no effect on preemptive schedulers.
             */
            portYIELD_FROM_ISR(higher_priority_task_woken);

            return RDNX_ERR_OK;
        }

        return RDNX_ERR_FAIL;
    }
    else
    {
        if (pdTRUE == xSemaphoreGive((SemaphoreHandle_t)semaphore_handle))
        {
            return RDNX_ERR_OK;
        }
    }

    return RDNX_ERR_FAIL;
}

uint8_t rdnx_sem_getcount(rdnx_sem_t semaphore_handle, uint32_t *count)
{
    if ((NULL == semaphore_handle) || (NULL == count))
    {
        return RDNX_ERR_BADARG;
    }

    *count = uxSemaphoreGetCount((SemaphoreHandle_t)semaphore_handle);
    return RDNX_ERR_OK;
}

uint8_t rdnx_sem_reset(rdnx_sem_t semaphore_handle)
{
    if (NULL == semaphore_handle)
    {
        return RDNX_ERR_BADARG;
    }

    // FreeRTOS doesn't have a semaphore reset, so we'll get the count and
    // take it that many times to reset the count to zero.  Note that this
    // might take a while if the count is very high
    //
    // Loop on xSemaphoreTake() until semaphore is empty
    // Other thread's activity on this semaphore will either covered by this
    // loop or happen after call to rdnx_ll_boards_sem_reset().

    while (xSemaphoreTake((SemaphoreHandle_t)semaphore_handle, 0) == pdTRUE)
    {
    };

    return RDNX_ERR_OK;
}

/* ============================================================================
 * Get memory helpers
 * ==========================================================================*/

size_t rdnx_get_free_heap(void)
{
    return xPortGetFreeHeapSize();
}

size_t rdnx_get_free_ram(void)
{
    TaskHandle_t xTask = xTaskGetCurrentTaskHandle();
    UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark(xTask);
    return (size_t)uxHighWaterMark;
}

static const char *get_task_state_name(eTaskState state)
{
    switch(state)
    {
        case eRunning:
            return "Run";
        case eReady:
            return "Ready";
        case eBlocked:
            return "Block";
        case eSuspended:
            return "Susp";
        case eDeleted:
            return "Del";
        default:
            return "???";
    }
}

static int cli_cmd_ps_cmp(const void *a, const void *b)
{
    const TaskStatus_t *ta = (const TaskStatus_t *)a;
    const TaskStatus_t *tb = (const TaskStatus_t *)b;

    // sort first by current priority
    if (ta->uxCurrentPriority > tb->uxCurrentPriority)
    {
        // higher priority comes first
        return -1;
    }
    else if (ta->uxCurrentPriority < tb->uxCurrentPriority)
    {
        // lower priority comes last
        return 1;
    }

    // sort second by task status (running, ready, blocked, suspended, deleted)
    if (ta->eCurrentState < tb->eCurrentState)
    {
        // lower numeric status comes first
        return -1;
    }
    else if (ta->eCurrentState > tb->eCurrentState)
    {
        // higher numeric status comes last
        return 1;
    }

    // sort last by task name
    return strcmp(ta->pcTaskName, tb->pcTaskName);
}

void rdnx_cmd_ps(rdnx_printf_t printf_func)
{
    volatile UBaseType_t uxArraySize;
    TaskStatus_t        *pxTaskStatusArray;
    uint32_t            ulTotalRunTime;

    // Take a snapshot of the number of tasks in case it changes while this
    // function is executing.
    uxArraySize = uxTaskGetNumberOfTasks();

    // Allocate a TaskStatus_t structure for each task. An array could be
    // allocated statically at compile time.
    pxTaskStatusArray = rdnx_alloc(uxArraySize * sizeof(TaskStatus_t));
    if (pxTaskStatusArray == NULL)
    {
        return;
    }

    // Generate raw status information about each task.
    uxArraySize = uxTaskGetSystemState(
        pxTaskStatusArray,
        uxArraySize,
        &ulTotalRunTime
    );
    // Sort task info list.
    qsort(pxTaskStatusArray, uxArraySize, sizeof(TaskStatus_t), cli_cmd_ps_cmp); //NOSONAR
    // Scale total run time for percentage calculations.
    ulTotalRunTime /= 100UL;

    // Print header for FreeRTOS task list.
    printf_func("%-*.*s   STATE   PRIORITY   STACK",
            configMAX_TASK_NAME_LEN,
            configMAX_TASK_NAME_LEN,
            "NAME"
    );
    if (ulTotalRunTime > 0)
    {
        printf_func("   RUN (ticks & %%)");
    }
    printf_func("\r\n");

    // For each populated position in the pxTaskStatusArray array, format the
    // raw data as human readable ASCII data.
    for (UBaseType_t i = 0; i < uxArraySize; i++)
    {
        printf_func(
            "%-*.*s   %-5s   %2lu %2s %2lu   %5u",
            configMAX_TASK_NAME_LEN,
            configMAX_TASK_NAME_LEN,
            pxTaskStatusArray[i].pcTaskName,
            get_task_state_name(pxTaskStatusArray[i].eCurrentState),
            pxTaskStatusArray[i].uxCurrentPriority,
            (pxTaskStatusArray[i].uxCurrentPriority == pxTaskStatusArray[i].uxBasePriority) ? "==" : "<-",
            pxTaskStatusArray[i].uxBasePriority,
            pxTaskStatusArray[i].usStackHighWaterMark * 4
        );
        if (ulTotalRunTime > 0)
        {
            // What percentage of the total run time has the task used?
            // This will always be rounded down to the nearest integer.
            // ulTotalRunTimeDiv100 has already been divided by 100.
            unsigned long ulStatsAsPercentage =
                pxTaskStatusArray[i].ulRunTimeCounter / ulTotalRunTime;
            printf_func("   %10lu%3lu%%",
                   pxTaskStatusArray[i].ulRunTimeCounter,
                   ulStatsAsPercentage
            );
        }
        printf_func("\r\n");
    }

    // Print footer explaining some columns.
    printf_func("* STACK is min free bytes; PRIORITY is current & base");
    if (ulTotalRunTime > 0)
    {
        printf_func("; RUN tick rate is 32768 Hz");
    }
    printf_func("\r\n");

    // The array is no longer needed, free the memory it consumes.
    rdnx_free(pxTaskStatusArray);

    return;
}
