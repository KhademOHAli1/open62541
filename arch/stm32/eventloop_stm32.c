/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) STM32 Port - based on LWIP EventLoop
 */

#include "eventloop_stm32.h"

#if defined(UA_ARCHITECTURE_STM32)

#include <open62541/plugin/log_stdout.h>

/* Global pointer to the event loop for task communication */
static UA_EventLoopSTM32 *g_eventLoop = NULL;

/*********************/
/* Utility Functions */
/*********************/

static UA_StatusCode
setNonBlocking(UA_FD sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if(flags == -1)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    flags |= O_NONBLOCK;
    if(fcntl(sockfd, F_SETFL, flags) == -1)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    return UA_STATUSCODE_GOOD;
}

static UA_StatusCode
setNoSigPipe(UA_FD sockfd) {
    /* Not applicable for lwIP */
    return UA_STATUSCODE_GOOD;
}

static void
setFDSetsFromFDTree(UA_EventLoopSTM32 *el, fd_set *readset, fd_set *writeset,
                   UA_FD *maxfd) {
    FD_ZERO(readset);
    FD_ZERO(writeset);
    *maxfd = UA_INVALID_FD;

    UA_RegisteredFD *rfd;
    ZIP_ITER(UA_FDTree, &el->el.fds, rfd) {
        UA_FD fd = rfd->fd;
        if(fd > *maxfd || *maxfd == UA_INVALID_FD)
            *maxfd = fd;
        
        if(rfd->listenEvents & UA_FDEVENT_IN)
            FD_SET(fd, readset);
        if(rfd->listenEvents & UA_FDEVENT_OUT)
            FD_SET(fd, writeset);
    }
}

/*********************/
/* EventLoop Task    */
/*********************/

void
UA_EventLoopSTM32_Task(void *pvParameters) {
    UA_EventLoopSTM32 *el = (UA_EventLoopSTM32*)pvParameters;
    UA_EventLoop *el_base = &el->el.eventLoop;
    
    UA_STM32Event event;
    fd_set readset, writeset;
    UA_FD maxfd;
    struct timeval tv;
    TickType_t lastWatchdogRefresh = 0;
    
    UA_LOG_INFO(el_base->logger, UA_LOGCATEGORY_EVENTLOOP,
                "STM32 EventLoop task started");
    
    while(el->el.eventLoop.state != UA_EVENTLOOPSTATE_STOPPED) {
        /* Refresh watchdog if enabled */
        if(el->useWatchdog) {
            TickType_t currentTick = xTaskGetTickCount();
            if((currentTick - lastWatchdogRefresh) >= el->watchdogTimeout) {
                STM32_WatchdogRefresh();
                lastWatchdogRefresh = currentTick;
            }
        }
        
        /* Process timer events */
        UA_DateTime now = UA_DateTime_nowMonotonic();
        UA_Timer_process(&el->el.timer, now, 
                        (UA_TimerExecutionCallback)UA_EventLoop_addDelayedCallback,
                        el);
        
        /* Calculate next timer timeout */
        UA_DateTime nextTimer = UA_Timer_nextRepeatedTime(&el->el.timer);
        UA_UInt32 timeout = 100; /* Default 100ms timeout */
        
        if(nextTimer != UA_INT64_MAX) {
            UA_DateTime diff = nextTimer - now;
            if(diff < 0)
                timeout = 0;
            else if(diff < (UA_DateTime)(UA_UINT32_MAX / UA_DATETIME_MSEC))
                timeout = (UA_UInt32)(diff / UA_DATETIME_MSEC);
        }
        
        /* Set up file descriptor sets */
        setFDSetsFromFDTree(el, &readset, &writeset, &maxfd);
        
        /* Set timeout for select */
        tv.tv_sec = timeout / 1000;
        tv.tv_usec = (timeout % 1000) * 1000;
        
        /* Wait for network events or timeout */
        int selectResult = 0;
        if(maxfd != UA_INVALID_FD) {
            selectResult = select(maxfd + 1, &readset, &writeset, NULL, &tv);
        } else {
            /* No sockets to monitor, just delay */
            vTaskDelay(pdMS_TO_TICKS(timeout));
        }
        
        /* Process socket events */
        if(selectResult > 0) {
            UA_RegisteredFD *rfd, *rfd_tmp;
            ZIP_ITER_SAFE(UA_FDTree, &el->el.fds, rfd, rfd_tmp) {
                UA_FD fd = rfd->fd;
                short events = 0;
                
                if(FD_ISSET(fd, &readset))
                    events |= UA_FDEVENT_IN;
                if(FD_ISSET(fd, &writeset))
                    events |= UA_FDEVENT_OUT;
                
                if(events != 0) {
                    UA_LOG_DEBUG(el_base->logger, UA_LOGCATEGORY_EVENTLOOP,
                                "Processing events %u for fd %u", events, fd);
                    rfd->callback(el_base, rfd, events);
                }
            }
        } else if(selectResult < 0) {
            UA_LOG_WARNING(el_base->logger, UA_LOGCATEGORY_EVENTLOOP,
                          "Select error: %s", strerror(errno));
        }
        
        /* Check for events from other tasks */
        if(xQueueReceive(el->eventQueue, &event, 0) == pdTRUE) {
            switch(event.type) {
                case STM32_EVENT_STOP:
                    UA_LOG_INFO(el_base->logger, UA_LOGCATEGORY_EVENTLOOP,
                               "Stop event received");
                    el->el.eventLoop.state = UA_EVENTLOOPSTATE_STOPPED;
                    break;
                    
                case STM32_EVENT_NETWORK_STATUS:
                    if(event.param) {
                        UA_LOG_INFO(el_base->logger, UA_LOGCATEGORY_EVENTLOOP,
                                   "Network connected");
                    } else {
                        UA_LOG_WARNING(el_base->logger, UA_LOGCATEGORY_EVENTLOOP,
                                      "Network disconnected");
                    }
                    break;
                    
                default:
                    break;
            }
        }
        
        /* Yield to other tasks */
        taskYIELD();
    }
    
    /* Signal that we've stopped */
    xSemaphoreGive(el->stopSemaphore);
    
    UA_LOG_INFO(el_base->logger, UA_LOGCATEGORY_EVENTLOOP,
                "STM32 EventLoop task finished");
    
    vTaskDelete(NULL);
}

