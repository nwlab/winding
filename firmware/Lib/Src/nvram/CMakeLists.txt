target_sources(rdnx_stm32 INTERFACE
    entry.c
    entry.h
    map.c
    map.h
    rdnx_nvram.c
    storage.c
    storage.h
)

target_include_directories(rdnx_stm32 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)