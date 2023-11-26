/**
 ******************************************************************************
 *
 * @file       pios_callbackscheduler.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2013.
 * @brief      Scheduler to run callback functions from a shared context with given priorities.
 *
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <pios.h>
#ifdef PIOS_INCLUDE_CALLBACKSCHEDULER

#include <utlist.h>
#include <uavobjectmanager.h>
#include <taskinfo.h>

// Private constants
#define STACK_SAFETYCOUNT 16
#define STACK_SIZE        (300 + STACK_SAFETYSIZE)
#define STACK_SAFETYSIZE  8
#define MAX_SLEEP         1000

// Private types
/**
 * task information
 */
struct DelayedCallbackTaskStruct {
    DelayedCallbackInfo *callbackQueue[CALLBACK_PRIORITY_LOW + 1];
    DelayedCallbackInfo *queueCursor[CALLBACK_PRIORITY_LOW + 1];
    xTaskHandle callbackSchedulerTaskHandle;
    char name[3];
    uint32_t    stackSize;
    DelayedCallbackPriorityTask priorityTask;
    xSemaphoreHandle signal;
    struct DelayedCallbackTaskStruct *next;
};

/**
 * callback information
 */
struct DelayedCallbackInfoStruct {
    DelayedCallback   cb;
    int16_t callbackID;
    bool volatile     waiting;
    uint32_t volatile scheduletime;
    uint32_t stackSize;
    int32_t  stackFree;
    int32_t  stackNotFree;
    uint16_t stackSafetyCount;
    uint16_t currentSafetyCount;
    uint32_t runCount;
    struct DelayedCallbackTaskStruct *task;
    struct DelayedCallbackInfoStruct *next;
};


// Private variables
static struct DelayedCallbackTaskStruct *schedulerTasks;
static xSemaphoreHandle mutex;
static bool schedulerStarted;

// Private functions
static void CallbackSchedulerTask(void *task);
static int32_t runNextCallback(struct DelayedCallbackTaskStruct *task, DelayedCallbackPriority priority);

/**
 * Initialize the scheduler
 * must be called before any other functions are called
 * \return Success (0), failure (-1)
 */
int32_t PIOS_CALLBACKSCHEDULER_Initialize()
{
    // Initialize variables
    schedulerTasks   = NULL;
    schedulerStarted = false;

    // Create mutex
    mutex = xSemaphoreCreateRecursiveMutex();
    if (mutex == NULL) {
        return -1;
    }

    // Done
    return 0;
}

/**
 * Start all scheduler tasks
 * Will instantiate all scheduler tasks registered so far.  Although new
 * callbacks CAN be registered beyond that point, any further scheduling tasks
 * will be started the moment of instantiation.  It is not possible to increase
 * the STACK requirements of a scheduler task after this function has been
 * run.  No callbacks will be run before this function is called, although
 * they can be marked for later execution by executing the dispatch function.
 * \return Success (0), failure (-1)
 */
int32_t PIOS_CALLBACKSCHEDULER_Start()
{
    xSemaphoreTakeRecursive(mutex, portMAX_DELAY);

    // only call once
    PIOS_Assert(schedulerStarted == false);

    // start tasks
    struct DelayedCallbackTaskStruct *cursor = NULL;
    int t = 0;
    LL_FOREACH(schedulerTasks, cursor) {
        xTaskCreate(
            CallbackSchedulerTask,
            cursor->name,
            1 + (cursor->stackSize / 4),
            cursor,
            cursor->priorityTask,
            &cursor->callbackSchedulerTaskHandle
            );
        if (TASKINFO_RUNNING_CALLBACKSCHEDULER0 + t <= TASKINFO_RUNNING_CALLBACKSCHEDULER3) {
            PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_CALLBACKSCHEDULER0 + t, cursor->callbackSchedulerTaskHandle);
        }
        t++;
    }

    schedulerStarted = true;

    xSemaphoreGiveRecursive(mutex);

    return 0;
}

/**
 * Schedule dispatching a callback at some point in the future. The function returns immediately.
 * \param[in] *cbinfo the callback handle
 * \param[in] milliseconds How far in the future to dispatch the callback
 * \param[in] updatemode What to do if the callback is already scheduled but not dispatched yet.
 * The options are:
 * UPDATEMODE_NONE: An existing schedule will not be touched, the call will have no effect at all if there's an existing schedule.
 * UPDATEMODE_SOONER: The callback will be rescheduled only if the new schedule triggers before the original one would have triggered.
 * UPDATEMODE_LATER: The callback will be rescheduled only if the new schedule triggers after the original one would have triggered.
 * UPDATEMODE_OVERRIDE: The callback will be rescheduled in any case, effectively overriding any previous schedule. (sooner+later=override)
 * \return 0: not scheduled, previous schedule takes precedence, 1: new schedule, 2: previous schedule overridden
 */
