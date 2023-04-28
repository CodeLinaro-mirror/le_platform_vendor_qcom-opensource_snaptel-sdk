cmake_minimum_required(VERSION 2.8.9)

set(TARGET_CA_LOAD_LISTENER ca_load_cap_listener)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXSEC REQUIRED telux-sec)

set(SOURCES CALoadCapacityListener.cpp)

add_executable(${TARGET_CA_LOAD_LISTENER} ${SOURCES})

target_compile_options(${TARGET_CA_LOAD_LISTENER} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_CA_LOAD_LISTENER} ${LIB_TELUXSEC_LIBRARIES})

install(TARGETS ${TARGET_CA_LOAD_LISTENER}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
