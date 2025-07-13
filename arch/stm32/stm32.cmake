# STM32 architecture support for open62541
# Copyright 2025

# Add STM32 architecture to the available architectures
if(UA_ARCHITECTURE_STM32)
    message(STATUS "Building open62541 for STM32 with FreeRTOS and lwIP")
    
    # STM32 requires FreeRTOS and lwIP
    set(UA_ARCHITECTURE_FREERTOS ON)
    set(UA_ARCHITECTURE_LWIP ON)
    
    # Define STM32 specific compiler flags
    add_compile_definitions(UA_ARCHITECTURE_STM32=1)
    add_compile_definitions(STM32_PORT=1)
    
    # STM32 HAL and FreeRTOS include directories
    # These should be set by the user's CMakeLists.txt before including this file
    if(NOT DEFINED STM32_HAL_PATH)
        message(WARNING "STM32_HAL_PATH not defined. Please set it to your STM32 HAL path")
    endif()
    
    if(NOT DEFINED FREERTOS_PATH)
        message(WARNING "FREERTOS_PATH not defined. Please set it to your FreeRTOS path")
    endif()
    
    if(NOT DEFINED LWIP_PATH)
        message(WARNING "LWIP_PATH not defined. Please set it to your lwIP path")
    endif()
    
    # Include directories (if paths are provided)
    if(DEFINED STM32_HAL_PATH)
        include_directories(${STM32_HAL_PATH}/Inc)
    endif()
    
    if(DEFINED FREERTOS_PATH)
        include_directories(${FREERTOS_PATH}/include)
        include_directories(${FREERTOS_PATH}/portable/GCC/ARM_CM4F)  # Adjust for your MCU
    endif()
    
    if(DEFINED LWIP_PATH)
        include_directories(${LWIP_PATH}/src/include)
        include_directories(${LWIP_PATH}/system)
    endif()
    
    # STM32 specific compile options
    add_compile_options(-mcpu=cortex-m4)  # Adjust for your MCU
    add_compile_options(-mthumb)
    add_compile_options(-mfpu=fpv4-sp-d16)
    add_compile_options(-mfloat-abi=hard)
    
    # Optimize for size on embedded systems
    add_compile_options(-Os)
    add_compile_options(-ffunction-sections)
    add_compile_options(-fdata-sections)
    
    # Add linker options
    add_link_options(-mcpu=cortex-m4)
    add_link_options(-mthumb)
    add_link_options(-mfpu=fpv4-sp-d16)
    add_link_options(-mfloat-abi=hard)
    add_link_options(-Wl,--gc-sections)
    add_link_options(-Wl,--print-memory-usage)
    
    # Define memory-constrained build options
    set(UA_LOGLEVEL 400 CACHE STRING "Set log level to WARNING to save memory" FORCE)
    set(UA_ENABLE_DISCOVERY OFF CACHE BOOL "Disable discovery to save memory" FORCE)
    set(UA_ENABLE_SUBSCRIPTIONS_EVENTS OFF CACHE BOOL "Disable events to save memory" FORCE)
    set(UA_ENABLE_HISTORIZING OFF CACHE BOOL "Disable historizing to save memory" FORCE)
    set(UA_ENABLE_EXPERIMENTAL_TYPEDESCRIPTION OFF CACHE BOOL "Disable experimental features" FORCE)
    
    # Reduce memory usage
    set(UA_MULTITHREADING 200 CACHE STRING "Use basic threading for STM32" FORCE)
    
    message(STATUS "STM32 build configuration applied")
    
endif()
