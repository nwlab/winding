target_sources(rdnx_stm32 INTERFACE
               rdnx_socket.c
               ethernet/w5500.c
               ethernet/w5500_socket.c
               ethernet/w5500_socket_backend.c
)

target_link_libraries(rdnx_stm32 INTERFACE
    Network
)

if(RDNX_CONFIG_TFTP)
    add_subdirectory(proto/tftp)
endif(RDNX_CONFIG_TFTP)
