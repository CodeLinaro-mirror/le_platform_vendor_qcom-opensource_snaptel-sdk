set(CMAKE_VERBOSE_MAKEFILE ON)
file(GLOB LIBQCODER_SOURCES ./*/*.cpp)

#add_library(qmessenger SHARED ${LIBQCODER_SOURCES})
add_library(qmessenger ${LIBQCODER_SOURCES})

target_link_libraries(qmessenger ${GLIB_LIBRARIES} telux_cv2x telux_loc v2x_veh
    v2xcodec)

install(TARGETS qmessenger
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
