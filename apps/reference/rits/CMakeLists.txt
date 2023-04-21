cmake_minimum_required(VERSION 2.8.9)

project(qits)
include(GNUInstallDirs)
include(FindPkgConfig)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3")

if (WITH_AEROLINK)
    add_definitions( -DAEROLINK)
elseif(DEFINED ENV{AEROLINK_PATH})
    add_definitions( -DAEROLINK)
    add_definitions(-I$ENV{AEROLINK_PATH}/include/)
    message("Building with security")
endif()

# add "-DEMIT_ASN_DEBUG -DASN_THREAD_SAFE" to debug asn1 decoder
if(ASN1C_PATH)
    add_definitions(-DETSI -DWITH_WSA -D_DEFAULT_SOURCE)
    set(ASN1C_SKELETONS ${ASN1C_PATH}/skeletons)
elseif(DEFINED ENV{ASN1C_PATH})
    add_definitions(-DETSI -D_DEFAULT_SOURCE)
    set(ASN1C_SKELETONS $ENV{ASN1C_PATH}/skeletons)
endif()

find_path(GLIB_INCLUDE_DIR NAMES glibconfig.h glib.h PATH_SUFFIXES glib-2.0)

#Find sysroot path in environment CC
string(REGEX MATCH "=.*" GLIB64_PATH $ENV{CC})

include_directories(BEFORE
    ${PROJECT_SOURCE_DIR}/include
    #Including this libraries given a bug with glib
    ${GLIB_INCLUDE_DIR}
    ${GLIB64_PATH}/usr/lib64/glib-2.0/include/
    ${PROJECT_SOURCE_DIR}/src/qCoder/include
    ${PROJECT_SOURCE_DIR}/src/qMessenger/RadioInterface
    ${PROJECT_SOURCE_DIR}/src/qMessenger/RadioTransmit
    ${PROJECT_SOURCE_DIR}/src/qMessenger/RadioReceive
    ${PROJECT_SOURCE_DIR}/src/qMessenger/KinematicsReceive
    ${PROJECT_SOURCE_DIR}/src/qMessenger/VehicleReceive
    ${PROJECT_SOURCE_DIR}/src/qApplication/Application
    ${PROJECT_SOURCE_DIR}/src/qApplication/Ldm
    ${PROJECT_SOURCE_DIR}/src/qApplication/SafetyApps
    ${PROJECT_SOURCE_DIR}/src/qMessenger/ThrottleManager
)
if(DEFINED ENV{ASN1C_PATH} OR DEFINED ASN1C_PATH)
    include_directories(${ASN1C_SKELETONS}
    ${PROJECT_SOURCE_DIR}/src/qCoder/src/etsi/CAM/generated
    ${PROJECT_SOURCE_DIR}/src/qCoder/src/etsi/DENM/generated
    ${PROJECT_SOURCE_DIR}/src/qCoder/src/etsi/dictionary/generated
    ${PROJECT_SOURCE_DIR}/src/qCoder/src/sae/generated
    ${PROJECT_SOURCE_DIR}/src/qMessenger/GeoNetRouter)
endif()
add_subdirectory(src/qCoder/)
add_subdirectory(src/qMessenger/)
add_subdirectory(src/qApplication)
add_subdirectory(tests/applicationTest)

if(WITH_SYSTEMD)
    install(FILES scripts/qits@.service
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../lib/systemd/system)
endif()
