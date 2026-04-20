cmake_minimum_required(VERSION 3.10.2)

project (telematics-apps)

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

set(ENV{PKG_CONFIG_PATH} "${CMAKE_INSTALL_PREFIX}/lib/pkgconfig/:$ENV{PKG_CONFIG_PATH}")

pkg_check_modules(TELUX_SATCOM_LIB telux-satcom)

if (TELUX_SATCOM_LIB_FOUND)
    set (TELSDK_FEATURE_SATCOM ON)
    add_definitions(-DTELSDK_FEATURE_SATCOM_ENABLED)
endif (TELUX_SATCOM_LIB_FOUND)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

#To enable debug symbols, set -DENABLE_DEBUG=ON
if(ENABLE_DEBUG)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
endif()

# build subdirectories
add_subdirectory( common/console_app_framework )
add_subdirectory( tests )
add_subdirectory( samples)
