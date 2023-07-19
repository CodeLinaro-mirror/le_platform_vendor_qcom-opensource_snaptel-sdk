cmake_minimum_required(VERSION 2.8.9)

set(TARGET_CV2X_SLSS_UTC_APP cv2x_slss_utc_app)

set(CV2X_SLSS_UTC_SOURCES
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ${telematics-apps_SOURCE_DIR}/common/utils/SignalHandler.cpp
    Cv2xSlssUtcTest.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_compile_options(${GLIB_CFLAGS})

add_executable (${TARGET_CV2X_SLSS_UTC_APP} ${CV2X_SLSS_UTC_SOURCES})
target_link_libraries(${TARGET_CV2X_SLSS_UTC_APP} ${GLIB_LIBRARIES} telux_cv2x telux_platform pthread)

# install to target
install ( TARGETS ${TARGET_CV2X_SLSS_UTC_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
