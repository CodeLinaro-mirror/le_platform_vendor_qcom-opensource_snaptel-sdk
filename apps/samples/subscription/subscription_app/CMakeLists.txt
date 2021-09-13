cmake_minimum_required(VERSION 2.8.9)

set(TARGET_SUBSCRIPTION_APP subscription_app)

set(SOURCES
    SubscriptionApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_SUBSCRIPTION_APP} ${SOURCES})
target_link_libraries(${TARGET_SUBSCRIPTION_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_SUBSCRIPTION_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
