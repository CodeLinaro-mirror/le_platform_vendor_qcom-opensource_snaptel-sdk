cmake_minimum_required(VERSION 2.8.9)

set(TARGET_AES_SYM_ENC_DEC aes_sym_enc_dec)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXSEC REQUIRED telux-sec)

set(SOURCES
    AESsymEncDecApp.cpp
)

add_executable(${TARGET_AES_SYM_ENC_DEC} ${SOURCES})
target_compile_options(${TARGET_AES_SYM_ENC_DEC} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_AES_SYM_ENC_DEC} ${LIB_TELUXSEC_LIBRARIES})

# install to target
install( TARGETS ${TARGET_AES_SYM_ENC_DEC}
         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
