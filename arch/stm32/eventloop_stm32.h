/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *    Copyright 2025 (c) STM32 Port - based on LWIP EventLoop
 */

#ifndef UA_EVENTLOOP_STM32_H_
#define UA_EVENTLOOP_STM32_H_

#include <open62541/config.h>

#if defined(UA_ARCHITECTURE_STM32)

#include <open62541/plugin/eventloop.h>
#include "../common/eventloop_common.h"

/* STM32 HAL includes */
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"

/* lwIP includes for networking */
#include "lwip/tcpip.h"
#include "lwip/sockets.h"
#include "lwip/inet.h"
#include "lwip/netdb.h"
#include "lwip/errno.h"
#include "lwip/ip_addr.h"
#include "lwip/opt.h"
#include "lwip/netif.h"

#include "../../deps/mp_printf.h"
#include "../common/timer.h"

/* STM32-specific definitions */
#define STM32_EVENTLOOP_TASK_PRIORITY    (tskIDLE_PRIORITY + 3)
#define STM32_EVENTLOOP_TASK_STACK_SIZE  (2048)
#define STM32_EVENTLOOP_QUEUE_SIZE       (16)

#if !LWIP_NETIF_HOSTNAME
#define DEFAULT_HOSTNAME "stm32-ua-device"
#endif

/*********************/
/* Network Definitions */
/*********************/

#define NI_NUMERICHOST 1
#define NI_NUMERICSERV 2
#define NI_NOFQDN      4
#define NI_NAMEREQD    8

#define NI_MAXHOST 1025
#define NI_MAXSERV 32

#if !LWIP_DNS
#define AI_PASSIVE      0x01
#define AI_CANONNAME    0x02
#define AI_NUMERICHOST  0x04
#define AI_NUMERICSERV  0x08
#define AI_V4MAPPED     0x800
#define AI_ALL          0x100
#define AI_ADDRCONFIG   0x400

#define EAI_BADFLAGS    -1
#define EAI_NONAME      -2
#define EAI_AGAIN       -3
#define EAI_FAIL        -4
#define EAI_FAMILY      -6
#define EAI_SOCKTYPE    -7
#define EAI_SERVICE     -8
#define EAI_MEMORY      -10
#define EAI_SYSTEM      -11
#define EAI_OVERFLOW    -12

struct addrinfo {
    int ai_flags;
    int ai_family;
    int ai_socktype;
    int ai_protocol;
    socklen_t ai_addrlen;
    struct sockaddr *ai_addr;
    char *ai_canonname;
    struct addrinfo *ai_next;
};
#endif

/*********************/
/* STM32 EventLoop   */
/*********************/

typedef struct {
    UA_EventLoopPOSIX el;

    /* STM32/FreeRTOS specific */
    TaskHandle_t eventLoopTask;
    QueueHandle_t eventQueue;
    SemaphoreHandle_t stopSemaphore;
    
    /* Network interface */
    struct netif *netif;
    
    /* Watchdog support */
    UA_Boolean useWatchdog;
    uint32_t watchdogTimeout;
    
} UA_EventLoopSTM32;

/* Event types for FreeRTOS queue */
typedef enum {
    STM32_EVENT_TIMER,
    STM32_EVENT_SOCKET,
    STM32_EVENT_STOP,
    STM32_EVENT_NETWORK_STATUS
} UA_STM32EventType;

typedef struct {
    UA_STM32EventType type;
    void *data;
    uint32_t param;
} UA_STM32Event;

/* Function declarations */
UA_EventLoop *
UA_EventLoop_new_STM32(const UA_Logger *logger,
                       struct netif *netif);

UA_StatusCode
UA_EventLoopSTM32_addTimer(UA_EventLoopSTM32 *el,
                          UA_DelayedCallback *dc);

UA_StatusCode
UA_EventLoopSTM32_removeTimer(UA_EventLoopSTM32 *el,
                             UA_DelayedCallback *dc);

void
UA_EventLoopSTM32_Task(void *pvParameters);

/* Network utility functions */
int stm32_getnameinfo(const struct sockaddr *sa, socklen_t salen,
                     char *host, size_t hostlen,
                     char *serv, size_t servlen, int flags);

int stm32_getaddrinfo(const char *node, const char *service,
                     const struct addrinfo *hints,
                     struct addrinfo **res);

void stm32_freeaddrinfo(struct addrinfo *res);

/* Hardware abstraction functions - to be implemented by user */
void STM32_WatchdogRefresh(void);
UA_Boolean STM32_IsNetworkConnected(void);
void STM32_NetworkStatusCallback(UA_Boolean connected);

#endif /* UA_ARCHITECTURE_STM32 */

#endif /* UA_EVENTLOOP_STM32_H_ */
