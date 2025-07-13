# STM32 Port Integration Guide

This guide explains how to integrate the open62541 STM32 port into your existing STM32 project or create a new one from scratch.

## Quick Start

### Option 1: Using the Example Project

1. **Navigate to the STM32 example directory:**
   ```bash
   cd examples/stm32
   ```

2. **Set up your environment variables:**
   ```bash
   export STM32_HAL_PATH="/path/to/your/STM32Cube_FW_XX/Drivers/STM32XXxx_HAL_Driver"
   export FREERTOS_PATH="/path/to/your/FreeRTOS"
   export LWIP_PATH="/path/to/your/lwip"
   export ARM_TOOLCHAIN_PATH="/path/to/arm-none-eabi-gcc/bin"
   ```

3. **Build the example:**
   ```bash
   ./build_stm32.sh --family STM32F4xx --type Release
   ```

### Option 2: Integrating into Existing Project

1. **Add open62541 as a submodule:**
   ```bash
   cd your_stm32_project
   git submodule add https://github.com/open62541/open62541.git
   ```

2. **Configure your CMakeLists.txt:**
   ```cmake
   # Set architecture before including open62541
   set(UA_ARCHITECTURE "stm32" CACHE STRING "Architecture" FORCE)
   
   # Include open62541
   add_subdirectory(open62541)
   
   # Link to your application
   target_link_libraries(your_app open62541)
   ```

3. **Configure your main application:**
   ```c
   #include <open62541/server.h>
   #include "open62541/arch/stm32/eventloop_stm32.h"
   
   void your_opcua_init(void) {
       // Initialize network interface
       // Create EventLoop
       // Create and configure server
       // Start server in FreeRTOS task
   }
   ```

## File Structure

After integration, your project should have:

```
your_stm32_project/
├── open62541/                     # open62541 library
│   ├── arch/stm32/               # STM32 port files
│   │   ├── clock_stm32.c
│   │   ├── eventloop_stm32.h
│   │   ├── eventloop_stm32.c
│   │   ├── eventloop_stm32_tcp.c
│   │   ├── stm32.cmake
│   │   └── stm32_hal.h
│   └── examples/stm32/           # Example and templates
├── your_application/
├── STM32_HAL/                    # Your STM32 HAL
├── FreeRTOS/                     # Your FreeRTOS
├── lwip/                         # Your lwIP
└── CMakeLists.txt                # Your main CMake file
```

## Step-by-Step Integration

### 1. Hardware Requirements

**Minimum STM32 Requirements:**
- **Flash:** 256KB (512KB recommended)
- **RAM:** 64KB (128KB recommended)  
- **Ethernet:** Built-in or external PHY
- **Crystal:** For accurate timing (optional RTC)

**Recommended Development Boards:**
- STM32F407 Discovery
- STM32F746 Discovery  
- STM32H743 Nucleo
- Any STM32 with Ethernet capability

### 2. Software Setup

**Required Software:**
```bash
# Install ARM GCC toolchain
# Ubuntu/Debian:
sudo apt-get install gcc-arm-none-eabi

# macOS with Homebrew:
brew install arm-none-eabi-gcc

# Windows: Download from ARM website
```

**Required Libraries:**
- STM32 HAL (from STM32CubeMX or STM32Cube firmware)
- FreeRTOS v10.0+ 
- lwIP v2.1+
- CMake 3.16+

### 3. Project Configuration

**3.1. Configure STM32CubeMX:**
- Enable Ethernet (ETH peripheral)
- Enable FreeRTOS
- Enable lwIP with socket API
- Configure adequate heap size (32KB minimum)
- Enable RTC (optional, for timestamps)
- Enable IWDG (optional, for watchdog)

**3.2. Configure FreeRTOS (in FreeRTOSConfig.h):**
```c
#define configUSE_PREEMPTION                    1
#define configCPU_CLOCK_HZ                      168000000  // Adjust for your MCU
#define configTICK_RATE_HZ                      1000
#define configTOTAL_HEAP_SIZE                   (32 * 1024)
#define configUSE_MUTEXES                       1
#define configUSE_RECURSIVE_MUTEXES             1
#define configUSE_COUNTING_SEMAPHORES           1
```

**3.3. Configure lwIP (in lwipopts.h):**
```c
#define LWIP_SOCKET                             1
#define LWIP_NETCONN                            0
#define LWIP_NETIF_HOSTNAME                     1
#define LWIP_SO_RCVTIMEO                        1
#define LWIP_SO_SNDTIMEO                        1
#define LWIP_TCP_KEEPALIVE                      1
#define MEMP_NUM_TCP_PCB                        8
#define MEMP_NUM_TCP_PCB_LISTEN                 2
#define TCP_MSS                                 1460
#define TCP_SND_BUF                             (4 * TCP_MSS)
#define TCP_WND                                 (4 * TCP_MSS)
```

### 4. Code Integration

