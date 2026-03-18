cmake_minimum_required(VERSION 3.22)

option(RDNX_CONFIG_CANBUS "Enable RDNX CAN bus support" OFF)
option(RDNX_CONFIG_NETWORK "Enable RDNX network support" OFF)
option(RDNX_CONFIG_RTT "Enable SEGGER RTT support" OFF)
option(RDNX_CONFIG_SENSOR "Enable RDNX sensor support" OFF)
option(RDNX_CONFIG_TFTP "Enable TFTP protocol support" OFF)
option(RDNX_CONFIG_I2C "Enable I2C support" OFF)
option(RDNX_CONFIG_SAL "Enable SAL support" OFF)

project(rdnx_stm32)
add_library(rdnx_stm32 INTERFACE)

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

message(STATUS "RDNX library target architectures: ${CMAKE_TARGET_ARCHITECTURES}")

# Enable CMake support for ASM and C languages
enable_language(C ASM)

target_compile_definitions(rdnx_stm32 INTERFACE
    STM32F407xx
    $<$<CONFIG:Debug>:DEBUG>
    RDNX_CONFIG_CANBUS=$<BOOL:${RDNX_CONFIG_CANBUS}>
    RDNX_CONFIG_NETWORK=$<BOOL:${RDNX_CONFIG_NETWORK}>
    RDNX_CONFIG_SENSOR=$<BOOL:${RDNX_CONFIG_SENSOR}>
    RDNX_CONFIG_TFTP=$<BOOL:${RDNX_CONFIG_TFTP}>
    RDNX_CONFIG_SAL=$<BOOL:${RDNX_CONFIG_SAL}>
    RDNX_CONFIG_I2C=$<BOOL:${RDNX_CONFIG_I2C}>
)

target_include_directories(rdnx_stm32 INTERFACE
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

target_sources(rdnx_stm32 INTERFACE
    Src/adc/ade7758.c
    Src/adc/ltc2946.c
    Src/hal/rdnx_gpio.c
    Src/hal/rdnx_spi.c
    Src/hal/rdnx_timer_LL.c
    Src/rdnx_alloc.c
    Src/rdnx_bkpsram.c
    Src/rdnx_clock.c
    Src/rdnx_hal.c
    Src/rdnx_platform.c
    Src/rdnx_ring.c
    Src/rdnx_time.c
    Src/rdnx_util.c
)

if(CMAKE_TARGET_ARCHITECTURE_CODE STREQUAL "armv7")
    target_sources(rdnx_stm32 INTERFACE
        Src/rdnx_bootloader.c
        Src/rdnx_flash.c
        Src/rdnx_hardfault.c
        Src/rdnx_wdg.c
    )
endif()

# Validate that STM32CubeMX code is compatible with C standard
if(CMAKE_C_STANDARD LESS 11)
    message(ERROR "Generated code requires C11 or higher")
endif()

if(RDNX_CONFIG_SAL)
    target_sources(rdnx_stm32 INTERFACE
        Src/rdnx_mempool.c
        Src/rdnx_sal.c
        Src/rdnx_queue.c
        Src/rdnx_event.c
    )
endif(RDNX_CONFIG_SAL)

if(RDNX_CONFIG_I2C)
    target_sources(rdnx_stm32 INTERFACE
        Src/hal/rdnx_i2c.c
        Src/onewire/ds2430.c
        Src/onewire/ds2482.c
    )
endif(RDNX_CONFIG_I2C)

if(RDNX_CONFIG_CANBUS)
    target_sources(rdnx_stm32 INTERFACE
        Src/hal/rdnx_can.c
    )
endif(RDNX_CONFIG_CANBUS)

if(RDNX_CONFIG_NETWORK)
    add_subdirectory(Src/network)
endif(RDNX_CONFIG_NETWORK)

if(RDNX_CONFIG_RTT)
    add_subdirectory(Src/rtt)
endif(RDNX_CONFIG_RTT)

if(RDNX_CONFIG_SENSOR)
    add_subdirectory(Src/sensor)
endif(RDNX_CONFIG_SENSOR)

add_subdirectory(Src/drivers)
add_subdirectory(Src/nvram_kved)
