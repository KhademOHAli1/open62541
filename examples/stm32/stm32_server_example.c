/**
 * STM32 OPC UA Server Example
 * 
 * This example demonstrates how to create an OPC UA server on STM32
 * using FreeRTOS and lwIP for networking.
 * 
 * Requirements:
 * - STM32 microcontroller with Ethernet capability
 * - FreeRTOS
 * - lwIP TCP/IP stack
 * - open62541 library with STM32 port
 */

#include <open62541/server.h>
#include <open62541/server_config_default.h>

/* STM32 HAL and FreeRTOS includes */
#include "main.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

/* lwIP includes */
#include "lwip/tcpip.h"
#include "lwip/dhcp.h"
#include "lwip/netif.h"
#include "ethernetif.h"

/* open62541 STM32 port includes */
#include "../../arch/stm32/eventloop_stm32.h"

/* Configuration */
#define OPC_UA_SERVER_PORT      4840
#define OPC_UA_TASK_PRIORITY    (tskIDLE_PRIORITY + 2)
#define OPC_UA_TASK_STACK_SIZE  (4096)

/* Global variables */
static UA_Server *server = NULL;
static struct netif gnetif;
static TaskHandle_t opcua_task_handle = NULL;

/**
 * Hardware abstraction functions - implement these in your project
 */

/* RTC function - implement to return current Unix timestamp */
uint32_t HAL_RTC_GetUnixTimestamp(void) {
    /* Implement with your RTC */
    /* Example: return HAL_RTC_GetTimeStamp(); */
    return 0; /* Return 0 if no RTC available */
}

/* Watchdog refresh function */
void STM32_WatchdogRefresh(void) {
    /* Implement with your watchdog */
    /* Example: HAL_IWDG_Refresh(&hiwdg); */
}

/* Network status check function */
UA_Boolean STM32_IsNetworkConnected(void) {
    return netif_is_up(&gnetif) && netif_is_link_up(&gnetif);
}

/**
 * Network initialization
 */
