cmake_minimum_required(VERSION 2.8.9)

set(TARGET_SIMULATION_SERVER_APP telsdk_simulation_server)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTOC_COMPILER $<TARGET_FILE:protobuf::protoc>)
set(GRPCPP_EXECUTABLE gRPC::grpc++)
set(GRPCPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
set (PROTOS "")

set(TARGET_SIMULATION_SERVER_APP_SRC
    SimulationServer.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)
file(MAKE_DIRECTORY ${PROTO_SRC_DIR})
include_directories(${PROTO_SRC_DIR})

file(GLOB files "${CMAKE_CURRENT_LIST_DIR}/../../../protos/*.proto")
foreach(file ${files})
    get_filename_component(filename ${file} NAME_WE)
    get_filename_component(file_path ${file} PATH)

    set(telsdk_message_source "${PROTO_SRC_DIR}/${filename}.pb.cc")
    set(telsdk_message_header "${PROTO_SRC_DIR}/${filename}.pb.h")
    set(telsdk_service_source "${PROTO_SRC_DIR}/${filename}.grpc.pb.cc")
    set(telsdk_service_header "${PROTO_SRC_DIR}/${filename}.grpc.pb.h")

    add_custom_command(
        OUTPUT "${telsdk_message_source}" "${telsdk_message_header}" "${telsdk_service_source}" "${telsdk_service_header}"
        COMMAND ${PROTOC_COMPILER}
        ARGS --grpc_out "${PROTO_SRC_DIR}"
          --cpp_out "${PROTO_SRC_DIR}"
          -I "${file_path}"
          --plugin=protoc-gen-grpc="${GRPCPP_PLUGIN_EXECUTABLE}"
          "${file}"
        DEPENDS "${file}")
    list(APPEND PROTOS
        ${telsdk_message_source}
        ${telsdk_message_header}
        ${telsdk_service_source}
        ${telsdk_service_header}
    )
endforeach()

add_library(telsdk_grpc_proto ${PROTOS})

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_executable (${TARGET_SIMULATION_SERVER_APP} ${TARGET_SIMULATION_SERVER_APP_SRC})
target_link_libraries(${TARGET_SIMULATION_SERVER_APP}
    telsdk_grpc_proto
    telux_common
    ${GRPCPP_EXECUTABLE}
    )

# install to target
install ( TARGETS ${TARGET_SIMULATION_SERVER_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )