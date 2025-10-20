cmake_minimum_required(VERSION 2.8.9)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( make_aecs_call_app )
endif(NOT BUILD_FOR_SIMULATION)
