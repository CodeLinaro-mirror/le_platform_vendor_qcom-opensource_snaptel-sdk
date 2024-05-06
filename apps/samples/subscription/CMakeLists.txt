cmake_minimum_required(VERSION 3.10.2)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( subscription_app )
endif(NOT BUILD_FOR_SIMULATION)
