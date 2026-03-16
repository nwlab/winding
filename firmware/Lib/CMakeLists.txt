cmake_minimum_required(VERSION 3.22)

option(YAA_CONFIG_CANBUS "Enable CAN bus support" OFF)
option(YAA_CONFIG_NETWORK "Enable network support" OFF)
option(YAA_CONFIG_RTT "Enable SEGGER RTT support" OFF)
option(YAA_CONFIG_SENSOR "Enable sensor support" OFF)
option(YAA_CONFIG_TFTP "Enable TFTP protocol support" OFF)

project(yaa_stm32)
add_library(yaa_stm32 INTERFACE)

include("../cmake/target_arch.cmake")

# Set target architectures
target_architecture(CMAKE_TARGET_ARCHITECTURES)
list(LENGTH CMAKE_TARGET_ARCHITECTURES cmake_target_arch_len)
if(NOT "${cmake_target_arch_len}" STREQUAL "1")
    set(CMAKE_TARGET_ARCHITECTURE_UNIVERSAL TRUE)
    set(CMAKE_TARGET_ARCHITECTURE_CODE "universal")
else()
    set(CMAKE_TARGET_ARCHITECTURE_UNIVERSAL FALSE)
    set(CMAKE_TARGET_ARCHITECTURE_CODE "${CMAKE_TARGET_ARCHITECTURES}")
endif()

message(STATUS "YAA library target architectures: ${CMAKE_TARGET_ARCHITECTURES}")

# Enable CMake support for ASM and C languages
enable_language(C ASM)

target_compile_definitions(yaa_stm32 INTERFACE
    STM32F407xx
    $<$<CONFIG:Debug>:DEBUG>
    YAA_CONFIG_CANBUS=$<BOOL:${YAA_CONFIG_CANBUS}>
    YAA_CONFIG_NETWORK=$<BOOL:${YAA_CONFIG_NETWORK}>
    YAA_CONFIG_SENSOR=$<BOOL:${YAA_CONFIG_SENSOR}>
    YAA_CONFIG_TFTP=$<BOOL:${YAA_CONFIG_TFTP}>
)

target_include_directories(yaa_stm32 INTERFACE
    Inc
    Inc/drivers
    Inc/sensor
    ../Drivers/STM32F4xx_HAL_Driver/Inc
    ../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ../Middlewares/Third_Party/FreeRTOS/Source/include
    ../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS
    ../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F
    ../Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ../Drivers/CMSIS/Include
)

target_sources(yaa_stm32 INTERFACE
    Src/adc/ltc2946.c
    Src/adc/ade7758.c
    Src/hal/yaa_gpio.c
    Src/hal/yaa_i2c.c
    Src/hal/yaa_spi.c
    Src/onewire/ds2430.c
    Src/onewire/ds2482.c
    Src/yaa_bkpsram.c
    Src/yaa_clock.c
    Src/yaa_event.c
    Src/yaa_hal.c
    Src/yaa_mempool.c
    Src/yaa_platform.c
    Src/yaa_queue.c
    Src/yaa_ring.c
    Src/yaa_time.c
    Src/yaa_util.c
)

if(CMAKE_TARGET_ARCHITECTURE_CODE STREQUAL "armv7")
    target_sources(yaa_stm32 INTERFACE
        Src/yaa_bootloader.c
        Src/yaa_flash.c
        Src/yaa_hardfault.c
        Src/yaa_sal.c
        Src/yaa_wdg.c
    )
endif()

# Validate that STM32CubeMX code is compatible with C standard
if(CMAKE_C_STANDARD LESS 11)
    message(ERROR "Generated code requires C11 or higher")
endif()

if(YAA_CONFIG_CANBUS)
    target_sources(yaa_stm32 INTERFACE
        Src/hal/yaa_can.c
    )
endif(YAA_CONFIG_CANBUS)

if(YAA_CONFIG_NETWORK)
    add_subdirectory(Src/network)
endif(YAA_CONFIG_NETWORK)

if(YAA_CONFIG_RTT)
    add_subdirectory(Src/rtt)
endif(YAA_CONFIG_RTT)

if(YAA_CONFIG_SENSOR)
    add_subdirectory(Src/sensor)
endif(YAA_CONFIG_SENSOR)

add_subdirectory(Src/drivers)
add_subdirectory(Src/nvram_kved)
