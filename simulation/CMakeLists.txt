cmake_minimum_required(VERSION 2.8.9)

project (telematics-apps-stubs)

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)

add_subdirectory( common )
add_subdirectory( loc )
add_subdirectory( data )