int32_t PIOS_CALLBACKSCHEDULER_Schedule(
    DelayedCallbackInfo *cbinfo,
    int32_t milliseconds,
    DelayedCallbackUpdateMode updatemode)
{
    int32_t result = 0;

    PIOS_Assert(cbinfo);

    if (milliseconds <= 0) {
        milliseconds = 0; // we can and will not schedule in the past since that ruins the wraparound of uint32_t
    }

    xSemaphoreTakeRecursive(mutex, portMAX_DELAY);

    uint32_t new = xTaskGetTickCount() + (milliseconds / portTICK_RATE_MS);
    if (!new) {
        new = 1; // zero has a special meaning, schedule at time 1 instead
    }

    int32_t diff = new - cbinfo->scheduletime;
    if ((!cbinfo->scheduletime)
        || ((updatemode & CALLBACK_UPDATEMODE_SOONER) && diff < 0)
        || ((updatemode & CALLBACK_UPDATEMODE_LATER) && diff > 0)
        ) {
        // the scheduletime may be updated
        if (!cbinfo->scheduletime) {
            result = 1;
        } else {
            result = 2;
        }
        cbinfo->scheduletime = new;

        // scheduler needs to be notified to adapt sleep times
        xSemaphoreGive(cbinfo->task->signal);
    }

    xSemaphoreGiveRecursive(mutex);

    return result;
}

/**
 * Dispatch an event by invoking the supplied callback. The function
 * returns immediately, the callback is invoked from the event task.
 * \param[in] cbinfo the callback handle
 * \return Success (-1), failure (0)
 */
int32_t PIOS_CALLBACKSCHEDULER_Dispatch(DelayedCallbackInfo *cbinfo)
{
    PIOS_Assert(cbinfo);

    // no semaphore needed for the callback
    cbinfo->waiting = true;
    // but the scheduler as a whole needs to be notified
    return xSemaphoreGive(cbinfo->task->signal);
}

/**
 * Dispatch an event by invoking the supplied callback. The function
 * returns immediately, the callback is invoked from the event task.
 * \param[in] cbinfo the callback handle
 * \param[in] pxHigherPriorityTaskWoken
 * xSemaphoreGiveFromISR() will set *pxHigherPriorityTaskWoken to pdTRUE if
 * giving the semaphore caused a task to unblock, and the unblocked task has a
 * priority higher than the currently running task.  If xSemaphoreGiveFromISR()
 * sets this value to pdTRUE then a context switch should be requested before
 * the interrupt is exited.
 * From FreeRTOS Docu: Context switching from an ISR uses port specific syntax.
 * Check the demo task for your port to find the syntax required.
 * \return Success (-1), failure (0)
 */
int32_t PIOS_CALLBACKSCHEDULER_DispatchFromISR(DelayedCallbackInfo *cbinfo, long *pxHigherPriorityTaskWoken)
{
    PIOS_Assert(cbinfo);

    // no semaphore needed for the callback
    cbinfo->waiting = true;
    // but the scheduler as a whole needs to be notified
    return xSemaphoreGiveFromISR(cbinfo->task->signal, pxHigherPriorityTaskWoken);
}

/**
 * Register a new callback to be called by a delayed callback scheduler task.
 * If a scheduler task with the specified task priority does not exist yet, it
 * will be created.
 * \param[in] cb The callback to be invoked
 * \param[in] priority Priority of the callback compared to other callbacks scheduled by the same delayed callback scheduler task.
 * \param[in] priorityTask Task priority of the scheduler task. One scheduler task will be spawned for each distinct value specified,
 *            further callbacks created  with the same priorityTask will all be handled by the same delayed callback scheduler task
 *            and scheduled according to their individual callback priorities
 * \param[in] stacksize The stack requirements of the callback when called by the scheduler.
 * \return CallbackInfo Pointer on success, NULL if failed.
 */
