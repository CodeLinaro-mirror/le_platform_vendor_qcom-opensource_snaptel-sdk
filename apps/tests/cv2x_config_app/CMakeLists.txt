cmake_minimum_required(VERSION 2.8.9)

set(TARGET_CV2X_CONFIG_APP cv2x_config_app)

set(CV2X_CONFIG_SOURCES
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ../../common/console_app_framework/ConsoleAppCommand.cpp
    ../../common/console_app_framework/ConsoleApp.cpp
    Cv2xConfigApp.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../common/console_app_framework
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_CV2X_CONFIG_APP} ${CV2X_CONFIG_SOURCES})
target_link_libraries(${TARGET_CV2X_CONFIG_APP} telux_cv2x)

# install to target
install ( TARGETS ${TARGET_CV2X_CONFIG_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
