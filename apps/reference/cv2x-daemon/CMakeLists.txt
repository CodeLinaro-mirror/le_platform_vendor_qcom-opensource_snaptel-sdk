project(cv2x-daemon)

cmake_minimum_required(VERSION 2.8.9)

include(GNUInstallDirs)
include(FindPkgConfig)

macro(SYSR_INCLUDE_DIR subdir)
     set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

pkg_check_modules(TELUX REQUIRED telux)

SYSR_INCLUDE_DIR(telux/power)

if(WITH_SYSTEMD)
    add_definitions(-DWITH_SYSTEMD)
    set(SYSTEMD_LIB systemd)
endif()

set(TARGET_CV2X_DAEMON_APP cv2x-daemon)

set(SOURCES
    src/Cv2xDaemon.cpp
    src/Cv2xTelux.cpp
    src/Cv2xLog.cpp
    src/Cv2xUtils.cpp
)

# Daemon app
add_executable(${TARGET_CV2X_DAEMON_APP} ${SOURCES})
target_compile_options(${TARGET_CV2X_DAEMON_APP} PRIVATE -std=c++11 -Wall -Werror)
target_link_libraries(${TARGET_CV2X_DAEMON_APP} pthread telux_data telux_cv2x telux_power ${SYSTEMD_LIB} ${GLIB_LIBRARIES})

set(EXEC_PERM OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ)

# Installation
install(TARGETS ${TARGET_CV2X_DAEMON_APP}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WITH_SYSTEMD)
    install(FILES scripts/cv2x.service scripts/cv2x.target
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../lib/systemd/system)
else()
    install(FILES scripts/start_cv2x_le
            DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d
            PERMISSIONS ${EXEC_PERM})
endif()

