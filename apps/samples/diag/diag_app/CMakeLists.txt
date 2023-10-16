cmake_minimum_required(VERSION 2.8.9)

set(TARGET_DIAG_APP diag_app)

set(SOURCES
    DiagApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_DIAG_APP} ${SOURCES})
target_link_libraries(${TARGET_DIAG_APP} telux_platform_diag)

# install to target
install ( TARGETS ${TARGET_DIAG_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
