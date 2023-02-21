cmake_minimum_required(VERSION 2.8.9)

set(TARGET_HMAC_SYM_SG_VR hmac_sym_sign_vrfy_app)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXCRYPT REQUIRED telux-crypt)

set(SOURCES HMACSymKeySignVerifyApp.cpp)

add_executable(${TARGET_HMAC_SYM_SG_VR} ${SOURCES})

target_compile_options(${TARGET_HMAC_SYM_SG_VR} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_HMAC_SYM_SG_VR} ${LIB_TELUXCRYPT_LIBRARIES})

install(TARGETS ${TARGET_HMAC_SYM_SG_VR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
