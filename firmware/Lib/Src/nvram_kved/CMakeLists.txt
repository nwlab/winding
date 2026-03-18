target_sources(rdnx_stm32 INTERFACE
    rdnx_nvram.c
    kved/kved.c
    kved/port/stm32f4/port_flash.c
)

target_include_directories(rdnx_stm32 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kved
    ${CMAKE_CURRENT_SOURCE_DIR}/kved/port/stm32f4
)