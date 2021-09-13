cmake_minimum_required(VERSION 2.8.9)

set(TARGET_SMS_APP receive_sms_app)

set(SMS_APP_SOURCES
    SimpleSmsListener.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_SMS_APP} ${SMS_APP_SOURCES})
target_link_libraries(${TARGET_SMS_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_SMS_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
