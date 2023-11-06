cmake_minimum_required(VERSION 2.8.9)

set(TARGET_PRNG rng_trng)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXSEC REQUIRED telux-sec)

set(SOURCES GenerateRNTrng.cpp)

add_executable(${TARGET_PRNG} ${SOURCES})

target_compile_options(${TARGET_PRNG} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_PRNG} ${LIB_TELUXSEC_LIBRARIES})

install(TARGETS ${TARGET_PRNG}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
