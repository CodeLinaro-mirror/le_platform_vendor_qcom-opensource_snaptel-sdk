cmake_minimum_required(VERSION 2.8.9)

# pkg-config support
configure_file("telux-loc.pc.in" "telux-loc.pc" @ONLY)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

set(TARGET_LOCATION_LIBRARY telux_loc)

add_definitions(-DTELUX_TECH_AREA=4)

set(LOCATION_TEST_LIB_SOURCES
    LocationFactoryStub.cpp
    LocationManagerStub.cpp
    LocationConfiguratorStub.cpp
    DgnssManagerStub.cpp
    ReportHandler.cpp
    ReportReader.cpp
)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

add_library(${TARGET_LOCATION_LIBRARY} SHARED ${LOCATION_TEST_LIB_SOURCES})

target_link_libraries(${TARGET_LOCATION_LIBRARY}
    jsoncpp
    telux_common
)

install(TARGETS ${TARGET_LOCATION_LIBRARY}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-loc.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/")
