cmake_minimum_required(VERSION 2.8.9)

set(TARGET_CARD_APP card_services_app)

set(CARD_APP_SOURCES
    CardServicesApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_CARD_APP} ${CARD_APP_SOURCES})
target_link_libraries(${TARGET_CARD_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_CARD_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
