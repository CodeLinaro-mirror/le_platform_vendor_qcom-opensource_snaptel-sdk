cmake_minimum_required(VERSION 2.8.9)

set(TARGET_ECALL_APP ecall_app)

set(ECALL_APP_SOURCES
    ECallApp.cpp
    ECallManager.cpp
    ThermClient.cpp
    AudioClient.cpp
    LocationClient.cpp
    TelClient.cpp
    TelClientUtils.cpp
    MsdProvider.cpp
    ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
    ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ${telematics-apps_SOURCE_DIR}/common/ConfigParser.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common/console_app_framework
    ${telematics-apps_SOURCE_DIR}/common/utils
    ${telematics-apps_SOURCE_DIR}/common
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_ECALL_APP} ${ECALL_APP_SOURCES})
target_link_libraries(${TARGET_ECALL_APP} telux_tel telux_loc telux_therm telux_audio pthread)

# install to target
install ( TARGETS ${TARGET_ECALL_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
