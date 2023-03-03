cmake_minimum_required(VERSION 2.8.9)

set(TARGET_RSA_EXPORT_APP rsa_export_app)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXCRYPT REQUIRED telux-crypt)

set(SOURCES RSAExportKeyApp.cpp)

add_executable(${TARGET_RSA_EXPORT_APP} ${SOURCES})

target_compile_options(${TARGET_RSA_EXPORT_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_RSA_EXPORT_APP} ${LIB_TELUXCRYPT_LIBRARIES})

install(TARGETS ${TARGET_RSA_EXPORT_APP}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
