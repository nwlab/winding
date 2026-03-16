/**
 * @file yaa_sal.c
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
#include <yaa_macro.h>
#include <yaa_sal.h>
#include <yaa_types.h>

/* ============================================================================
 * Private Type Declarations
 * ==========================================================================*/

typedef struct yaa_mutex_struct
{
    SemaphoreHandle_t mutex;
#if (configUSE_RECURSIVE_MUTEXES == 1)
    bool recursive;
#endif
} yaa_mutex_struct_t;

typedef struct yaa_semaphore_struct
{
    SemaphoreHandle_t semaphore;
} yaa_semaphore_struct_t;

/* ============================================================================
 * Public Variable Declarations (externs)
 * ==========================================================================*/

/* ============================================================================
 * Global Function Definitions
 * ==========================================================================*/

void *yaa_alloc(size_t size)
{
    return pvPortMalloc(size);
}

void yaa_free(void *ptr)
{
    vPortFree(ptr);
}

uint8_t yaa_mdelay(uint32_t delay_ms)
{
    const TickType_t xDelay = delay_ms / portTICK_PERIOD_MS;
    vTaskDelay(xDelay);
    return YAA_ERR_OK;
}

/* Determine whether we are in thread mode or handler mode. */
YAA_STATIC_INLINE int in_handler_mode(void)
{
    return __get_IPSR() != 0;
}

uint8_t yaa_mutex_create(yaa_mutex_t *mutex_handle)
{
#if (configUSE_MUTEXES == 1)
    if (NULL == mutex_handle)
    {
        return YAA_ERR_BADARG;
    }

    yaa_mutex_struct_t *mtx = (yaa_mutex_struct_t *)yaa_alloc(sizeof(yaa_mutex_struct_t));
    if (NULL == mtx)
    {
        return YAA_ERR_NOMEM;
    }

    mtx->mutex = xSemaphoreCreateMutex();
#if (configUSE_RECURSIVE_MUTEXES == 1)
    mtx->recursive = false;
#endif

    if (NULL == mtx->mutex)
    {
        yaa_free(mtx);
        return YAA_ERR_NORESOURCE;
    }

    *mutex_handle = mtx;
    return YAA_ERR_OK;
#else
    return YAA_ERR_NORESOURCE;
#endif // configUSE_MUTEXES == 1
}

uint8_t yaa_mutex_destroy(yaa_mutex_t mutex_handle)
{
    if (NULL == mutex_handle)
    {
        return YAA_ERR_BADARG;
    }

    vSemaphoreDelete(mutex_handle->mutex);
    yaa_free(mutex_handle);
    return YAA_ERR_OK;
}

uint8_t yaa_mutex_lock(yaa_mutex_t mutex_handle, uint32_t wait_ms)
{
    portBASE_TYPE taskWoken = pdFALSE;

    if (NULL == mutex_handle)
    {
        return YAA_ERR_BADARG;
    }

    const TickType_t wait_ticks =
        (wait_ms == (uint32_t)YAA_TIMO_FOREVER) ? portMAX_DELAY : (wait_ms / portTICK_PERIOD_MS);

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
        return YAA_ERR_OK;
    }

    return YAA_ERR_TIMEOUT;
}
uint8_t yaa_mutex_unlock(yaa_mutex_t mutex_handle)
{
    portBASE_TYPE taskWoken = pdFALSE;

    if (NULL == mutex_handle)
    {
        return YAA_ERR_BADARG;
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
        return YAA_ERR_OK;
    }

    return YAA_ERR_FAIL;
}

uint8_t yaa_sem_create(yaa_sem_t *semaphore_handle, uint32_t initial_count, uint32_t max_count)
{
    if ((NULL == semaphore_handle) || (max_count == 0) || (initial_count > max_count))
    {
        return YAA_ERR_BADARG;
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
        return YAA_ERR_NORESOURCE;
    }

    *semaphore_handle = (yaa_sem_t)sem;
    return YAA_ERR_OK;
}

