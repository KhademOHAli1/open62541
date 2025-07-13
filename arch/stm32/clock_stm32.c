/* This work is licensed under a Creative Commons CCZero 1.0 Universal License.
 * See http://creativecommons.org/publicdomain/zero/1.0/ for more information.
 *
 *    Copyright 2025 (c) STM32 Port
 */

#include <open62541/types.h>

#ifdef UA_ARCHITECTURE_STM32

#include "FreeRTOS.h"
#include "task.h"
#include "stm32_hal.h"

/* External RTC function - to be implemented by user */
extern uint32_t HAL_RTC_GetUnixTimestamp(void);

/* The current time in UTC time */
UA_DateTime UA_DateTime_now(void) {
    /* Try to get real time from RTC if available */
    uint32_t unixTime = HAL_RTC_GetUnixTimestamp();
    if (unixTime > 0) {
        return ((UA_DateTime)unixTime * UA_DATETIME_SEC) + UA_DATETIME_UNIX_EPOCH;
    }
    
    /* Fallback to FreeRTOS ticks if no RTC available */
    UA_DateTime microSeconds = ((UA_DateTime)xTaskGetTickCount()) * (1000000 / configTICK_RATE_HZ);
    return ((microSeconds / 1000000) * UA_DATETIME_SEC) + 
           ((microSeconds % 1000000) * UA_DATETIME_USEC) + UA_DATETIME_UNIX_EPOCH;
}

/* Offset between local time and UTC time */
UA_Int64 UA_DateTime_localTimeUtcOffset(void) {
    return 0; /* Set to zero for embedded systems, adjust if timezone support needed */
}

/* CPU clock invariant to system time changes. Use only to measure durations,
 * not absolute time. */
UA_DateTime UA_DateTime_nowMonotonic(void) {
    return (((UA_DateTime)xTaskGetTickCount()) * 1000 / configTICK_RATE_HZ) * UA_DATETIME_MSEC;
}

#endif /* UA_ARCHITECTURE_STM32 */
