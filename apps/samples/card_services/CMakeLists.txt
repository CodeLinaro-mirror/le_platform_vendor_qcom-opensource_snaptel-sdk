cmake_minimum_required(VERSION 3.10.2)

if(NOT BUILD_FOR_SIMULATION)
    add_subdirectory( card_services_app )
    add_subdirectory( simple_sap_api_app )
endif(NOT BUILD_FOR_SIMULATION)