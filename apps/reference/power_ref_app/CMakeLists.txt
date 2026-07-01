cmake_minimum_required(VERSION 3.10.2)


set(TARGET_POWER_REF_DAEMON telux_power_refd)

find_package(PkgConfig REQUIRED)

pkg_check_modules(CAN_WRAPPER_LIB libcanwrapper)
if (CAN_WRAPPER_LIB_FOUND)
    add_definitions(-DCAN_TRIGGER_SUPPORTED)
endif (CAN_WRAPPER_LIB_FOUND)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common
    ${telematics-apps_SOURCE_DIR}/common/utils)

if (TELUX_QTI_EXTERNAL_AP OR TELUX_FOR_EXTERNAL_AP OR TELSDK_FEATURE_FOR_SECONDARY_VM)
    set(TELUX_POWER_REFD_EAP TRUE)
    add_definitions(-DTELUX_POWER_REFD_EAP)
endif (TELUX_QTI_EXTERNAL_AP OR TELUX_FOR_EXTERNAL_AP OR TELSDK_FEATURE_FOR_SECONDARY_VM)

set(TARGET_POWER_REF_DAEMON_SOURCES
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    common/ConfigParser.cpp
    common/RefAppUtils.cpp
    Event.cpp
    TCPKeepAliveHandler.cpp
    WakeupHandler.cpp
    trigger/GPIO/GPIOTrigger.cpp
    trigger/NAOIP/filter/DataConfigParser.cpp
    trigger/NAOIP/filter/DataFilterController.cpp
    trigger/NAOIP/NAOIpTrigger.cpp
    trigger/SMS/SMSTrigger.cpp
    common/socket/ConnectionHandler.cpp
    common/socket/TCPClient.cpp
    common/socket/TCPServer.cpp
    common/socket/UDPClient.cpp
    common/socket/UDPServer.cpp
    PowerRefDaemonMain.cpp
    slave/PowerRefDaemonSlave.cpp
    slave/TcuActivityMonitor.cpp
    trigger/NAOIP/filter/DataConfigParser.cpp
    trigger/NAOIP/filter/DataFilterController.cpp
)

if (NOT TELUX_POWER_REFD_EAP)
    list(APPEND TARGET_POWER_REF_DAEMON_SOURCES
        EventManager.cpp
        PowerRefDaemon.cpp
        trigger/GPIO/GPIOTrigger.cpp
        trigger/NAOIP/NAOIpTrigger.cpp
        trigger/SMS/SMSTrigger.cpp
        ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
        ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
    )
endif (NOT TELUX_POWER_REFD_EAP)

if (CAN_WRAPPER_LIB_FOUND AND NOT TELUX_POWER_REFD_EAP)
    list(APPEND TARGET_POWER_REF_DAEMON_SOURCES trigger/CAN/CANTrigger.cpp )
endif (CAN_WRAPPER_LIB_FOUND)

if (TELSDK_FEATURE_SATCOM)
    list(APPEND TARGET_POWER_REF_DAEMON_SOURCES NtnClient.cpp)
endif (TELSDK_FEATURE_SATCOM)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils
    ${CMAKE_CURRENT_SOURCE_DIR}/common/socket
    ${CMAKE_CURRENT_SOURCE_DIR}/common/
    ${CMAKE_CURRENT_SOURCE_DIR}/trigger/NAOIP/filter/
)

# Set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_POWER_REF_DAEMON} ${TARGET_POWER_REF_DAEMON_SOURCES})
target_link_libraries(${TARGET_POWER_REF_DAEMON} telux_common telux_tel telux_power
    telux_data pthread rt)

if (TELSDK_FEATURE_SATCOM)
    target_link_libraries(${TARGET_POWER_REF_DAEMON} telux_satcom)
endif (TELSDK_FEATURE_SATCOM)

if (TELSDK_FEATURE_LOC)
    target_link_libraries(${TARGET_POWER_REF_DAEMON} telux_loc)
endif (TELSDK_FEATURE_LOC)

if (CAN_WRAPPER_LIB_FOUND)
    target_link_libraries(${TARGET_POWER_REF_DAEMON} canwrapper)
endif (CAN_WRAPPER_LIB_FOUND)

# install to target
install ( TARGETS ${TARGET_POWER_REF_DAEMON}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
if (TELUX_POWER_REFD_EAP)
    install ( FILES config_files/slave/telux_power_refd.conf
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
else ()
    install ( FILES config_files/telux_power_refd.conf
        DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
endif (TELUX_POWER_REFD_EAP)
