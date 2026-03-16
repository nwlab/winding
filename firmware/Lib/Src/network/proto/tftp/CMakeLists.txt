target_sources(yaa_stm32 INTERFACE
               yaa_tftp.c
)

target_link_libraries(yaa_stm32 INTERFACE
    Network
)
