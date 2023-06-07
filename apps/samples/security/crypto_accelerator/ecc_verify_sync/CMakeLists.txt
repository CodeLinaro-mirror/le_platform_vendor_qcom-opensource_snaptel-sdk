cmake_minimum_required(VERSION 2.8.9)

set(TARGET_ECC_VERIFY_SYNC ecc_verify_sync)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXSEC REQUIRED telux-sec)

set(SOURCES ECCVerifySync.cpp)

add_executable(${TARGET_ECC_VERIFY_SYNC} ${SOURCES})

target_compile_options(${TARGET_ECC_VERIFY_SYNC} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_ECC_VERIFY_SYNC} ${LIB_TELUXSEC_LIBRARIES})

install(TARGETS ${TARGET_ECC_VERIFY_SYNC}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
