cmake_minimum_required(VERSION 2.8.9)

set(TARGET_LOC_APP loc_app)

set(LOC_APP_SOURCES
    SampleLocationApp.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_LOC_APP} ${LOC_APP_SOURCES})
target_link_libraries(${TARGET_LOC_APP} telux_loc)

# install to target
install ( TARGETS ${TARGET_LOC_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
