cmake_minimum_required(VERSION 3.16)
project(StepperHAL CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # disable GNU extensions

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -pedantic -Wswitch-enum -Wcast-qual -Woverflow")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -g0")

# Build the static library (shared if you want)
add_library(StepperHAL
    src/StepperHAL_STM32F4x1.cpp
)

target_compile_definitions(StepperHAL PRIVATE
	USE_HAL_DRIVER
	STM32F407xx
    $<$<CONFIG:Debug>:DEBUG>
)

# Include path
target_include_directories(StepperHAL PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_include_directories(StepperHAL PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Core/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Drivers/STM32F4xx_HAL_Driver/Inc
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Drivers/CMSIS/Device/ST/STM32F4xx/Include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../../Drivers/CMSIS/Include
)
