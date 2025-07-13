/**
 * STM32 Configuration Template for open62541
 * 
 * This file provides configuration templates for different STM32 families
 * and application scenarios. Copy and modify as needed for your project.
 */

#ifndef STM32_OPC_UA_CONFIG_H_
#define STM32_OPC_UA_CONFIG_H_

/* ========================================================================= */
/* STM32 Family Selection - Uncomment the one you're using                  */
/* ========================================================================= */

/* STM32F4 Series (e.g., STM32F407, STM32F429) */
//#define STM32F4xx
//#include "stm32f4xx_hal.h"

/* STM32F7 Series (e.g., STM32F746, STM32F769) */
//#define STM32F7xx
//#include "stm32f7xx_hal.h"

/* STM32H7 Series (e.g., STM32H743, STM32H750) */
//#define STM32H7xx
//#include "stm32h7xx_hal.h"

/* STM32L4 Series (e.g., STM32L476, STM32L496) */
//#define STM32L4xx
//#include "stm32l4xx_hal.h"

/* STM32G4 Series (e.g., STM32G474, STM32G484) */
//#define STM32G4xx
//#include "stm32g4xx_hal.h"

/* ========================================================================= */
/* Application Configuration                                                 */
/* ========================================================================= */

/* OPC UA Server Configuration */
#define OPC_UA_SERVER_PORT              4840
#define OPC_UA_MAX_CONNECTIONS          5
#define OPC_UA_BUFFER_SIZE              8192
#define OPC_UA_MAX_NODES                100

/* FreeRTOS Task Configuration */
#define OPC_UA_TASK_STACK_SIZE          (4 * 1024)    /* 4KB stack */
#define OPC_UA_TASK_PRIORITY            (tskIDLE_PRIORITY + 2)
#define NETWORK_TASK_STACK_SIZE         (2 * 1024)    /* 2KB stack */
#define NETWORK_TASK_PRIORITY           (tskIDLE_PRIORITY + 3)

/* EventLoop Configuration */
#define STM32_EVENTLOOP_TASK_PRIORITY   (tskIDLE_PRIORITY + 3)
#define STM32_EVENTLOOP_TASK_STACK_SIZE (2 * 1024)    /* 2KB stack */
#define STM32_EVENTLOOP_QUEUE_SIZE      16

/* Memory Configuration */
#define HEAP_SIZE                       (32 * 1024)   /* 32KB heap */

/* ========================================================================= */
/* Hardware Configuration Templates                                         */
/* ========================================================================= */

/* Template 1: STM32F407 Discovery Board */
#ifdef STM32F407xx_DISCOVERY
    #define MCU_FLASH_SIZE              (1024 * 1024)  /* 1MB */
    #define MCU_RAM_SIZE                (192 * 1024)   /* 192KB */
    #define MCU_FREQ                    168000000      /* 168MHz */
    #define ETH_PHY_ADDRESS             0x01
    #define USE_DHCP                    1
    #define USE_RTC                     1
    #define USE_WATCHDOG                1
#endif

/* Template 2: STM32F746 Discovery Board */
#ifdef STM32F746xx_DISCOVERY
    #define MCU_FLASH_SIZE              (1024 * 1024)  /* 1MB */
    #define MCU_RAM_SIZE                (320 * 1024)   /* 320KB */
    #define MCU_FREQ                    216000000      /* 216MHz */
    #define ETH_PHY_ADDRESS             0x00
    #define USE_DHCP                    1
    #define USE_RTC                     1
    #define USE_WATCHDOG                1
#endif

/* Template 3: STM32H743 Nucleo Board */
#ifdef STM32H743xx_NUCLEO
    #define MCU_FLASH_SIZE              (2048 * 1024)  /* 2MB */
    #define MCU_RAM_SIZE                (1024 * 1024)  /* 1MB */
    #define MCU_FREQ                    400000000      /* 400MHz */
    #define ETH_PHY_ADDRESS             0x00
    #define USE_DHCP                    1
    #define USE_RTC                     1
    #define USE_WATCHDOG                1
#endif

/* Template 4: Custom Low-Power Configuration */
#ifdef STM32_LOW_POWER
    #define OPC_UA_MAX_CONNECTIONS      2
    #define OPC_UA_BUFFER_SIZE          4096
    #define OPC_UA_MAX_NODES            50
    #define OPC_UA_TASK_STACK_SIZE      (2 * 1024)     /* 2KB stack */
    #define HEAP_SIZE                   (16 * 1024)    /* 16KB heap */
    #define USE_WATCHDOG                1
    #define USE_POWER_SAVING            1
#endif

/* ========================================================================= */
/* open62541 Configuration                                                  */
/* ========================================================================= */

