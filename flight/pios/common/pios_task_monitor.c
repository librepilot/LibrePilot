/**
 ******************************************************************************
 * @addtogroup LibrePilotSystem LibrePilot System
 * @{
 * @addtogroup LibrePilotLibraries LibrePilot System Libraries
 * @{
 * @file       pios_task_monitor.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2015.
 *             The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010-2015.
 * @brief      Task monitoring functions
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

#ifdef PIOS_INCLUDE_TASK_MONITOR

// Private variables
static xSemaphoreHandle mLock;
static xTaskHandle *mTaskHandles;
static uint32_t mLastMonitorTime;
static uint32_t mLastIdleMonitorTime;
static uint16_t mMaxTasks;

volatile uint32_t idleCounter;
uint32_t zeroLoadIdleCounterPerSec = UINT32_MAX;
volatile bool idleCounterClear;
/**
 * Initialize the Task Monitor
 */
int32_t PIOS_TASK_MONITOR_Initialize(uint16_t max_tasks)
{
    mLock = xSemaphoreCreateRecursiveMutex();
    if (!mLock) {
        return -1;
    }

    mTaskHandles = (xTaskHandle *)pios_malloc(max_tasks * sizeof(xTaskHandle));
    if (!mTaskHandles) {
        return -1;
    }
    memset(mTaskHandles, 0, max_tasks * sizeof(xTaskHandle));

    mMaxTasks = max_tasks;
#if (configGENERATE_RUN_TIME_STATS == 1)
    mLastMonitorTime     = portGET_RUN_TIME_COUNTER_VALUE();
    mLastIdleMonitorTime = portGET_RUN_TIME_COUNTER_VALUE();
#else
    mLastMonitorTime     = 0;
    mLastIdleMonitorTime = 0;
#endif
    return 0;
}

/**
 * Register a task handle
 */
int32_t PIOS_TASK_MONITOR_RegisterTask(uint16_t task_id, xTaskHandle handle)
{
    if (mTaskHandles && task_id < mMaxTasks) {
        xSemaphoreTakeRecursive(mLock, portMAX_DELAY);
        mTaskHandles[task_id] = handle;
        xSemaphoreGiveRecursive(mLock);
        return 0;
    } else {
        return -1;
    }
}

/**
 * Unregister a task handle
 */
int32_t PIOS_TASK_MONITOR_UnregisterTask(uint16_t task_id)
{
    if (mTaskHandles && task_id < mMaxTasks) {
        xSemaphoreTakeRecursive(mLock, portMAX_DELAY);
        mTaskHandles[task_id] = 0;
        xSemaphoreGiveRecursive(mLock);
        return 0;
    } else {
        return -1;
    }
}

/**
 * Query if a task is running
 */
bool PIOS_TASK_MONITOR_IsRunning(uint16_t task_id)
{
    return mTaskHandles && task_id <= mMaxTasks && mTaskHandles[task_id];
}

/**
 * Tell the caller the status of all tasks via a task-by-task callback
 */
void PIOS_TASK_MONITOR_ForEachTask(TaskMonitorTaskInfoCallback callback, void *context)
{
    if (!mTaskHandles) {
        return;
    }

    xSemaphoreTakeRecursive(mLock, portMAX_DELAY);

#if (configGENERATE_RUN_TIME_STATS == 1)
    /* Calculate the amount of elapsed run time between the last time we
     * measured and now. Scale so that we can convert task run times
     * directly to percentages. */
    uint32_t currentTime = portGET_RUN_TIME_COUNTER_VALUE();
    /* avoid divide-by-zero if the interval is too small */
    uint32_t deltaTime   = ((currentTime - mLastMonitorTime) / 100) ? : 1;
    mLastMonitorTime = currentTime;
#endif
    /* Update all task information */
    for (uint16_t n = 0; n < mMaxTasks; ++n) {
        struct pios_task_info info;
        if (mTaskHandles[n]) {
            info.is_running = true;
#if defined(ARCH_POSIX) || defined(ARCH_WIN32)
            info.stack_remaining = 10000;
#else
            info.stack_remaining = uxTaskGetStackHighWaterMark(mTaskHandles[n]) * 4;
#endif
#if (configGENERATE_RUN_TIME_STATS == 1)
            /* Generate run time percentage stats */
            info.running_time_percentage = uxTaskGetRunTime(mTaskHandles[n]) / deltaTime;
#else
            info.running_time_percentage = 0;
#endif
        } else {
            info.is_running = false;
            info.stack_remaining = 0;
            info.running_time_percentage = 0;
        }
        /* Pass the information for this task back to the caller */
        callback(n, &info, context);
    }

    xSemaphoreGiveRecursive(mLock);
}


