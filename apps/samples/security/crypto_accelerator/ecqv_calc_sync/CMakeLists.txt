cmake_minimum_required(VERSION 2.8.9)

set(TARGET_ECQV_CALC_SYNC ecqv_calc_sync)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXCRYPT_ACCEL REQUIRED telux-crypt-accel)

set(SOURCES ECQVCalcSync.cpp)

add_executable(${TARGET_ECQV_CALC_SYNC} ${SOURCES})

target_compile_options(${TARGET_ECQV_CALC_SYNC} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_ECQV_CALC_SYNC} ${LIB_TELUXCRYPT_ACCEL_LIBRARIES})

install(TARGETS ${TARGET_ECQV_CALC_SYNC}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
