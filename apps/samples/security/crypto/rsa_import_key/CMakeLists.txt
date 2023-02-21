cmake_minimum_required(VERSION 2.8.9)

set(TARGET_RSA_IMPORT_KEY rsa_import_key_app)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXCRYPT REQUIRED telux-crypt)

set(SOURCES RSAImportKeyApp.cpp)

add_executable(${TARGET_RSA_IMPORT_KEY} ${SOURCES})

target_compile_options(${TARGET_RSA_IMPORT_KEY} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_RSA_IMPORT_KEY} ${LIB_TELUXCRYPT_LIBRARIES})

install(TARGETS ${TARGET_RSA_IMPORT_KEY}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
