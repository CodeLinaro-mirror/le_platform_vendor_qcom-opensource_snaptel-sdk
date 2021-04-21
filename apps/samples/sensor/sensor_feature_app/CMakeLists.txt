cmake_minimum_required(VERSION 2.8.9)

set(TARGET_SENSOR_FEATURE_APP sensor_feature_app)

pkg_check_modules(TELUXSENSOR REQUIRED telux-sensor)

add_compile_options(${TELUXSENSOR_CFLAGS})

set(SOURCES
    SensorFeatureSampleApp.cpp
)

add_executable (${TARGET_SENSOR_FEATURE_APP} ${SOURCES})
target_compile_options(${TARGET_SENSOR_FEATURE_APP} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_SENSOR_FEATURE_APP} ${TELUXSENSOR_LIBRARIES})

# install to target
install ( TARGETS ${TARGET_SENSOR_FEATURE_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
