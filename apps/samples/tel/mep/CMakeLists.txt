cmake_minimum_required(VERSION 3.10.2)

set(TARGET_MEP_APP mep_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXTEL REQUIRED telux-tel)

set(MEP_APP_SOURCES MepApp.cpp)

add_executable(${TARGET_MEP_APP} ${MEP_APP_SOURCES})

target_include_directories(${TARGET_MEP_APP} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_MEP_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_MEP_APP} PRIVATE ${TELUXTEL_LIBRARIES})

install(TARGETS ${TARGET_MEP_APP} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
