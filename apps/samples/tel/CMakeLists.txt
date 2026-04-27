cmake_minimum_required(VERSION 2.8.9)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( call )
endif(NOT BUILD_FOR_SIMULATION)

add_subdirectory( card_services )
add_subdirectory( mep )
add_subdirectory( phone )
add_subdirectory( serving_system )
add_subdirectory( sms )
add_subdirectory( subscription )
add_subdirectory( rsp )
