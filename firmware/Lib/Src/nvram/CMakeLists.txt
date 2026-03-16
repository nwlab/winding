target_sources(yaa_stm32 INTERFACE
    entry.c
    entry.h
    map.c
    map.h
    yaa_nvram.c
    storage.c
    storage.h
)

target_include_directories(yaa_stm32 INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}
)