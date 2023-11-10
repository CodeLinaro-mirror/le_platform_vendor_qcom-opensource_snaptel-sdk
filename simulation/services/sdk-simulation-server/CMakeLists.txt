cmake_minimum_required(VERSION 2.8.9)

set(TARGET_SIMULATION_SERVER_APP telsdk_simulation_server)

include(FindPkgConfig)

find_library(JSONCPP_LIBRARY NAMES jsoncpp)

set(protobuf_MODULE_COMPATIBLE TRUE)
find_package(Protobuf CONFIG REQUIRED)
find_package(gRPC CONFIG REQUIRED)

pkg_check_modules(PROTOBUF_LIB protobuf)
pkg_check_modules(GRPC_LIB grpc)
pkg_check_modules(GRPCPP_LIB grpc++)

set(GRPCPP_EXECUTABLE gRPC::grpc++)

set(TARGET_SIMULATION_SERVER_APP_SRC
    SimulationServer.cpp
    data/DataConnectionServerImpl.cpp
    data/DataProfileServerImpl.cpp
    tel/CardManagerServerImpl.cpp
    tel/SubscriptionManagerServerImpl.cpp
    tel/SmsManagerServerImpl.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -pthread")

add_executable (${TARGET_SIMULATION_SERVER_APP} ${TARGET_SIMULATION_SERVER_APP_SRC})
target_link_libraries(${TARGET_SIMULATION_SERVER_APP}
    telux_common
    telux_data
    telux_protos
    ${PROTOBUF_LIB_LIBRARIES}
    ${GRPC_LIB_LIBRARIES}
    ${GRPCPP_LIB_LIBRARIES}
    )

# install to target
install ( TARGETS ${TARGET_SIMULATION_SERVER_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
