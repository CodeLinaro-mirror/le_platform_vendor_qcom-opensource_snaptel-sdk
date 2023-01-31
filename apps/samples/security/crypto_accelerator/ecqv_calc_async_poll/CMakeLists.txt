cmake_minimum_required(VERSION 2.8.9)

set(TARGET_ECQV_CALC_ASYNC_POLL ecqv_calc_async_poll)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXCRYPT_ACCEL REQUIRED telux-crypt-accel)

set(SOURCES ECQVCalcAsyncPoll.cpp)

add_executable(${TARGET_ECQV_CALC_ASYNC_POLL} ${SOURCES})

target_compile_options(${TARGET_ECQV_CALC_ASYNC_POLL} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_ECQV_CALC_ASYNC_POLL} ${LIB_TELUXCRYPT_ACCEL_LIBRARIES})

install(TARGETS ${TARGET_ECQV_CALC_ASYNC_POLL}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
