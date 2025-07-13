# STM32 OPC UA Example CMakeLists.txt
# This file shows how to build open62541 for STM32

cmake_minimum_required(VERSION 3.16)

# Set cross-compilation toolchain
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Set toolchain paths
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)

# Prevent CMake from testing the compiler
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_ASM_COMPILER_FORCED TRUE)

# Project name
project(stm32_opcua_example C ASM)

# Set C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# STM32 specific settings - adjust for your MCU
set(MCU_FAMILY STM32F4xx)
set(MCU_MODEL STM32F407xx)

# Paths to your STM32 development environment
# Adjust these paths according to your setup
set(STM32_HAL_PATH "${CMAKE_CURRENT_SOURCE_DIR}/STM32Cube_FW_F4_V1.27.1/Drivers/STM32F4xx_HAL_Driver")
set(CMSIS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/STM32Cube_FW_F4_V1.27.1/Drivers/CMSIS")
set(FREERTOS_PATH "${CMAKE_CURRENT_SOURCE_DIR}/FreeRTOS")
set(LWIP_PATH "${CMAKE_CURRENT_SOURCE_DIR}/lwip")

# Set architecture for open62541
set(UA_ARCHITECTURE "stm32" CACHE STRING "Architecture" FORCE)

# Configure open62541 for embedded systems
set(UA_ENABLE_AMALGAMATION ON CACHE BOOL "Enable amalgamation" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
set(UA_ENABLE_DISCOVERY OFF CACHE BOOL "Disable discovery" FORCE)
set(UA_ENABLE_SUBSCRIPTIONS_EVENTS OFF CACHE BOOL "Disable events" FORCE)
set(UA_ENABLE_HISTORIZING OFF CACHE BOOL "Disable historizing" FORCE)
set(UA_LOGLEVEL 400 CACHE STRING "Log level" FORCE)

# Include open62541
add_subdirectory(../.. open62541)

# STM32 specific compiler flags
set(CMAKE_C_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fdata-sections -ffunction-sections -Wall -fstack-usage")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${MCU_MODEL} -DUSE_HAL_DRIVER")

# Debug/Release flags
set(CMAKE_C_FLAGS_DEBUG "-g3 -O0")
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG")

# Linker flags
set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -specs=nano.specs -specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage")

# Include directories
include_directories(
    ${STM32_HAL_PATH}/Inc
    ${CMSIS_PATH}/Include
    ${CMSIS_PATH}/Device/ST/${MCU_FAMILY}/Include
    ${FREERTOS_PATH}/include
    ${FREERTOS_PATH}/portable/GCC/ARM_CM4F
    ${LWIP_PATH}/src/include
    ${LWIP_PATH}/system
    ${CMAKE_CURRENT_SOURCE_DIR}/Inc
)

# STM32 HAL sources (add the ones you need)
file(GLOB STM32_HAL_SOURCES
    "${STM32_HAL_PATH}/Src/stm32f4xx_hal.c"
    "${STM32_HAL_PATH}/Src/stm32f4xx_hal_rcc.c"
    "${STM32_HAL_PATH}/Src/stm32f4xx_hal_gpio.c"
    "${STM32_HAL_PATH}/Src/stm32f4xx_hal_uart.c"
    "${STM32_HAL_PATH}/Src/stm32f4xx_hal_eth.c"
    # Add other HAL sources as needed
)

# FreeRTOS sources
file(GLOB FREERTOS_SOURCES
    "${FREERTOS_PATH}/*.c"
    "${FREERTOS_PATH}/portable/GCC/ARM_CM4F/*.c"
    "${FREERTOS_PATH}/portable/MemMang/heap_4.c"
)

# lwIP sources (simplified - you may need more)
file(GLOB LWIP_SOURCES
    "${LWIP_PATH}/src/core/*.c"
    "${LWIP_PATH}/src/core/ipv4/*.c"
    "${LWIP_PATH}/src/api/*.c"
    "${LWIP_PATH}/src/netif/*.c"
    "${LWIP_PATH}/system/*.c"
)

# Application sources
set(APP_SOURCES
    stm32_server_example.c
    # Add your other application sources here
    # main.c
    # syscalls.c
    # system_stm32f4xx.c
    # etc.
)

# Create executable
add_executable(${PROJECT_NAME}.elf
    ${APP_SOURCES}
    ${STM32_HAL_SOURCES}
    ${FREERTOS_SOURCES}
    ${LWIP_SOURCES}
)

# Link libraries
target_link_libraries(${PROJECT_NAME}.elf
    open62541
)

# Create hex and bin files
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Building ${PROJECT_NAME}.hex and ${PROJECT_NAME}.bin"
)

# Print size information
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Size information:"
)
