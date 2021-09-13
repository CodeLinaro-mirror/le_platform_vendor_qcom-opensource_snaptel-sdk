cmake_minimum_required(VERSION 2.8.9)

set(TARGET_THERMAL_APP thermal_app)

pkg_check_modules(TELUXTHERM REQUIRED telux-therm)

add_compile_options(${TELUXTHERM_CFLAGS})

set(SOURCES
    ThermalSampleApp.cpp
)

add_executable (${TARGET_THERMAL_APP} ${SOURCES})
target_compile_options(${TARGET_THERMAL_APP} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_THERMAL_APP} ${TELUXTHERM_LIBRARIES})

# install to target
install ( TARGETS ${TARGET_THERMAL_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
         )
