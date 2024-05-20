cmake_minimum_required(VERSION 2.8.9)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( make_call_app )
    add_subdirectory( make_ecall_app )
    add_subdirectory( make_ecall_with_pdu_app )
    add_subdirectory( network_scan_app )
endif(NOT BUILD_FOR_SIMULATION)
