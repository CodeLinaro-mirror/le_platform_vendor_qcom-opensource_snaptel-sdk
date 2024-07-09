cmake_minimum_required(VERSION 3.10.2)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( thermal_app )
    add_subdirectory( thermal_shutdown )
endif(NOT BUILD_FOR_SIMULATION)
