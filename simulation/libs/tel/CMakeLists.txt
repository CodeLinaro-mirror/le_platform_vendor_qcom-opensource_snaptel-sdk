cmake_minimum_required(VERSION 2.8.9)

set(TARGET_TEL_LIBRARY telux_tel)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)

set(TEL_STUB_SOURCES
    PhoneFactoryImplStub.cpp
    CardManagerStub.cpp
    CardStub.cpp
    CardAppStub.cpp
    CardFileHandlerStub.cpp
)

FILE(GLOB SOURCE "${PROTO_SRC_DIR}/*.cc")

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_library (${TARGET_TEL_LIBRARY} SHARED ${SOURCE} ${TEL_STUB_SOURCES})

target_link_libraries(${TARGET_TEL_LIBRARY}
    ${_GRPC_GRPCPP}
    jsoncpp
    telux_protos
    telux_common
    )

install ( TARGETS ${TARGET_TEL_LIBRARY}
    LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR} )
