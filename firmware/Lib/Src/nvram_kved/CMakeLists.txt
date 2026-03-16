target_sources(yaa_stm32 INTERFACE
    yaa_nvram.c
    kved/kved.c
    kved/port/stm32f4/port_flash.c
)

target_include_directories(yaa_stm32 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/kved
    ${CMAKE_CURRENT_SOURCE_DIR}/kved/port/stm32f4
)