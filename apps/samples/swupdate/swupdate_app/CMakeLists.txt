cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SWUPDATE_APP swupdate_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXSWUPDATE REQUIRED telux-swupdate)

set(SWUPDATE_APP_SOURCES SwUpdateApp.cpp)

add_executable(${TARGET_SWUPDATE_APP} ${SWUPDATE_APP_SOURCES})

target_include_directories(${TARGET_SWUPDATE_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_SWUPDATE_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_SWUPDATE_APP} PRIVATE telux_swupdate)

install(TARGETS ${TARGET_SWUPDATE_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
