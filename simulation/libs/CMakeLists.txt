cmake_minimum_required(VERSION 3.5.1)

find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

set(PROTO_SRC_DIR ${CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES}/proto-src)
set(GRPCPP_EXECUTABLE gRPC::grpc++)

FILE(GLOB SOURCE "${PROTO_SRC_DIR}/*.cc")

add_subdirectory( common )
add_subdirectory( data )
add_subdirectory( loc )
add_subdirectory( tel )