DelayedCallbackInfo *PIOS_CALLBACKSCHEDULER_Create(
    DelayedCallback cb,
    DelayedCallbackPriority priority,
    DelayedCallbackPriorityTask priorityTask,
    int16_t callbackID,
    uint32_t stacksize)
{
    xSemaphoreTakeRecursive(mutex, portMAX_DELAY);

    // add callback schedulers own stack requirements
    stacksize += STACK_SIZE;

    // find appropriate scheduler task matching priorityTask
    struct DelayedCallbackTaskStruct *task = NULL;
    int t = 0;
    LL_FOREACH(schedulerTasks, task) {
        if (task->priorityTask == priorityTask) {
            break; // found
        }
        t++;
    }
    // if given priorityTask does not exist, create it
    if (!task) {
        // allocate memory if possible
        task = (struct DelayedCallbackTaskStruct *)pios_malloc(sizeof(struct DelayedCallbackTaskStruct));
        if (!task) {
            xSemaphoreGiveRecursive(mutex);
            return NULL;
        }

        // initialize structure
        for (DelayedCallbackPriority p = 0; p <= CALLBACK_PRIORITY_LOW; p++) {
            task->callbackQueue[p] = NULL;
            task->queueCursor[p]   = NULL;
        }
        task->name[0]      = 'C';
        task->name[1]      = 'a' + t;
        task->name[2]      = 0;
        task->stackSize    = stacksize;
        task->priorityTask = priorityTask;
        task->next = NULL;

        // create the signaling semaphore
        vSemaphoreCreateBinary(task->signal);
        if (!task->signal) {
            xSemaphoreGiveRecursive(mutex);
            return NULL;
        }

        // add to list of scheduler tasks
        LL_APPEND(schedulerTasks, task);

        // Previously registered tasks are spawned when PIOS_CALLBACKSCHEDULER_Start() is called.
        // Tasks registered afterwards need to spawn upon creation.
        if (schedulerStarted) {
            xTaskCreate(
                CallbackSchedulerTask,
                task->name,
                1 + (task->stackSize / 4),
                task,
                task->priorityTask,
                &task->callbackSchedulerTaskHandle
                );
            if (TASKINFO_RUNNING_CALLBACKSCHEDULER0 + t <= TASKINFO_RUNNING_CALLBACKSCHEDULER3) {
                PIOS_TASK_MONITOR_RegisterTask(TASKINFO_RUNNING_CALLBACKSCHEDULER0 + t, task->callbackSchedulerTaskHandle);
            }
        }
    }

    if (!schedulerStarted && stacksize > task->stackSize) {
        task->stackSize = stacksize; // previous to task initialisation we can still adapt to the maximum needed stack
    }


    if (stacksize > task->stackSize) {
        xSemaphoreGiveRecursive(mutex);
        return NULL; // error - not enough memory
    }

    // initialize callback scheduling info
    DelayedCallbackInfo *info = (DelayedCallbackInfo *)pios_malloc(sizeof(DelayedCallbackInfo));
    if (!info) {
        xSemaphoreGiveRecursive(mutex);
        return NULL; // error - not enough memory
    }
    info->next               = NULL;
    info->waiting            = false;
    info->scheduletime       = 0;
    info->task               = task;
    info->cb = cb;
    info->callbackID         = callbackID;
    info->runCount           = 0;
    info->stackSize          = stacksize - STACK_SIZE;
    info->stackNotFree       = info->stackSize;
    info->stackFree          = 0;
    info->stackSafetyCount   = STACK_SAFETYCOUNT;
    info->currentSafetyCount = 0;

    // add to scheduling queue
    LL_APPEND(task->callbackQueue[priority], info);

    xSemaphoreGiveRecursive(mutex);

    return info;
}

/**
 * Iterator. Iterates over all callbacks and all scheduler tasks and retrieves information
 *
 * @param[in] callback  Callback function to receive the data - will be called in same task context as the callerThe id of the task the task_info refers to.
 * @param     context   Context information optionally provided to the callback.
 */
void PIOS_CALLBACKSCHEDULER_ForEachCallback(CallbackSchedulerCallbackInfoCallback callback, void *context)
{
    if (!callback) {
        return;
    }

    struct pios_callback_info info;

    struct DelayedCallbackTaskStruct *task = NULL;
    LL_FOREACH(schedulerTasks, task) {
        int prio;

        for (prio = 0; prio < (CALLBACK_PRIORITY_LOW + 1); prio++) {
            struct DelayedCallbackInfoStruct *cbinfo;
            /* use volatile to avoid opt*/
            LL_FOREACH(task->callbackQueue[prio], cbinfo) {
                xSemaphoreTakeRecursive(mutex, portMAX_DELAY);
                info.is_running = true;
                info.stack_remaining    = cbinfo->stackNotFree;
                info.running_time_count = cbinfo->runCount;
                xSemaphoreGiveRecursive(mutex);
                callback(cbinfo->callbackID, &info, context);
            }
        }
    }
}

/**
 * Stack magic, find how much stack is being used without affecting performance
 */
