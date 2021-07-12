# CMakeList.txt : CMake project for RadioInterface, include source and define
# project specific logic here.

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

set(TARGET_QITS qits)

set(QITS_SOURCES ApplicationTest.cpp)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O0 -pthread")

add_executable (${TARGET_QITS} ${QITS_SOURCES})
target_link_libraries(${TARGET_QITS} telux_cv2x qapplication qmessenger v2xcodec)

# install to target
install ( TARGETS ${TARGET_QITS}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )

