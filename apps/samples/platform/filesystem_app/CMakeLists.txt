cmake_minimum_required(VERSION 2.8.9)

set(TARGET_FILESYSTEM_APP filesystem_app)

pkg_check_modules(TELUXPLATFORM REQUIRED telux-platform)

add_compile_options(${TELUXPLATFORM_CFLAGS})

set(SOURCES
    EfsRestoreSampleApp.cpp
    ../../../common/utils/Utils.cpp
)

add_executable (${TARGET_FILESYSTEM_APP} ${SOURCES})
target_compile_options(${TARGET_FILESYSTEM_APP} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_FILESYSTEM_APP} ${TELUXPLATFORM_LIBRARIES})

# install to target
install ( TARGETS ${TARGET_FILESYSTEM_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
