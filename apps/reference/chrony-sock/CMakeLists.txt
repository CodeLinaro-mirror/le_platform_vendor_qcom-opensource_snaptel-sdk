cmake_minimum_required(VERSION 3.10.2)

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

set(APP_NAME chrony-sock)

find_package(PkgConfig REQUIRED)

pkg_check_modules(TELUXCOMMON REQUIRED telux-common)
pkg_check_modules(TELUXPLATFORM REQUIRED telux-platform)

if(TELSDK_FEATURE_POWER AND TELSDK_FEATURE_TEL)
    add_definitions(-DFEATURE_NETWORK_TIME_ENABLED)
endif()

# Glib for strlcpy and strlcat
pkg_check_modules(GLIB REQUIRED glib-2.0)
add_compile_options(${GLIB_CFLAGS})


set(EXEC_PERM OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ)

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/../../common/ConfigParser.cpp
    ${CMAKE_CURRENT_LIST_DIR}/../../common/utils/SignalHandler.cpp
    MyUtils.cpp
    ChronySock.cpp)

add_executable(${APP_NAME} ${SOURCES})
target_compile_options(${APP_NAME} PRIVATE ${TELUXCOMMON_CFLAGS}
                       ${TELUXPLATFORM_CFLAGS} -std=c++11 -Wall -Werror)
target_link_libraries(${APP_NAME} rt ${GLIB_LIBRARIES}
                      ${TELUXCOMMON_LIBRARIES} ${TELUXPLATFORM_LIBRARIES} pthread)

if(TELSDK_FEATURE_POWER AND TELSDK_FEATURE_TEL)
    target_link_libraries(${APP_NAME} telux_power telux_tel)
endif()

install(TARGETS ${APP_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WITH_SYSTEMD)
    install(FILES chrony-sock.service
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../usr/lib/systemd/system)
else()
    install(FILES start_chrony_sock_le
            DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d
            PERMISSIONS ${EXEC_PERM})
endif()
