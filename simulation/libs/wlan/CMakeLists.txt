cmake_minimum_required(VERSION 3.10.2)

set(TARGET_WLAN_LIBRARY telux_wlan)

configure_file("telux-wlan.pc.in" "telux-wlan.pc" @ONLY)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)

set(WLAN_STUB_SOURCES
    WlanFactoryStub.cpp
    WlanDeviceManagerStub.cpp
    ApInterfaceManagerStub.cpp
    StaInterfaceManagerStub.cpp
)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

include_directories(BEFORE
    ${CMAKE_CURRENT_BINARY_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library (${TARGET_WLAN_LIBRARY} SHARED ${SOURCE} ${WLAN_STUB_SOURCES})

target_link_libraries(${TARGET_WLAN_LIBRARY}
    telux_protos
    telux_common
    )

install ( TARGETS ${TARGET_WLAN_LIBRARY}
    LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR} )

install ( FILES "${CMAKE_CURRENT_BINARY_DIR}/telux-wlan.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig/" )