/* Logging Configuration */
#ifdef DEBUG
    #define UA_LOGLEVEL                 200    /* DEBUG */
#else
    #define UA_LOGLEVEL                 400    /* WARNING */
#endif

/* Feature Configuration */
#define UA_ENABLE_DISCOVERY             0      /* Disable to save memory */
#define UA_ENABLE_SUBSCRIPTIONS_EVENTS  0      /* Disable to save memory */
#define UA_ENABLE_HISTORIZING           0      /* Disable to save memory */
#define UA_ENABLE_EXPERIMENTAL          0      /* Disable experimental features */

/* Network Configuration */
#define UA_ENABLE_JSON_ENCODING         0      /* Disable to save memory */
#define UA_ENABLE_XML_ENCODING          0      /* Disable to save memory */

/* Security Configuration */
#define UA_ENABLE_ENCRYPTION            0      /* Basic security only */

/* ========================================================================= */
/* lwIP Configuration Hints                                                 */
/* ========================================================================= */

/*
 * Add these to your lwipopts.h file:
 * 
 * #define LWIP_SOCKET                 1
 * #define LWIP_NETCONN                0
 * #define LWIP_NETIF_HOSTNAME         1
 * #define LWIP_SO_RCVTIMEO            1
 * #define LWIP_SO_SNDTIMEO            1
 * #define LWIP_TCP_KEEPALIVE          1
 * #define LWIP_STATS                  0
 * #define MEMP_NUM_TCP_PCB            OPC_UA_MAX_CONNECTIONS + 2
 * #define MEMP_NUM_TCP_PCB_LISTEN     2
 * #define MEMP_NUM_NETCONN            0
 * #define PBUF_POOL_SIZE              16
 * #define TCP_MSS                     1460
 * #define TCP_SND_BUF                 (4 * TCP_MSS)
 * #define TCP_WND                     (4 * TCP_MSS)
 */

/* ========================================================================= */
/* FreeRTOS Configuration Hints                                             */
/* ========================================================================= */

/*
 * Add these to your FreeRTOSConfig.h file:
 * 
 * #define configUSE_PREEMPTION                    1
 * #define configUSE_IDLE_HOOK                     0
 * #define configUSE_TICK_HOOK                     0
 * #define configCPU_CLOCK_HZ                      MCU_FREQ
 * #define configTICK_RATE_HZ                      1000
 * #define configMAX_PRIORITIES                    7
 * #define configMINIMAL_STACK_SIZE                128
 * #define configTOTAL_HEAP_SIZE                   HEAP_SIZE
 * #define configMAX_TASK_NAME_LEN                 16
 * #define configUSE_16_BIT_TICKS                  0
 * #define configIDLE_SHOULD_YIELD                 1
 * #define configUSE_MUTEXES                       1
 * #define configUSE_RECURSIVE_MUTEXES             1
 * #define configUSE_COUNTING_SEMAPHORES           1
 * #define configQUEUE_REGISTRY_SIZE               8
 * #define configUSE_QUEUE_SETS                    0
 * #define configUSE_TIME_SLICING                  1
 * #define configUSE_NEWLIB_REENTRANT              0
 * #define configENABLE_BACKWARD_COMPATIBILITY     0
 * #define configNUM_THREAD_LOCAL_STORAGE_POINTERS 5
 * #define configSTACK_DEPTH_TYPE                  uint16_t
 * #define configMESSAGE_BUFFER_LENGTH_TYPE        size_t
 */

/* ========================================================================= */
/* Example Hardware Abstraction Implementation                              */
/* ========================================================================= */

#ifdef EXAMPLE_IMPLEMENTATION

/* External variables - define these in your main application */
extern RTC_HandleTypeDef hrtc;
extern IWDG_HandleTypeDef hiwdg;
extern struct netif gnetif;

/* RTC implementation example */
uint32_t HAL_RTC_GetUnixTimestamp(void) {
    #if USE_RTC
    RTC_TimeTypeDef sTime;
    RTC_DateTypeDef sDate;
    
    HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    
    /* Convert RTC time to Unix timestamp */
    /* Implementation depends on your RTC setup */
    return convert_rtc_to_unix(&sTime, &sDate);
    #else
    return 0;  /* No RTC available */
    #endif
}

/* Watchdog implementation example */
void STM32_WatchdogRefresh(void) {
    #if USE_WATCHDOG
    HAL_IWDG_Refresh(&hiwdg);
    #endif
}

/* Network status implementation example */
UA_Boolean STM32_IsNetworkConnected(void) {
    return netif_is_up(&gnetif) && netif_is_link_up(&gnetif);
}

#endif /* EXAMPLE_IMPLEMENTATION */

#endif /* STM32_OPC_UA_CONFIG_H_ */