uint8_t yaa_sem_destroy(yaa_sem_t semaphore_handle)
{
    if (NULL == semaphore_handle)
    {
        return YAA_ERR_BADARG;
    }

    vSemaphoreDelete((SemaphoreHandle_t)semaphore_handle);
    return YAA_ERR_OK;
}

uint8_t yaa_sem_take(yaa_sem_t semaphore_handle, uint32_t wait_ms)
{
    if (NULL == semaphore_handle)
    {
        return YAA_ERR_BADARG;
    }

    const TickType_t wait_ticks =
        (wait_ms == (uint32_t)YAA_TIMO_FOREVER) ? portMAX_DELAY : (wait_ms / portTICK_PERIOD_MS);

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

            return YAA_ERR_OK;
        }
        return YAA_ERR_FAIL;
    }
    else
    {
        if (pdTRUE == xSemaphoreTake((SemaphoreHandle_t)semaphore_handle, wait_ticks))
        {
            return YAA_ERR_OK;
        }
    }

    return YAA_ERR_TIMEOUT;
}

uint8_t yaa_sem_give(yaa_sem_t semaphore_handle)
{
    if (NULL == semaphore_handle)
    {
        return YAA_ERR_BADARG;
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

            return YAA_ERR_OK;
        }

        return YAA_ERR_FAIL;
    }
    else
    {
        if (pdTRUE == xSemaphoreGive((SemaphoreHandle_t)semaphore_handle))
        {
            return YAA_ERR_OK;
        }
    }

    return YAA_ERR_FAIL;
}

uint8_t yaa_sem_getcount(yaa_sem_t semaphore_handle, uint32_t *count)
{
    if ((NULL == semaphore_handle) || (NULL == count))
    {
        return YAA_ERR_BADARG;
    }

    *count = uxSemaphoreGetCount((SemaphoreHandle_t)semaphore_handle);
    return YAA_ERR_OK;
}

uint8_t yaa_sem_reset(yaa_sem_t semaphore_handle)
{
    if (NULL == semaphore_handle)
    {
        return YAA_ERR_BADARG;
    }

    // FreeRTOS doesn't have a semaphore reset, so we'll get the count and
    // take it that many times to reset the count to zero.  Note that this
    // might take a while if the count is very high
    //
    // Loop on xSemaphoreTake() until semaphore is empty
    // Other thread's activity on this semaphore will either covered by this
    // loop or happen after call to yaa_ll_boards_sem_reset().

    while (xSemaphoreTake((SemaphoreHandle_t)semaphore_handle, 0) == pdTRUE)
    {
    };

    return YAA_ERR_OK;
}

/* ============================================================================
 * Wrappers to replace library memory allocation functions
 * ==========================================================================*/

void *__wrap__malloc_r(void *reent, size_t nbytes)
{
    YAA_UNUSED(reent);
    return yaa_alloc(nbytes);
}

void __wrap__free_r(void *reent, void *ptr)
{
    YAA_UNUSED(reent);
    if (ptr)
    {
        yaa_free(ptr);
    }
}

/* ============================================================================
 * Get memory helpers
 * ==========================================================================*/

size_t yaa_get_free_heap(void)
{
    return xPortGetFreeHeapSize();
}

size_t yaa_get_free_ram(void)
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

void yaa_cmd_ps(yaa_printf_t printf_func)
{
    volatile UBaseType_t uxArraySize;
    TaskStatus_t        *pxTaskStatusArray;
    uint32_t            ulTotalRunTime;

    // Take a snapshot of the number of tasks in case it changes while this
    // function is executing.
    uxArraySize = uxTaskGetNumberOfTasks();

    // Allocate a TaskStatus_t structure for each task. An array could be
    // allocated statically at compile time.
    pxTaskStatusArray = yaa_alloc(uxArraySize * sizeof(TaskStatus_t));
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
    yaa_free(pxTaskStatusArray);

    return;
}
