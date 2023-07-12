cmake_minimum_required(VERSION 2.8.9)

project (telsdk-simulation)

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_definitions(-DDEFAULT_STUB_CONFIG_FILE_NAME="simulation.conf")
add_definitions(-DDEFAULT_STUB_CONFIG_FILE_PATH="${CMAKE_INSTALL_PREFIX}/etc/telux/conf/")
add_definitions(-DDEFAULT_JSON_FILE_PATH="${CMAKE_INSTALL_PREFIX}/etc/telux/json/")

add_subdirectory( utils/event-injector )
add_subdirectory( services/sdk-simulation-server )
add_subdirectory( libs )
