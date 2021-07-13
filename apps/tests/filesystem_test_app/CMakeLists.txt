cmake_minimum_required(VERSION 2.8.9)

set(TARGET_FILESYSTEM_TEST_APP filesystem_test_app)

set(FILESYSTEM_TEST_APP
    FileSystemTestApp.cpp
    FileSystemListener.cpp
    FileSystemCommandMgr.cpp
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ../../common/console_app_framework/ConsoleAppCommand.cpp
    ../../common/console_app_framework/ConsoleApp.cpp
)

add_definitions(-DAPP_NAME=\"\\033[1;32mFilesystem_test_app\\033[0m\")

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/console_app_framework
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/utils
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_FILESYSTEM_TEST_APP} ${FILESYSTEM_TEST_APP})
target_link_libraries(${TARGET_FILESYSTEM_TEST_APP} telux_platform)

# install to target
install ( TARGETS ${TARGET_FILESYSTEM_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
