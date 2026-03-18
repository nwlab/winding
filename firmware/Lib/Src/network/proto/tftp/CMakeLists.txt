target_sources(rdnx_stm32 INTERFACE
               rdnx_tftp.c
)

target_link_libraries(rdnx_stm32 INTERFACE
    Network
)
