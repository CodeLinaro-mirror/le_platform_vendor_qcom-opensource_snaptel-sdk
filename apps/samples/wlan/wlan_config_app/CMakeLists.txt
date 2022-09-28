cmake_minimum_required(VERSION 2.8.9)

set(TARGET_WLAN_APP wlan_config_app)

set(SOURCES
    WlanConfigApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_WLAN_APP} ${SOURCES})
target_link_libraries(${TARGET_WLAN_APP} telux_wlan)

# install to target
install ( TARGETS ${TARGET_WLAN_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
