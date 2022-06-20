cmake_minimum_required(VERSION 2.8.9)

set(TARGET_EC_ASYM_SG_VR ec_asym_sign_vrfy_app)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUXSEC REQUIRED telux-sec)

set(SOURCES
    ECAsymKeySignVerifyApp.cpp
)

add_executable(${TARGET_EC_ASYM_SG_VR} ${SOURCES})
target_compile_options(${TARGET_EC_ASYM_SG_VR} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_EC_ASYM_SG_VR} ${LIB_TELUXSEC_LIBRARIES})

# install to target
install( TARGETS ${TARGET_EC_ASYM_SG_VR}
         RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
