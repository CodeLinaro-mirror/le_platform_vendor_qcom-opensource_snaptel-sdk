cmake_minimum_required(VERSION 2.8.9)

set(TARGET_MODEM_CONFIG_APP modem_config_app)

set(MODEM_CONFIG_APP_SOURCES
    ModemConfigApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_MODEM_CONFIG_APP} ${MODEM_CONFIG_APP_SOURCES})
target_include_directories(${TARGET_MODEM_CONFIG_APP} BEFORE PUBLIC
                           "${telematics-apps_SOURCE_DIR}/common" )
target_link_libraries(${TARGET_MODEM_CONFIG_APP} telux_config)

# install to target
install ( TARGETS ${TARGET_MODEM_CONFIG_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
