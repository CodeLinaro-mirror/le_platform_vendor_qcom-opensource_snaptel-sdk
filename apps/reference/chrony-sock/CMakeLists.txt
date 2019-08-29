include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR})

set(APP_NAME chrony-sock)

pkg_check_modules(TELUXCOMMON REQUIRED telux-common)
pkg_checK_modules(TELUXLOC REQUIRED telux-loc)

# Glib for strlcpy and strlcat
pkg_check_modules(GLIB REQUIRED glib-2.0)
add_compile_options(${GLIB_CFLAGS})

include_directories(BEFORE ${CMAKE_CURRENT_SOURCE_DIR}/inc)

set(EXEC_PERM OWNER_EXECUTE OWNER_WRITE OWNER_READ
              GROUP_EXECUTE GROUP_READ
              WORLD_EXECUTE WORLD_READ)

set(SOURCES chrony-sock.cpp)

add_executable(${APP_NAME} ${SOURCES})
target_compile_options(${APP_NAME} PRIVATE ${TELUXCOMMON_CFLAGS}
                       ${TELUXLOC_CFLAGS} -std=c++11 -Wall -Werror)
target_link_libraries(${APP_NAME} rt ${GLIB_LIBRARIES}
                      ${TELUXCOMMON_LIBRARIES} ${TELUXLOC_LIBRARIES})

install(TARGETS ${APP_NAME}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

if(WITH_SYSTEMD)
    install(FILES chrony-sock.service
            DESTINATION ${CMAKE_INSTALL_PREFIX}/../lib/systemd/system)
else()
    install(FILES start_chrony_sock_le
            DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}/init.d
            PERMISSIONS ${EXEC_PERM})
endif()
