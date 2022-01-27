cmake_minimum_required(VERSION 2.8.9)

set(GCC_EXPECTED_VERSION 9.3)
#Location library can be built with GCC version higher than 9.3
if(${CMAKE_CXX_COMPILER_VERSION} GREATER ${GCC_EXPECTED_VERSION})
    set(TARGET_LOCATION_LIBRARY telux_loc)

    set(LOCATION_TEST_LIB_SOURCES
        LocationFactoryStub.cpp
        LocationManagerStub.cpp
        LocationConfiguratorStub.cpp
        DgnssManagerStub.cpp
        StubHelper.cpp
        StubSystemStarter.cpp
        ReportHandler.cpp
        ReportReader.cpp
        ConfigParserStub.cpp
        Logger/Logger.cpp
    )
    macro(SYSR_INCLUDE_DIR subdir)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
    endmacro()

    # add these sub-folders from /usr/include/<subdir>
    SYSR_INCLUDE_DIR(telux)

    include_directories(BEFORE
        ${CMAKE_CURRENT_SOURCE_DIR}
    )

    # set global variables
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -pthread")

    add_library (${TARGET_LOCATION_LIBRARY} SHARED ${LOCATION_TEST_LIB_SOURCES})

        install ( TARGETS ${TARGET_LOCATION_LIBRARY}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
endif()






