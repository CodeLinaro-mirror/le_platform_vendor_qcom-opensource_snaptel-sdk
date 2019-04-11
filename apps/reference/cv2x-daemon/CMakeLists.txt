project(cv2x-daemon)

cmake_minimum_required(VERSION 2.8.9)

include(GNUInstallDirs)
include(FindPkgConfig)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

pkg_check_modules(TELUX REQUIRED telux)

if(WITH_SYSTEMD)
    add_definitions(-DWITH_SYSTEMD)
    set(SYSTEMD_LIB systemd)
endif()

set(TARGET_CV2X_DAEMON_APP cv2x-control)

set(SOURCES
    src/Cv2xDaemon.cpp
    src/Cv2xTelux.cpp
    src/Cv2xLog.cpp
    src/Cv2xUtils.cpp
)

# Daemon app
add_executable(${TARGET_CV2X_DAEMON_APP} ${SOURCES})
target_compile_options(${TARGET_CV2X_DAEMON_APP} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_CV2X_DAEMON_APP} telux_data telux_cv2x ${SYSTEMD_LIB} ${GLIB_LIBRARIES})

#add_compile_options(${TELUX_CFLAGS} -Werror)
#target_link_libraries(${TARGET_CV2X_DAEMON_APP} ${TELUX_LIBRARIES})

set(EXEC_PERM OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ)

# Installation
install(TARGETS ${TARGET_CV2X_DAEMON_APP}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

