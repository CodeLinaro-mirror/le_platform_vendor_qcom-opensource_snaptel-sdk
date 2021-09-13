cmake_minimum_required(VERSION 2.8.9)

set(TARGET_ECALL_APP make_ecall_app)

set(ECALL_APP_SOURCES
    MakeECall.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror")

add_executable (${TARGET_ECALL_APP} ${ECALL_APP_SOURCES})
target_link_libraries(${TARGET_ECALL_APP} telux_tel)

# install to target
install ( TARGETS ${TARGET_ECALL_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
