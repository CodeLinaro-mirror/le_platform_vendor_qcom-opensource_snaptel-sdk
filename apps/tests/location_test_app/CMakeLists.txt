cmake_minimum_required(VERSION 2.8.9)

include(GNUInstallDirs)
set(TARGET_LOCATION_TEST_APP location_test_app)

set(APPS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

set(LOCATION_TEST_APP_SOURCES
    LocationMenu.cpp
    MyLocationListener.cpp
    MyLocationCommandCallback.cpp
    LocationUtils.cpp
    ${APPS_SOURCE_DIR}/common/utils/Utils.cpp
    ${APPS_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
    ${APPS_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
    ${APPS_SOURCE_DIR}/common/ConfigParser.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${APPS_SOURCE_DIR}/common
    ${APPS_SOURCE_DIR}/common/utils
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_LOCATION_TEST_APP} ${LOCATION_TEST_APP_SOURCES})
target_link_libraries(${TARGET_LOCATION_TEST_APP} telux_loc telux_common)

# install to target
install ( TARGETS ${TARGET_LOCATION_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )