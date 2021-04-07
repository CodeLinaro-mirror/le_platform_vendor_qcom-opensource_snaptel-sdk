cmake_minimum_required(VERSION 2.8.9)

set(TARGET_DATA_APP dmz_sample_app)

set(SOURCES
    DataDmzApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_DATA_APP} ${SOURCES})
target_link_libraries(${TARGET_DATA_APP} telux_data)

# install to target
install ( TARGETS ${TARGET_DATA_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
