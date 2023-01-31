cmake_minimum_required(VERSION 2.8.9)

set(TARGET_IMEI_TEST_APP imei_test_app)

pkg_check_modules(TELUXPLATFORM REQUIRED telux-platform)
add_compile_options(${TELUXPLATFORM_CFLAGS})
set(IMEI_TEST_APP_SOURCES
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ImeiTestApp.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_IMEI_TEST_APP} ${IMEI_TEST_APP_SOURCES})
target_link_libraries(${TARGET_IMEI_TEST_APP} telux_common ${TELUXPLATFORM_LIBRARIES})

# install to target
install ( TARGETS ${TARGET_IMEI_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
