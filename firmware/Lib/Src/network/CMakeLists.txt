target_sources(yaa_stm32 INTERFACE
               yaa_socket.c
               ethernet/w5500.c
               ethernet/w5500_socket.c
               ethernet/w5500_socket_backend.c
)

target_link_libraries(yaa_stm32 INTERFACE
    Network
)

if(YAA_CONFIG_TFTP)
    add_subdirectory(proto/tftp)
endif(YAA_CONFIG_TFTP)
