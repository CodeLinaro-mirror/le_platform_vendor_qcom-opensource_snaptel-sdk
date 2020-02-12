cmake_minimum_required(VERSION 2.8.9)

set(TARGET_THERMAL_TEST_APP thermal_test_app)

pkg_check_modules(TELUXTHERM REQUIRED telux-therm)
pkg_check_modules(TELUXCOMMON REQUIRED telux-common)

add_compile_options(${TELUXTHERM_CFLAGS} ${TELUXCOMMON_CFLAGS})

set(TELUX_LIBRARIES ${TELUXTHERM_LIBRARIES} ${TELUXCOMMON_LIBRARIES})

set(THERMAL_APP_SOURCES
    ThermalHelper.cpp
    ThermalTestApp.cpp
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
    ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common
)

add_executable (${TARGET_THERMAL_TEST_APP} ${THERMAL_APP_SOURCES})
target_compile_options(${TARGET_THERMAL_TEST_APP} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_THERMAL_TEST_APP} ${TELUX_LIBRARIES})

# install to target
install ( TARGETS ${TARGET_THERMAL_TEST_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
         )
