cmake_minimum_required(VERSION 2.8.9)

set(TARGET_NETWORK_SCAN_APP network_scan_app)

set(NETWORK_SCAN_SOURCES
    NetworkApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_NETWORK_SCAN_APP} ${NETWORK_SCAN_SOURCES})
target_link_libraries(${TARGET_NETWORK_SCAN_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_NETWORK_SCAN_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      )
