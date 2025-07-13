# STM32 CMake Toolchain File
# This file configures CMake for cross-compilation to STM32 microcontrollers

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR arm)

# Toolchain settings
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_AR arm-none-eabi-ar)
set(CMAKE_RANLIB arm-none-eabi-ranlib)

# Prevent CMake from testing the compiler (cross-compilation)
set(CMAKE_C_COMPILER_FORCED TRUE)
set(CMAKE_CXX_COMPILER_FORCED TRUE)
set(CMAKE_ASM_COMPILER_FORCED TRUE)

# Search for programs in the build host directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)

# Search for libraries and headers in the target directories
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# STM32 Family-specific settings
if(NOT DEFINED STM32_FAMILY)
    set(STM32_FAMILY "STM32F4xx" CACHE STRING "STM32 Family")
endif()

# CPU and FPU settings based on STM32 family
if(STM32_FAMILY STREQUAL "STM32F0xx")
    set(MCU_FLAGS "-mcpu=cortex-m0 -mthumb")
    set(MCU_DEFINES "STM32F0xx")
elseif(STM32_FAMILY STREQUAL "STM32F1xx")
    set(MCU_FLAGS "-mcpu=cortex-m3 -mthumb")
    set(MCU_DEFINES "STM32F1xx")
elseif(STM32_FAMILY STREQUAL "STM32F3xx")
    set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
    set(MCU_DEFINES "STM32F3xx")
elseif(STM32_FAMILY STREQUAL "STM32F4xx")
    set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
    set(MCU_DEFINES "STM32F4xx")
elseif(STM32_FAMILY STREQUAL "STM32F7xx")
    set(MCU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-sp-d16 -mfloat-abi=hard")
    set(MCU_DEFINES "STM32F7xx")
elseif(STM32_FAMILY STREQUAL "STM32H7xx")
    set(MCU_FLAGS "-mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard")
    set(MCU_DEFINES "STM32H7xx")
elseif(STM32_FAMILY STREQUAL "STM32L0xx")
    set(MCU_FLAGS "-mcpu=cortex-m0plus -mthumb")
    set(MCU_DEFINES "STM32L0xx")
elseif(STM32_FAMILY STREQUAL "STM32L4xx")
    set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
    set(MCU_DEFINES "STM32L4xx")
elseif(STM32_FAMILY STREQUAL "STM32G4xx")
    set(MCU_FLAGS "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
    set(MCU_DEFINES "STM32G4xx")
else()
    message(FATAL_ERROR "Unsupported STM32 family: ${STM32_FAMILY}")
endif()

# Common compiler flags
set(COMMON_FLAGS "${MCU_FLAGS} -fdata-sections -ffunction-sections -Wall -fstack-usage")
set(COMMON_FLAGS "${COMMON_FLAGS} -D${MCU_DEFINES} -DUSE_HAL_DRIVER -DUA_ARCHITECTURE_STM32")

# C-specific flags
set(CMAKE_C_FLAGS "${COMMON_FLAGS}" CACHE STRING "C Compiler Flags" FORCE)
set(CMAKE_C_FLAGS_DEBUG "-g3 -O0 -DDEBUG" CACHE STRING "C Debug Flags" FORCE)
set(CMAKE_C_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "C Release Flags" FORCE)
set(CMAKE_C_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "C MinSizeRel Flags" FORCE)
set(CMAKE_C_FLAGS_RELWITHDEBINFO "-Os -g -DNDEBUG" CACHE STRING "C RelWithDebInfo Flags" FORCE)

# C++ flags (if needed)
set(CMAKE_CXX_FLAGS "${COMMON_FLAGS} -fno-exceptions -fno-rtti" CACHE STRING "CXX Compiler Flags" FORCE)
set(CMAKE_CXX_FLAGS_DEBUG "-g3 -O0 -DDEBUG" CACHE STRING "CXX Debug Flags" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-Os -DNDEBUG" CACHE STRING "CXX Release Flags" FORCE)
set(CMAKE_CXX_FLAGS_MINSIZEREL "-Os -DNDEBUG" CACHE STRING "CXX MinSizeRel Flags" FORCE)
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "-Os -g -DNDEBUG" CACHE STRING "CXX RelWithDebInfo Flags" FORCE)

# Linker flags
set(LINKER_FLAGS "${MCU_FLAGS} -specs=nano.specs -specs=nosys.specs")
set(LINKER_FLAGS "${LINKER_FLAGS} -Wl,--gc-sections -Wl,--print-memory-usage")

set(CMAKE_EXE_LINKER_FLAGS "${LINKER_FLAGS}" CACHE STRING "Linker Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "" CACHE STRING "Debug Linker Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "" CACHE STRING "Release Linker Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_MINSIZEREL "" CACHE STRING "MinSizeRel Linker Flags" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO "" CACHE STRING "RelWithDebInfo Linker Flags" FORCE)

# Set the C standard
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Disable shared libraries
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

message(STATUS "STM32 Toolchain configured for ${STM32_FAMILY}")
message(STATUS "MCU Flags: ${MCU_FLAGS}")
message(STATUS "Compiler: ${CMAKE_C_COMPILER}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