**4.1. Include Headers:**
```c
#include <open62541/server.h>
#include <open62541/server_config_default.h>
#include "open62541/arch/stm32/eventloop_stm32.h"
```

**4.2. Implement Hardware Abstraction:**
```c
// In your main.c or dedicated file
uint32_t HAL_RTC_GetUnixTimestamp(void) {
    // Your RTC implementation
    return get_unix_timestamp_from_rtc();
}

void STM32_WatchdogRefresh(void) {
    HAL_IWDG_Refresh(&hiwdg);
}

UA_Boolean STM32_IsNetworkConnected(void) {
    return netif_is_up(&gnetif) && netif_is_link_up(&gnetif);
}
```

**4.3. Create OPC UA Server Task:**
```c
void opcua_server_task(void *pvParameters) {
    // Wait for network initialization
    while (!STM32_IsNetworkConnected()) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    
    // Create EventLoop
    UA_EventLoop *eventLoop = UA_EventLoop_new_STM32(UA_Log_Stdout, &gnetif);
    
    // Create server configuration
    UA_ServerConfig *config = UA_ServerConfig_new_default();
    config->eventLoop = eventLoop;
    
    // Create and start server
    UA_Server *server = UA_Server_newWithConfig(config);
    UA_Server_runBootstrap(server);
    
    // Main server loop
    while (1) {
        UA_Server_run_iterate(server, 100);
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void start_opcua_server(void) {
    xTaskCreate(opcua_server_task, "OPC_UA", 4096, NULL, 
                tskIDLE_PRIORITY + 2, NULL);
}
```

**4.4. Initialize in main():**
```c
int main(void) {
    // HAL and hardware initialization
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_ETH_Init();
    MX_RTC_Init();  // Optional
    MX_IWDG_Init(); // Optional
    
    // Initialize network
    MX_LWIP_Init();
    
    // Start OPC UA server
    start_opcua_server();
    
    // Start FreeRTOS scheduler
    vTaskStartScheduler();
    
    while (1) {
        // Should never reach here
    }
}
```

### 5. Building and Deployment

**5.1. Build with CMake:**
```bash
mkdir build && cd build
cmake -DCMAKE_TOOLCHAIN_FILE=../toolchain_stm32.cmake \
      -DSTM32_FAMILY=STM32F4xx \
      -DCMAKE_BUILD_TYPE=Release \
      ..
make
```

**5.2. Flash to Device:**
```bash
# Using ST-LINK
st-flash write your_project.bin 0x8000000

# Using OpenOCD
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
        -c "program your_project.elf verify reset exit"
```

### 6. Testing and Verification

**6.1. Network Connectivity:**
- Connect STM32 to network via Ethernet
- Verify DHCP assigns IP address
- Test basic network connectivity (ping)

**6.2. OPC UA Connectivity:**
```bash
# Test with UaExpert or similar OPC UA client
# Connect to: opc.tcp://STM32_IP_ADDRESS:4840
```

**6.3. Monitor via Serial:**
- Connect UART for debug output
- Monitor server startup and client connections

### 7. Optimization and Troubleshooting

**7.1. Memory Optimization:**
- Reduce log level to WARNING or ERROR
- Disable unused OPC UA features
- Optimize FreeRTOS heap and stack sizes
- Use -Os optimization flag

**7.2. Performance Tuning:**
- Adjust FreeRTOS task priorities
- Optimize lwIP buffer sizes
- Monitor CPU usage and memory usage

**7.3. Common Issues:**
- **Build errors:** Check toolchain and library paths
- **Network issues:** Verify lwIP configuration and PHY setup  
- **Memory errors:** Increase heap size or reduce feature set
- **Connection issues:** Check firewall and network configuration

### 8. Advanced Configuration

**8.1. Security:**
```c
// Enable security policies (requires more memory)
#define UA_ENABLE_ENCRYPTION 1
```

**8.2. Custom Data Types:**
```c
// Add custom variables and methods
UA_Server_addVariableNode(server, nodeId, parentNodeId, ...);
```

**8.3. Multiple Network Interfaces:**
```c
// Configure additional network layers
config->networkLayers[1] = UA_ConnectionManager_new_TCP(...);
```

## Support and Resources

- **Documentation:** See `examples/stm32/README.md`
- **Configuration Templates:** Use `stm32_config_template.h`
- **Example Code:** Study `stm32_server_example.c`
- **Build Scripts:** Use `build_stm32.sh` as reference
- **Issues:** Report STM32-specific issues with [STM32] prefix

## Performance Expectations

**Typical Performance (STM32F407 @ 168MHz):**
- **Memory Usage:** ~50KB RAM, ~200KB Flash
- **Connections:** 5-10 concurrent clients
- **Throughput:** ~1MB/s data transfer
- **Response Time:** <10ms for simple read operations

**Optimization Tips:**
- Use Release build (-Os optimization)
- Enable only required OPC UA features  
- Optimize lwIP buffer configuration
- Use appropriate FreeRTOS tick rate (1000Hz recommended)
