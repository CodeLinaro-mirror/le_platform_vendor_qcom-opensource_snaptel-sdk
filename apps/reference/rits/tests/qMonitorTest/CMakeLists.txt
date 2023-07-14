# CMakeList.txt : CMake project for qMonitorTest, include source and define
# project specific logic here.

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

pkg_check_modules(JSONC_LIB REQUIRED json-c)
if (JSONC_LIB_FOUND)
    include_directories(${JSONC_LIB_INCLUDE_DIRS})
endif (JSONC_LIB_FOUND)

set(TARGET_QMONITOR qMonitorTest)



set(QMONITOR_SOURCES
    qMonitorTest.cpp
)


# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -O3 -pthread")

add_executable (${TARGET_QMONITOR} ${QMONITOR_SOURCES})
target_link_libraries(${TARGET_QMONITOR} qapplication json-c)


# install to target
install ( TARGETS ${TARGET_QMONITOR}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})

