cmake_minimum_required(VERSION 2.8.9)

set(TARGET_DIAG_APP diag_test_app)

set(SOURCES
    DiagTestApp.cpp
    DiagFileMethodMenu.cpp
    DiagCallbackMethodMenu.cpp
    DiagUtils.cpp
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ../../common/console_app_framework/ConsoleAppCommand.cpp
    ../../common/console_app_framework/ConsoleApp.cpp
)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/console_app_framework
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror  -g")

add_executable (${TARGET_DIAG_APP} ${SOURCES})
target_link_libraries(${TARGET_DIAG_APP} telux_platform_diag telux_common)

# install to target
install ( TARGETS ${TARGET_DIAG_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
