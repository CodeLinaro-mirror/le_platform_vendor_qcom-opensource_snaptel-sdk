cmake_minimum_required(VERSION 2.8.9)

set(TARGET_ECC_VERIFY_ASYNC_POLL ecc_verify_async_poll)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXSEC REQUIRED telux-sec)

set(SOURCES ECCVerifyAsyncPoll.cpp)

add_executable(${TARGET_ECC_VERIFY_ASYNC_POLL} ${SOURCES})

target_compile_options(${TARGET_ECC_VERIFY_ASYNC_POLL} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_ECC_VERIFY_ASYNC_POLL} ${LIB_TELUXSEC_LIBRARIES})

install(TARGETS ${TARGET_ECC_VERIFY_ASYNC_POLL}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
