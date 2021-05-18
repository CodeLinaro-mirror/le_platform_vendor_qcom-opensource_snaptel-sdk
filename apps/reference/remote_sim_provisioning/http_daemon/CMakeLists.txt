cmake_minimum_required(VERSION 2.8.9)

set(TARGET_HTTP_DAEMON rsp_httpd)

set(TARGET_HTTP_DAEMON_SOURCES
    Main.cpp
    Daemon.cpp
    HttpTransactionListener.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

# Remote Sim provisioning http daemon
add_executable (${TARGET_HTTP_DAEMON} ${TARGET_HTTP_DAEMON_SOURCES})
target_link_libraries(${TARGET_HTTP_DAEMON} telux_tel telux_common curl pthread)

# install to target
install ( TARGETS ${TARGET_HTTP_DAEMON}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
