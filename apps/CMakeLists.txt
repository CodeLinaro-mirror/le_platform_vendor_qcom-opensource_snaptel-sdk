cmake_minimum_required(VERSION 2.8.9)

project (telematics-apps)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wnon-virtual-dtor ")
# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

if (MACHINE_HAS_CV2X_ONLY)
    add_definitions(-DFEATURE_CV2X_ONLY)
endif (MACHINE_HAS_CV2X_ONLY)

if (TELUX_FOR_EXTERNAL_AP)
    add_definitions(-DTELUX_FOR_EXTERNAL_AP)
    if (TELUX_QTI_EXTERNAL_AP)
        add_definitions(-DTELUX_QTI_EXTERNAL_AP)
    endif (TELUX_QTI_EXTERNAL_AP)
endif (TELUX_FOR_EXTERNAL_AP)

#For simulation apps, set rpath to CMAKE_INSTALL_LIBDIR for telux stub libraries
if(BUILD_FOR_SIMULATION)
    set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
endif(BUILD_FOR_SIMULATION)

# build subdirectories
add_subdirectory( common/console_app_framework )
add_subdirectory( reference )
add_subdirectory( tests )
add_subdirectory( samples)
