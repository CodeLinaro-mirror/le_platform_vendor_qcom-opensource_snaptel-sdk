cmake_minimum_required(VERSION 2.8.9)

set(TARGET_PARAM_CONFIG_APP param_config_app)

set(PARAM_CONFIG_APP_SOURCES
    ParamConfigApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_PARAM_CONFIG_APP} ${PARAM_CONFIG_APP_SOURCES})
target_include_directories(${TARGET_PARAM_CONFIG_APP} BEFORE PUBLIC
                           "${telematics-apps_SOURCE_DIR}/common" )
target_link_libraries(${TARGET_PARAM_CONFIG_APP} telux_config telux_common)

# install to target
install ( TARGETS ${TARGET_PARAM_CONFIG_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )