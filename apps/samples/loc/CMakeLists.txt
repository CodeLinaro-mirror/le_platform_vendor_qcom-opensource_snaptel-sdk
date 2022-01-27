cmake_minimum_required(VERSION 2.8.9)

set(GCC_EXPECTED_VERSION 9.3)

if(BUILD_FOR_SIMULATION)
    #Location library can be built with GCC version higher than 9.3
    if(${CMAKE_CXX_COMPILER_VERSION} GREATER ${GCC_EXPECTED_VERSION})
        add_subdirectory( loc_app )
    endif()
else()
    add_subdirectory( loc_app )
endif()
