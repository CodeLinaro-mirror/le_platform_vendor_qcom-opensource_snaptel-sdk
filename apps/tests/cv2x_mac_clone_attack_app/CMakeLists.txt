cmake_minimum_required(VERSION 2.8.9)

set(TARGET_CV2X_MAC_CLONE_ATTACK_APP cv2x_mac_clone_attack_app)

set(CV2X_MAC_CLONE_ATTACK_SOURCES
    ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
    ${telematics-apps_SOURCE_DIR}/common/utils/SignalHandler.cpp
    Cv2xMacCloneAttackTest.cpp
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_compile_options(${GLIB_CFLAGS})

add_executable (${TARGET_CV2X_MAC_CLONE_ATTACK_APP} ${CV2X_MAC_CLONE_ATTACK_SOURCES})
target_link_libraries(${TARGET_CV2X_MAC_CLONE_ATTACK_APP} ${GLIB_LIBRARIES} telux_cv2x pthread)

# install to target
install ( TARGETS ${TARGET_CV2X_MAC_CLONE_ATTACK_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
