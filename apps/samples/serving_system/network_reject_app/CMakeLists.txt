cmake_minimum_required(VERSION 3.10.2)

set(TARGET_NETWORK_REJECT_APP network_reject_app)

set(SOURCES
    NetworkRejectApp.cpp
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_NETWORK_REJECT_APP} ${SOURCES})
target_link_libraries(${TARGET_NETWORK_REJECT_APP} telux_tel)

install ( TARGETS ${TARGET_NETWORK_REJECT_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
