cmake_minimum_required(VERSION 2.8.9)

set(TARGET_RSA_ASYM_SG_VR rsa_asym_sign_vrfy_app)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXCRYPT REQUIRED telux-crypt)

set(SOURCES RSAAsymKeySignVerifyApp.cpp)

add_executable(${TARGET_RSA_ASYM_SG_VR} ${SOURCES})

target_compile_options(${TARGET_RSA_ASYM_SG_VR} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_RSA_ASYM_SG_VR} ${LIB_TELUXCRYPT_LIBRARIES})

install(TARGETS ${TARGET_RSA_ASYM_SG_VR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
