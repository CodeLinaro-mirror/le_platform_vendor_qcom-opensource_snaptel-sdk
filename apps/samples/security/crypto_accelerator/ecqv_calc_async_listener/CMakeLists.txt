cmake_minimum_required(VERSION 2.8.9)

set(TARGET_ECQV_CALC_ASYNC_LISTENER ecqv_calc_async_listener)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXSEC REQUIRED telux-sec)

set(SOURCES ECQVCalcAsyncListener.cpp)

add_executable(${TARGET_ECQV_CALC_ASYNC_LISTENER} ${SOURCES})

target_compile_options(${TARGET_ECQV_CALC_ASYNC_LISTENER} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_ECQV_CALC_ASYNC_LISTENER} ${LIB_TELUXSEC_LIBRARIES})

install(TARGETS ${TARGET_ECQV_CALC_ASYNC_LISTENER}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
