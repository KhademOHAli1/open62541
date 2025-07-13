# STM32 Port for open62541 OPC UA Library

This directory contains the STM32 microcontroller port for the open62541 OPC UA library. This port enables running OPC UA servers and clients on STM32 microcontrollers using FreeRTOS and lwIP.

## Features

- **FreeRTOS Integration**: Uses FreeRTOS tasks for event loop management
- **lwIP Networking**: Full TCP/IP networking support through lwIP
- **Low Memory Footprint**: Optimized for memory-constrained embedded systems
- **Hardware Abstraction**: Clean abstraction layer for STM32 HAL integration
- **Watchdog Support**: Optional watchdog integration for system reliability
- **RTC Support**: Real-time clock integration for accurate timestamps

## Requirements

### Hardware
- STM32 microcontroller with at least 256KB Flash and 64KB RAM
- Ethernet capability (built-in or external PHY)
- Optional: RTC for accurate timestamps
- Optional: Hardware watchdog

### Software
- STM32 HAL library
- FreeRTOS (v10.0 or later)
- lwIP TCP/IP stack (v2.1 or later)
- ARM GCC toolchain
- CMake (v3.16 or later)

## Architecture Overview

The STM32 port consists of several key components:

### 1. Clock Implementation (`clock_stm32.c`)
- Provides time functions required by open62541
- Supports both FreeRTOS tick-based timing and RTC-based real-time
- Implements `UA_DateTime_now()`, `UA_DateTime_nowMonotonic()`, and `UA_DateTime_localTimeUtcOffset()`

### 2. EventLoop Implementation (`eventloop_stm32.c`)
- FreeRTOS-based event loop for handling network events and timers
- Integrates with lwIP for socket operations
- Supports non-blocking I/O and multiplexed connections
- Optional watchdog integration

### 3. TCP Implementation (`eventloop_stm32_tcp.c`)
- TCP server and client connection management
- Built on top of lwIP sockets API
- Supports multiple concurrent connections
- Optimized for embedded resource constraints

### 4. Hardware Abstraction (`stm32_hal.h`)
- Bridge between open62541 and STM32 HAL
- Defines interfaces for RTC, watchdog, and network status
- Provides weak default implementations

## Usage

### 1. Configure Your Project

Add the following to your CMakeLists.txt:

```cmake
# Set paths to your development environment
set(STM32_HAL_PATH "path/to/stm32/hal")
set(FREERTOS_PATH "path/to/freertos")
set(LWIP_PATH "path/to/lwip")

# Configure open62541 for STM32
set(UA_ARCHITECTURE "stm32" CACHE STRING "Architecture" FORCE)
set(UA_ENABLE_AMALGAMATION ON CACHE BOOL "Enable amalgamation" FORCE)

# Include open62541
add_subdirectory(path/to/open62541)

# Link your application
target_link_libraries(your_app open62541)
```

### 2. Implement Hardware Abstraction Functions

Implement these functions in your application:

```c
#include "arch/stm32/stm32_hal.h"

// RTC function - return current Unix timestamp
uint32_t HAL_RTC_GetUnixTimestamp(void) {
    // Your RTC implementation
    return your_rtc_get_timestamp();
}

// Watchdog refresh function
void STM32_WatchdogRefresh(void) {
    // Your watchdog implementation
    HAL_IWDG_Refresh(&hiwdg);
}

// Network status check
UA_Boolean STM32_IsNetworkConnected(void) {
    // Check your network interface status
    return netif_is_up(&your_netif);
}
```

### 3. Initialize Network and Create Server

```c
#include <open62541/server.h>
#include "arch/stm32/eventloop_stm32.h"

void opcua_server_init(void) {
    // Initialize lwIP and network interface
    tcpip_init(NULL, NULL);
    netif_add(&gnetif, NULL, NULL, NULL, NULL, &ethernetif_init, &tcpip_input);
    
    // Create STM32 EventLoop
    UA_EventLoop *eventLoop = UA_EventLoop_new_STM32(UA_Log_Stdout, &gnetif);
    
    // Create server configuration
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    config->eventLoop = eventLoop;
    
    // Create and start server
    UA_Server *server = UA_Server_newWithConfig(config);
    UA_Server_runBootstrap(server);
    
    // Server main loop in FreeRTOS task
    while (1) {
        UA_Server_run_iterate(server, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}
```

### 4. Configure STM32 HAL Header

Edit `arch/stm32/stm32_hal.h` to include your STM32 family HAL:

```c
/* Uncomment for your STM32 family */
#include "stm32f4xx_hal.h"  // For STM32F4
// #include "stm32f7xx_hal.h"  // For STM32F7
// #include "stm32h7xx_hal.h"  // For STM32H7
```

## Configuration Options

### Memory Optimization

The STM32 port automatically enables several memory optimizations:

- Log level set to WARNING (400) to reduce string storage
- Discovery service disabled
- Event subscriptions disabled
- Historizing disabled
- Single-file amalgamation enabled

### Custom Configuration

You can further customize the build by setting these CMake variables:

```cmake
# Networking
set(LWIP_PATH "path/to/lwip")

# Memory settings
set(UA_LOGLEVEL 500)  # ERROR level only
set(UA_ENABLE_METHODCALLS OFF)  # Disable method calls
set(UA_ENABLE_SUBSCRIPTIONS OFF)  # Disable all subscriptions

# STM32 specific
set(STM32_EVENTLOOP_TASK_PRIORITY 3)
set(STM32_EVENTLOOP_TASK_STACK_SIZE 2048)
```

## Example Application

See `examples/stm32/stm32_server_example.c` for a complete example that demonstrates:

- Network initialization with DHCP
- OPC UA server creation and configuration
- Variable creation and periodic updates
- FreeRTOS task integration
- Hardware abstraction implementation

## Memory Requirements

Typical memory usage for a basic OPC UA server:

- **Flash**: ~150-200 KB (with amalgamation and optimizations)
- **RAM**: ~40-60 KB (depending on configuration and concurrent connections)
- **Stack**: ~2-4 KB per task

## Troubleshooting

### Common Issues

1. **Build Errors**: Ensure all paths are correctly set and STM32 HAL is properly configured
2. **Network Issues**: Check lwIP configuration and Ethernet PHY setup
3. **Memory Issues**: Reduce log level, disable unused features, check stack sizes
4. **Timing Issues**: Verify FreeRTOS tick rate and RTC configuration

### Debug Tips

- Enable debug logging: `set(UA_LOGLEVEL 200)`
- Monitor stack usage with FreeRTOS stack overflow detection
- Use STM32 debugging tools to monitor memory usage
- Check network connectivity with ping and basic TCP tests

## Contributing

When contributing to the STM32 port:

1. Test on multiple STM32 families when possible
2. Maintain compatibility with existing FreeRTOS and lwIP versions
3. Keep memory usage optimized
4. Document any hardware-specific requirements
5. Include example code for new features

## License

This STM32 port is licensed under the same terms as open62541:
- Core port files: Mozilla Public License v2.0 (MPLv2)
- Example code: Creative Commons CCZero 1.0 Universal License

## Support

For STM32-specific issues:
1. Check the troubleshooting section
2. Review the example application
3. Submit issues to the open62541 GitHub repository with [STM32] prefix

For general open62541 usage, refer to the main open62541 documentation.
