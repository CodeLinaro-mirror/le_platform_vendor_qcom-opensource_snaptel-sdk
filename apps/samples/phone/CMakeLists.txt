cmake_minimum_required(VERSION 3.10.2)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( make_call_app )
    add_subdirectory( make_ecall_app )
    add_subdirectory( network_scan_app )
    add_subdirectory( make_tps_ecall_overims_app )
endif(NOT BUILD_FOR_SIMULATION)
