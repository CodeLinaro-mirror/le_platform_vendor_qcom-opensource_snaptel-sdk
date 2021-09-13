cmake_minimum_required(VERSION 2.8.9)

set(TARGET_SAP_APP simple_sap_api_app)

set(SAP_APP_SOURCES
    SapApiListener.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_SAP_APP} ${SAP_APP_SOURCES})
target_link_libraries(${TARGET_SAP_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_SAP_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
