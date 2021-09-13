cmake_minimum_required(VERSION 2.8.9)

set(TARGET_CALL_APP make_call_app)

set(CALL_APP_SOURCES
    MakeCall.cpp
    ${telematics-apps_SOURCE_DIR}/common/ConfigParser.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_CALL_APP} ${CALL_APP_SOURCES})
target_include_directories(${TARGET_CALL_APP} BEFORE PUBLIC
                           "${telematics-apps_SOURCE_DIR}/common" )
target_link_libraries(${TARGET_CALL_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_CALL_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
