cmake_minimum_required(VERSION 2.8.9)

project (telsdk-simulation)

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)

link_directories(${CMAKE_INSTALL_PREFIX}/lib)

add_definitions(-DDEFAULT_SIM_CONFIG_FILE_NAME="tel.conf")
add_definitions(-DDEFAULT_SIM_FILE_PREFIX="${CMAKE_INSTALL_PREFIX}")
add_definitions(-DDEFAULT_SIM_CONFIG_FILE_PATH="/etc/telux/")
add_definitions(-DDEFAULT_SIM_CSV_FILE_PATH="/data/telux/csv/")
add_definitions(-DDEFAULT_JSON_FILE_PATH="/data/telux/json/")

add_subdirectory( protos )
add_subdirectory( utils/event-injector )
add_subdirectory( services/sdk-simulation-server )
add_subdirectory( libs )