/*********************/
/* EventLoop Methods */
/*********************/

static UA_StatusCode
UA_EventLoopSTM32_start(UA_EventLoopSTM32 *el) {
    if(el->el.eventLoop.state != UA_EVENTLOOPSTATE_FRESH &&
       el->el.eventLoop.state != UA_EVENTLOOPSTATE_STOPPED)
        return UA_STATUSCODE_BADINTERNALERROR;
    
    el->el.eventLoop.state = UA_EVENTLOOPSTATE_STARTED;
    
    /* Create FreeRTOS task */
    BaseType_t result = xTaskCreate(
        UA_EventLoopSTM32_Task,
        "UA_EventLoop",
        STM32_EVENTLOOP_TASK_STACK_SIZE,
        el,
        STM32_EVENTLOOP_TASK_PRIORITY,
        &el->eventLoopTask
    );
    
    if(result != pdPASS) {
        el->el.eventLoop.state = UA_EVENTLOOPSTATE_STOPPED;
        return UA_STATUSCODE_BADOUTOFMEMORY;
    }
    
    return UA_STATUSCODE_GOOD;
}

static void
UA_EventLoopSTM32_stop(UA_EventLoopSTM32 *el) {
    if(el->el.eventLoop.state != UA_EVENTLOOPSTATE_STARTED)
        return;
    
    /* Send stop event */
    UA_STM32Event stopEvent = {
        .type = STM32_EVENT_STOP,
        .data = NULL,
        .param = 0
    };
    
    xQueueSend(el->eventQueue, &stopEvent, portMAX_DELAY);
    
    /* Wait for task to finish */
    xSemaphoreTake(el->stopSemaphore, portMAX_DELAY);
    
    el->el.eventLoop.state = UA_EVENTLOOPSTATE_STOPPED;
}

static UA_StatusCode
UA_EventLoopSTM32_run(UA_EventLoopSTM32 *el, UA_UInt32 timeout) {
    /* For FreeRTOS, we start the task and return immediately */
    /* The actual event processing happens in the task */
    
    if(el->el.eventLoop.state == UA_EVENTLOOPSTATE_FRESH) {
        UA_StatusCode res = UA_EventLoopSTM32_start(el);
        if(res != UA_STATUSCODE_GOOD)
            return res;
    }
    
    if(timeout == 0)
        return UA_STATUSCODE_GOOD;
    
    /* Wait for the specified timeout or until stopped */
    TickType_t timeoutTicks = (timeout == UA_UINT32_MAX) ? 
                             portMAX_DELAY : pdMS_TO_TICKS(timeout);
    
    xSemaphoreTake(el->stopSemaphore, timeoutTicks);
    
    return UA_STATUSCODE_GOOD;
}

