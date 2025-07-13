# STM32 HAL Integration Header
# This file provides the bridge between open62541 and STM32 HAL

#ifndef STM32_HAL_H_
#define STM32_HAL_H_

/* Include the appropriate STM32 HAL header based on your MCU family */
/* Uncomment the line for your STM32 family */

/* STM32F4 Series */
//#include "stm32f4xx_hal.h"

/* STM32F7 Series */
//#include "stm32f7xx_hal.h"

/* STM32H7 Series */
//#include "stm32h7xx_hal.h"

/* STM32L4 Series */
//#include "stm32l4xx_hal.h"

/* STM32G4 Series */
//#include "stm32g4xx_hal.h"

/* STM32F1 Series */
//#include "stm32f1xx_hal.h"

/* STM32F0 Series */
//#include "stm32f0xx_hal.h"

/* STM32L0 Series */
//#include "stm32l0xx_hal.h"

/* STM32F3 Series */
//#include "stm32f3xx_hal.h"

/* Add other STM32 families as needed */

/* If no specific HAL is included, provide a warning */
#if !defined(STM32F4xx) && !defined(STM32F7xx) && !defined(STM32H7xx) && \
    !defined(STM32L4xx) && !defined(STM32G4xx) && !defined(STM32F1xx) && \
    !defined(STM32F0xx) && !defined(STM32L0xx) && !defined(STM32F3xx)
#warning "No STM32 HAL header included. Please define your STM32 family and include the appropriate HAL header."
#endif

/* Standard C library includes needed for lwIP and open62541 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>

/* FreeRTOS includes */
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include "timers.h"

/* Provide errno definitions if not available */
#ifndef EAGAIN
#define EAGAIN 11
#endif

#ifndef EWOULDBLOCK
#define EWOULDBLOCK EAGAIN
#endif

#ifndef EINPROGRESS
#define EINPROGRESS 115
#endif

#ifndef ECONNRESET
#define ECONNRESET 104
#endif

/* File control definitions for sockets */
#ifndef F_GETFL
#define F_GETFL 3
#endif

#ifndef F_SETFL
#define F_SETFL 4
#endif

#ifndef O_NONBLOCK
#define O_NONBLOCK 04000
#endif

/* Provide fcntl function for lwIP if not available */
#ifndef fcntl
#define fcntl lwip_fcntl
#endif

/* RTC function declaration - to be implemented by user */
uint32_t HAL_RTC_GetUnixTimestamp(void);

/* Watchdog function declaration - to be implemented by user */
void STM32_WatchdogRefresh(void);

/* Network status function declaration - to be implemented by user */
UA_Boolean STM32_IsNetworkConnected(void);

#endif /* STM32_HAL_H_ */