static void network_init(void) {
    /* Initialize lwIP */
    tcpip_init(NULL, NULL);
    
    /* Initialize network interface */
    netif_add(&gnetif, NULL, NULL, NULL, NULL, &ethernetif_init, &tcpip_input);
    netif_set_default(&gnetif);
    
    if (netif_is_link_up(&gnetif)) {
        netif_set_up(&gnetif);
    } else {
        netif_set_down(&gnetif);
    }
    
    /* Start DHCP */
    dhcp_start(&gnetif);
    
    /* Wait for network to be ready */
    while (!netif_is_up(&gnetif) || !netif_is_link_up(&gnetif)) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    
    /* Wait for IP address */
    while (dhcp_supplied_address(&gnetif) == 0) {
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

/**
 * Add variables to the server
 */
static void add_server_variables(UA_Server *server) {
    /* Add a simple temperature variable */
    UA_VariableAttributes attr = UA_VariableAttributes_default;
    UA_Double temperature = 25.0;
    UA_Variant_setScalar(&attr.value, &temperature, &UA_TYPES[UA_TYPES_DOUBLE]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "CPU Temperature");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "CPU Temperature");
    attr.dataType = UA_TYPES[UA_TYPES_DOUBLE].typeId;
    attr.valueRank = UA_VALUERANK_SCALAR;
    
    UA_NodeId temperatureNodeId = UA_NODEID_STRING(1, "cpu.temperature");
    UA_QualifiedName temperatureName = UA_QUALIFIEDNAME(1, "CPU Temperature");
    UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
    UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
    
    UA_Server_addVariableNode(server, temperatureNodeId, parentNodeId,
                             parentReferenceNodeId, temperatureName,
                             UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                             attr, NULL, NULL);
    
    /* Add a system uptime variable */
    UA_UInt32 uptime = 0;
    UA_Variant_setScalar(&attr.value, &uptime, &UA_TYPES[UA_TYPES_UINT32]);
    attr.description = UA_LOCALIZEDTEXT("en-US", "System Uptime in seconds");
    attr.displayName = UA_LOCALIZEDTEXT("en-US", "System Uptime");
    attr.dataType = UA_TYPES[UA_TYPES_UINT32].typeId;
    
    UA_NodeId uptimeNodeId = UA_NODEID_STRING(1, "system.uptime");
    UA_QualifiedName uptimeName = UA_QUALIFIEDNAME(1, "System Uptime");
    
    UA_Server_addVariableNode(server, uptimeNodeId, parentNodeId,
                             parentReferenceNodeId, uptimeName,
                             UA_NODEID_NUMERIC(0, UA_NS0ID_BASEDATAVARIABLETYPE),
                             attr, NULL, NULL);
}

/**
 * Update variable values periodically
 */
static void update_variables(UA_Server *server) {
    /* Update temperature (simulate with random value) */
    UA_Double temperature = 20.0 + (rand() % 100) / 10.0;
    UA_NodeId temperatureNodeId = UA_NODEID_STRING(1, "cpu.temperature");
    UA_Variant value;
    UA_Variant_setScalar(&value, &temperature, &UA_TYPES[UA_TYPES_DOUBLE]);
    UA_Server_writeValue(server, temperatureNodeId, value);
    
    /* Update uptime */
    UA_UInt32 uptime = xTaskGetTickCount() / configTICK_RATE_HZ;
    UA_NodeId uptimeNodeId = UA_NODEID_STRING(1, "system.uptime");
    UA_Variant_setScalar(&value, &uptime, &UA_TYPES[UA_TYPES_UINT32]);
    UA_Server_writeValue(server, uptimeNodeId, value);
}

/**
 * OPC UA Server Task
 */
static void opcua_server_task(void *pvParameters) {
    /* Wait for network to be initialized */
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    /* Create STM32 EventLoop */
    UA_EventLoop *eventLoop = UA_EventLoop_new_STM32(UA_Log_Stdout, &gnetif);
    if (!eventLoop) {
        printf("Failed to create STM32 EventLoop\n");
        vTaskDelete(NULL);
        return;
    }
    
    /* Create server configuration */
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    if (!config) {
        printf("Failed to create server configuration\n");
        UA_EventLoop_delete(eventLoop);
        vTaskDelete(NULL);
        return;
    }
    
    /* Set the EventLoop for the server */
    config->eventLoop = eventLoop;
    
    /* Set server port */
    config->networkLayers[0].discoveryUrl = 
        UA_STRING_ALLOC("opc.tcp://0.0.0.0:4840");
    
    /* Create server */
    server = UA_Server_newWithConfig(config);
    if (!server) {
        printf("Failed to create OPC UA server\n");
        UA_ServerConfig_delete(config);
        UA_EventLoop_delete(eventLoop);
        vTaskDelete(NULL);
        return;
    }
    
    /* Add variables to the server */
    add_server_variables(server);
    
    /* Start the server */
    UA_StatusCode retval = UA_Server_runBootstrap(server);
    if (retval != UA_STATUSCODE_GOOD) {
        printf("Failed to start OPC UA server: %s\n", UA_StatusCode_name(retval));
        goto cleanup;
    }
    
    printf("OPC UA Server started on port %d\n", OPC_UA_SERVER_PORT);
    printf("Server endpoint: opc.tcp://%s:%d\n", 
           ip4addr_ntoa(netif_ip4_addr(&gnetif)), OPC_UA_SERVER_PORT);
    
    /* Main server loop */
    TickType_t lastUpdate = xTaskGetTickCount();
    
    while (1) {
        /* Update variables every 5 seconds */
        TickType_t now = xTaskGetTickCount();
        if ((now - lastUpdate) >= pdMS_TO_TICKS(5000)) {
            update_variables(server);
            lastUpdate = now;
        }
        
        /* Let the server process one iteration */
        UA_Server_run_iterate(server, 100);
        
        /* Yield to other tasks */
        vTaskDelay(pdMS_TO_TICKS(10));
    }
    
cleanup:
    UA_Server_delete(server);
    UA_EventLoop_delete(eventLoop);
    printf("OPC UA Server stopped\n");
    vTaskDelete(NULL);
}

/**
 * Initialize and start OPC UA server
 */
void opcua_server_init(void) {
    /* Initialize network first */
    network_init();
    
    /* Create OPC UA server task */
    BaseType_t result = xTaskCreate(
        opcua_server_task,
        "OPC_UA_Server",
        OPC_UA_TASK_STACK_SIZE,
        NULL,
        OPC_UA_TASK_PRIORITY,
        &opcua_task_handle
    );
    
    if (result != pdPASS) {
        printf("Failed to create OPC UA server task\n");
    }
}

/**
 * Stop OPC UA server
 */
void opcua_server_stop(void) {
    if (server) {
        UA_Server_shutdownAll(server);
    }
    
    if (opcua_task_handle) {
        vTaskDelete(opcua_task_handle);
        opcua_task_handle = NULL;
    }
}

/**
 * Example main function - call this from your STM32 main()
 */
void stm32_opcua_main(void) {
    /* Initialize your STM32 peripherals here */
    /* HAL_Init(), SystemClock_Config(), MX_GPIO_Init(), etc. */
    
    /* Initialize OPC UA server */
    opcua_server_init();
    
    /* Start FreeRTOS scheduler */
    vTaskStartScheduler();
    
    /* Should never reach here */
    while (1) {
        HAL_Delay(1000);
    }
}
