cmake_minimum_required(VERSION 2.8.9)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( receive_sms_app )
    add_subdirectory( send_sms_app )
endif(NOT BUILD_FOR_SIMULATION)