uint8_t PIOS_TASK_MONITOR_GetIdlePercentage()
{
    uint32_t ticks = PIOS_TASK_MONITOR_GetIdleTicksCount();

    return (uint8_t)((100 * ticks) / zeroLoadIdleCounterPerSec);
}

uint32_t PIOS_TASK_MONITOR_GetIdleTicksCount()
{
    static portTickType lastTickCount = 0;
    uint32_t ticks   = 0;
    portTickType now = xTaskGetTickCount();

    if (idleCounterClear) {
        idleCounter = 0;
    }

    if (now > lastTickCount) {
        uint32_t dT = (xTaskGetTickCount() - lastTickCount) * portTICK_RATE_MS; // in ms
        ticks = (1000 * idleCounter) / dT;
    } // else: TickCount has wrapped, do not calc now
    lastTickCount    = now;
    idleCounterClear = true;
    return ticks;
}

void PIOS_TASK_MONITOR_CalibrateIdleCounter()
{
    idleCounterClear = true;
    PIOS_TASK_MONITOR_GetIdleTicksCount();
    vTaskDelay(20);
    zeroLoadIdleCounterPerSec = PIOS_TASK_MONITOR_GetIdleTicksCount();
}

uint32_t PIOS_TASK_MONITOR_GetZeroLoadTicksCount()
{
    return zeroLoadIdleCounterPerSec;
}

/* 
 * print a human readable table of run time stats
 * information is generated from raw data provided by uxTaskGetSystemState().
 * The human readable table is written to pcWriteBuffer.  (see the vTaskList()
 * API function which actually does just this). 
 */
void PIOS_TASK_MONITOR_GetRunTimeStats(/*signed*/ char *pcWriteBuffer) {
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	unsigned long ulTotalRunTime, ulStatsAsPercentage;

   /* Make sure the write buffer does not contain a string. */
   *pcWriteBuffer = 0x00;

   /* Take a snapshot of the number of tasks in case it changes while this
   function is executing. */
   uxArraySize = uxTaskGetNumberOfTasks();

   /* Allocate a TaskStatus_t structure for each task.  An array could be
   allocated statically at compile time. */
   pxTaskStatusArray = pvPortMalloc( uxArraySize * sizeof( TaskStatus_t ) );

   if( pxTaskStatusArray != NULL ) {
      /* Generate raw status information about each task. */
      uxArraySize = uxTaskGetSystemState( pxTaskStatusArray,
                                 uxArraySize,
                                 &ulTotalRunTime );

      /* For percentage calculations. */
      ulTotalRunTime /= 100UL;

      /* Avoid divide by zero errors. */
      if( ulTotalRunTime > 0 ) {
         /* For each populated position in the pxTaskStatusArray array,
         format the raw data as human readable ASCII data. */
         for( x = 0; x < uxArraySize; x++ ) {
            /* What percentage of the total run time has the task used?
            This will always be rounded down to the nearest integer.
            ulTotalRunTimeDiv100 has already been divided by 100. */
            ulStatsAsPercentage =
                  pxTaskStatusArray[ x ].ulRunTimeCounter / ulTotalRunTime;

            if( ulStatsAsPercentage > 0UL ) {
               sprintf( pcWriteBuffer, "%s:%d,pct%d \n",
                                 pxTaskStatusArray[ x ].pcTaskName,
                                 pxTaskStatusArray[ x ].ulRunTimeCounter,
                                 ulStatsAsPercentage );
            }
            else {
               /* If the percentage is zero here then the task has
               consumed less than 1% of the total run time. */
               sprintf( pcWriteBuffer, "%s:%d,<1% \n",
                                 pxTaskStatusArray[ x ].pcTaskName,
                                 pxTaskStatusArray[ x ].ulRunTimeCounter );
            }

            pcWriteBuffer += strlen( ( char * ) pcWriteBuffer );
         }
      }

      /* The array is no longer needed, free the memory it consumes. */
      vPortFree( pxTaskStatusArray );
   }
}

#endif // PIOS_INCLUDE_TASK_MONITOR