static void markStack(DelayedCallbackInfo *current)
{
    register int8_t t;
    register int32_t halfWayMark;
    volatile unsigned char *marker;

    if (current->stackNotFree < 0) {
        return;
    }

    // end of stack watermark
    marker = (unsigned char *)(((size_t)&marker) - (size_t)current->stackSize);
    for (t = -STACK_SAFETYSIZE; t < 0; t++) {
        *(marker + t) = '#';
    }
    // shifted watermarks
    halfWayMark = current->stackFree + (current->stackNotFree - current->stackFree) / 2;
    marker = (unsigned char *)((size_t)marker + halfWayMark);
    for (t = -STACK_SAFETYSIZE; t < 0; t++) {
        *(marker + t) = '#';
    }
}
static void checkStack(DelayedCallbackInfo *current)
{
    register int8_t t;
    register int32_t halfWayMark;
    volatile unsigned char *marker;

    if (current->stackNotFree < 0) {
        return;
    }

    // end of stack watermark
    marker = (unsigned char *)(((size_t)&marker) - (size_t)current->stackSize);
    for (t = -STACK_SAFETYSIZE; t < 0; t++) {
        if (*(marker + t) != '#') {
            current->stackNotFree = -1; // stack overflow, disable all further checks
            return;
        }
    }
    // shifted watermarks
    halfWayMark = current->stackFree + (current->stackNotFree - current->stackFree) / 2;
    marker = (unsigned char *)((size_t)marker + halfWayMark);
    for (t = -STACK_SAFETYSIZE; t < 0; t++) {
        if (*(marker + t) != '#') {
            current->stackNotFree = halfWayMark; // tainted mark, this place is definitely used stack
            current->currentSafetyCount = 0;
            if (current->stackNotFree <= current->stackFree) {
                current->stackFree = 0; // if it was supposed to be free, restart search between here and bottom of stack
            }
            return;
        }
    }

    if (current->currentSafetyCount < 0xffff) {
        current->currentSafetyCount++; // mark has not been tainted, increase safety counter
    }
    if (current->currentSafetyCount >= current->stackSafetyCount) { // if the safety counter is above the limit, then
        if (halfWayMark == current->stackFree) { // check if search already converged, if so increase the limit to find very rare stack usage incidents
            current->stackSafetyCount = current->currentSafetyCount;
        } else {
            current->stackFree = halfWayMark; // otherwise just mark this position as free stack to narrow search
            current->currentSafetyCount = 0;
        }
    }
}

/**
 * Scheduler subtask
 * \param[in] task The scheduler task in question
 * \param[in] priority The scheduling priority of the callback to search for
 * \return wait time until next scheduled callback is due - 0 if a callback has just been executed
 */
static int32_t runNextCallback(struct DelayedCallbackTaskStruct *task, DelayedCallbackPriority priority)
{
    int32_t result = MAX_SLEEP;
    int32_t diff   = 0;

    // no such queue
    if (priority > CALLBACK_PRIORITY_LOW) {
        return result;
    }

    // queue is empty, search a lower priority queue
    if (task->callbackQueue[priority] == NULL) {
        return runNextCallback(task, priority + 1);
    }

    DelayedCallbackInfo *current = task->queueCursor[priority];
    DelayedCallbackInfo *next;
    do {
        if (current == NULL) {
            next = task->callbackQueue[priority]; // loop around the end of the list
            // also attempt to run a callback that has lower priority
            // every time the queue is completely traversed
            diff = runNextCallback(task, priority + 1);
            if (!diff) {
                task->queueCursor[priority] = next; // the recursive call has executed a callback
                return 0;
            }
            if (diff < result) {
                result = diff; // adjust sleep time
            }
        } else {
            next = current->next;
            xSemaphoreTakeRecursive(mutex, portMAX_DELAY); // access to scheduletime should be mutex protected
            if (current->scheduletime) {
                diff = current->scheduletime - xTaskGetTickCount();
                if (diff <= 0) {
                    current->waiting = true;
                } else if (diff < result) {
                    result = diff; // adjust sleep time
                }
            }
            if (current->waiting) {
                task->queueCursor[priority] = next;
                current->scheduletime = 0; // any schedules are reset
                current->waiting = false; // the flag is reset just before execution.
                xSemaphoreGiveRecursive(mutex);

                /* callback gets invoked here - check stack sizes */
                markStack(current);

                current->cb(); // call the callback

                checkStack(current);

                current->runCount++;

                return 0;
            }
            xSemaphoreGiveRecursive(mutex);
        }
        current = next;
    } while (current != task->queueCursor[priority]);
    // once the list has been traversed entirely without finding any to be executed task, abort (nothing to do)
    return result;
}

/**
 * Scheduler task, responsible of invoking callbacks.
 * \param[in] task The scheduling task being run
 */
static void CallbackSchedulerTask(void *task)
{
    uint32_t delay = 0;

    while (1) {
        delay = runNextCallback((struct DelayedCallbackTaskStruct *)task, CALLBACK_PRIORITY_CRITICAL);
        if (delay) {
            // nothing to do but sleep
            xSemaphoreTake(((struct DelayedCallbackTaskStruct *)task)->signal, delay);
        }
    }
}

#endif // ifdef PIOS_INCLUDE_CALLBACKSCHEDULER
