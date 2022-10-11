cmake_minimum_required(VERSION 2.8.9)


set(TARGET_POWER_REF_DAEMON telux_power_refd)

set(TARGET_POWER_REF_DAEMON_SOURCES
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    common/ConfigParser.cpp
    common/RefAppUtils.cpp
    Event.cpp
    EventManager.cpp
    PowerRefDaemon.cpp
    trigger/GPIO/GPIOTrigger.cpp
    trigger/NAOIP/filter/DataConfigParser.cpp
    trigger/NAOIP/filter/DataFilterController.cpp
    trigger/NAOIP/NAOIpTrigger.cpp
    trigger/SMS/SMSTrigger.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common/utils
)

# Set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_POWER_REF_DAEMON} ${TARGET_POWER_REF_DAEMON_SOURCES})
target_link_libraries(${TARGET_POWER_REF_DAEMON} telux_common telux_tel telux_power telux_connect
    telux_data curl pthread)

# install to target
install ( TARGETS ${TARGET_POWER_REF_DAEMON}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} )
install ( FILES config_files/telux_power_refd.conf
          DESTINATION ${CMAKE_INSTALL_SYSCONFDIR})

