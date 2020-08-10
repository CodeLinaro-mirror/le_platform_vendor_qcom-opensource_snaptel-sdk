cmake_minimum_required(VERSION 2.8.9)

set(TARGET_RSP_APP rsp_app)

set(TARGET_RSP_APP_SOURCES
   RspApp.cpp
   RspListener.cpp
   ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common/utils
)

# Set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

# Remote Sim provisioning http daemon
add_executable (${TARGET_RSP_APP} ${TARGET_RSP_APP_SOURCES})
target_link_libraries(${TARGET_RSP_APP} telux_rsp telux_common)

# install to target
install ( TARGETS ${TARGET_RSP_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
