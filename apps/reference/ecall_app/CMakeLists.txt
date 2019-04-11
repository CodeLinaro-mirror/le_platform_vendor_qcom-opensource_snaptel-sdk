cmake_minimum_required(VERSION 2.8.9)

set(TARGET_ECALL_APP ecall_app)

set(ECALL_APP_SOURCES
   ECallApp.cpp
   ${telematics-apps_SOURCE_DIR}/tests/telsdk_console_app/ECall/ECallMenu.cpp
   ${telematics-apps_SOURCE_DIR}/tests/telsdk_console_app/ECall/MsdSettings.cpp
   ${telematics-apps_SOURCE_DIR}/tests/telsdk_console_app/ECall/MyECallListener.cpp
   ${telematics-apps_SOURCE_DIR}/tests/telsdk_console_app/Utils.cpp
   ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
   ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common
    ${telematics-apps_SOURCE_DIR}/common/console_app_framework
    ${telematics-apps_SOURCE_DIR}/tests/telsdk_console_app/
    ${telematics-apps_SOURCE_DIR}/tests/telsdk_console_app/ECall
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_ECALL_APP} ${ECALL_APP_SOURCES})
target_link_libraries(${TARGET_ECALL_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_ECALL_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
