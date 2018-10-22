cmake_minimum_required(VERSION 2.8.9)

set(TARGET_NETWORK_SELECTION_APP network_selection_app)

set(NETWORK_SELECTION_SOURCES
    NetworkApp.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_NETWORK_SELECTION_APP} ${NETWORK_SELECTION_SOURCES})
target_link_libraries(${TARGET_NETWORK_SELECTION_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_NETWORK_SELECTION_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
      )
