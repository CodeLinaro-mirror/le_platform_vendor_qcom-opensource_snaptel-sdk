cmake_minimum_required(VERSION 2.8.9)

project(qits)
include(GNUInstallDirs)
include(FindPkgConfig)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0")

if(DEFINED ENV{SECURITY_LIB_PATH})
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSECURITY -I$ENV{SECURITY_LIB_PATH}/include")
endif()

if(NOT DEFINED ENV{ASN1C_PATH})
    message(FATAL_ERROR "No ASN1C_PATH environment specified, please check README.md")
else()
    set(ASN1C_SKELETONS $ENV{ASN1C_PATH}/skeletons)
endif()

find_path(GLIB_INCLUDE_DIR NAMES glibconfig.h glib.h PATH_SUFFIXES glib-2.0)

#Find sysroot path in environement CC
string(REGEX MATCH "=.*" GLIB64_PATH $ENV{CC})
#message( ${GLIB64_PATH_1})
#string(SUBSTRING ${GLIB64_PATH_1} 1 -1 GLIB64_PATH)
#message( ${GLIB64_PATH})

include_directories(BEFORE
    #${CMAKE_SOURCE_DIR}/include
    #Including this libraries given a bug with glib
    ${GLIB_INCLUDE_DIR}
    ${ASN1C_SKELETONS}
    ${GLIB64_PATH}/usr/lib64/glib-2.0/include/
    ${CMAKE_SOURCE_DIR}/src/qCoder/include
    ${CMAKE_SOURCE_DIR}/src/qCoder/src/etsi/CAM/generated
    ${CMAKE_SOURCE_DIR}/src/qCoder/src/etsi/DENM/generated
    ${CMAKE_SOURCE_DIR}/src/qCoder/src/etsi/dictionary/generated
    ${CMAKE_SOURCE_DIR}/src/qMessenger/RadioInterface
    ${CMAKE_SOURCE_DIR}/src/qMessenger/RadioTransmit
    ${CMAKE_SOURCE_DIR}/src/qMessenger/RadioReceive
    ${CMAKE_SOURCE_DIR}/src/qMessenger/KinematicsReceive
    ${CMAKE_SOURCE_DIR}/src/qMessenger/VehicleReceive
    ${CMAKE_SOURCE_DIR}/src/qMessenger/GeoNetRouter
    ${CMAKE_SOURCE_DIR}/src/qApplication/Application
    ${CMAKE_SOURCE_DIR}/src/qApplication/Ldm
    ${CMAKE_SOURCE_DIR}/src/qApplication/SafetyApps
)

add_subdirectory(src/qCoder/)
add_subdirectory(src/qMessenger/)
add_subdirectory(src/qApplication)
add_subdirectory(tests/applicationTest)
