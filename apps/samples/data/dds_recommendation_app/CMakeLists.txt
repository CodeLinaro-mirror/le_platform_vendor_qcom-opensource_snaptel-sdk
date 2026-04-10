cmake_minimum_required(VERSION 3.10.2)

set(TARGET_DDS_RECOMMENDATION_APP dds_recommendation_app)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TELUXDATA REQUIRED telux-data)

set(DDS_RECOMMENDATION_APP_SOURCES DdsRecommendationApp.cpp)

add_executable(${TARGET_DDS_RECOMMENDATION_APP} ${DDS_RECOMMENDATION_APP_SOURCES})

target_include_directories(${TARGET_DDS_RECOMMENDATION_APP}
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_options(${TARGET_DDS_RECOMMENDATION_APP} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_DDS_RECOMMENDATION_APP} PRIVATE ${TELUXDATA_LIBRARIES})

# install to target
install(TARGETS ${TARGET_DDS_RECOMMENDATION_APP}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