static void
UA_EventLoopSTM32_free(UA_EventLoopSTM32 *el) {
    /* Stop the event loop */
    UA_EventLoopSTM32_stop(el);
    
    /* Clean up FreeRTOS objects */
    if(el->eventQueue)
        vQueueDelete(el->eventQueue);
    if(el->stopSemaphore)
        vSemaphoreDelete(el->stopSemaphore);
    
    /* Clean up the base event loop */
    UA_EventLoopPOSIX_free(&el->el);
    
    /* Free the event loop structure */
    UA_free(el);
    
    g_eventLoop = NULL;
}

static UA_StatusCode
UA_EventLoopSTM32_registerFD(UA_EventLoopSTM32 *el, UA_RegisteredFD *rfd) {
    /* Set socket to non-blocking */
    UA_StatusCode res = setNonBlocking(rfd->fd);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    res = setNoSigPipe(rfd->fd);
    if(res != UA_STATUSCODE_GOOD)
        return res;
    
    /* Add to the FD tree */
    return UA_EventLoopPOSIX_registerFD(&el->el, rfd);
}

/*********************/
/* Public Functions  */
/*********************/

UA_EventLoop *
UA_EventLoop_new_STM32(const UA_Logger *logger, struct netif *netif) {
    UA_EventLoopSTM32 *el = (UA_EventLoopSTM32*)
        UA_calloc(1, sizeof(UA_EventLoopSTM32));
    if(!el)
        return NULL;
    
    /* Initialize base POSIX event loop */
    if(UA_EventLoopPOSIX_init(&el->el, logger) != UA_STATUSCODE_GOOD) {
        UA_free(el);
        return NULL;
    }
    
    /* Set STM32-specific methods */
    el->el.eventLoop.start = (UA_StatusCode(*)(UA_EventLoop*))UA_EventLoopSTM32_start;
    el->el.eventLoop.stop = (void(*)(UA_EventLoop*))UA_EventLoopSTM32_stop;
    el->el.eventLoop.run = (UA_StatusCode(*)(UA_EventLoop*, UA_UInt32))UA_EventLoopSTM32_run;
    el->el.eventLoop.free = (void(*)(UA_EventLoop*))UA_EventLoopSTM32_free;
    el->el.registerFD = (UA_StatusCode(*)(UA_EventLoopPOSIX*, UA_RegisteredFD*))UA_EventLoopSTM32_registerFD;
    
    /* Create FreeRTOS objects */
    el->eventQueue = xQueueCreate(STM32_EVENTLOOP_QUEUE_SIZE, sizeof(UA_STM32Event));
    el->stopSemaphore = xSemaphoreCreateBinary();
    
    if(!el->eventQueue || !el->stopSemaphore) {
        UA_EventLoopSTM32_free(el);
        return NULL;
    }
    
    /* Set network interface */
    el->netif = netif;
    
    /* Configure watchdog (disabled by default) */
    el->useWatchdog = UA_FALSE;
    el->watchdogTimeout = pdMS_TO_TICKS(1000); /* 1 second default */
    
    g_eventLoop = el;
    
    return &el->el.eventLoop;
}

/* Network status callback from ISR or other tasks */
void
STM32_NetworkStatusCallback(UA_Boolean connected) {
    if(!g_eventLoop)
        return;
    
    UA_STM32Event event = {
        .type = STM32_EVENT_NETWORK_STATUS,
        .data = NULL,
        .param = connected ? 1 : 0
    };
    
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xQueueSendFromISR(g_eventLoop->eventQueue, &event, &xHigherPriorityTaskWoken);
    
    if(xHigherPriorityTaskWoken == pdTRUE) {
        portYIELD_FROM_ISR();
    }
}

/* Default implementations for hardware abstraction functions */
__attribute__((weak)) void STM32_WatchdogRefresh(void) {
    /* Default empty implementation - user should override */
}

__attribute__((weak)) UA_Boolean STM32_IsNetworkConnected(void) {
    /* Default implementation - check lwIP netif status */
    if(g_eventLoop && g_eventLoop->netif) {
        return netif_is_up(g_eventLoop->netif) && netif_is_link_up(g_eventLoop->netif);
    }
    return UA_FALSE;
}

/* Default RTC implementation */
__attribute__((weak)) uint32_t HAL_RTC_GetUnixTimestamp(void) {
    /* Default implementation returns 0 - user should implement with actual RTC */
    return 0;
}

#endif /* UA_ARCHITECTURE_STM32 */
