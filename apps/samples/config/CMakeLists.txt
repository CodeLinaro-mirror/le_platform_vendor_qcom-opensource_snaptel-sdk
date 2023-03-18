cmake_minimum_required(VERSION 2.8.9)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( modem_config_app )
    add_subdirectory( param_config_app )
endif(NOT BUILD_FOR_SIMULATION)
