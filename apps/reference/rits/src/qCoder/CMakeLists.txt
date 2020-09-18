cmake_minimum_required(VERSION 2.8)
set(CMAKE_VERBOSE_MAKEFILE ON)

# set the project name
project(v2xc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )

# find all sources
file(GLOB SOURCE_FILES_TOP ${CMAKE_CURRENT_SOURCE_DIR}/src/wsmp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/v2x_codec.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bsm_utils.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/btp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/j2735.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etsi.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ieee1609.2.c
    v2xc.c)
file(GLOB ASN1C_FILES
    ${ASN1C_SKELETONS}/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etsi/dictionary/generated/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etsi/CAM/generated/*.c
    ${CMAKE_CURRENT_SOURCE_DIR}/src/etsi/DENM/generated/*.c)

set (SOURCE_FILES ${SOURCE_FILES_TOP} ${ASN1C_FILES})

add_executable(v2xc v2xc.c)

add_library(v2xcodec ${SOURCE_FILES})
target_link_libraries(v2xc v2xcodec)

install(TARGETS v2xc
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	RUNTIME DESTINATION bin)
