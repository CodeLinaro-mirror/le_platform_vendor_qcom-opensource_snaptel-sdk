cmake_minimum_required(VERSION 2.8.9)

set(TARGET_RCV_CELL_SCAN_REPORT cellular_consec_report)

include(FindPkgConfig)
pkg_check_modules(LIB_TELUX_SECCON REQUIRED telux-sec-con)

set(SOURCES CellularConnectionReportListener.cpp)

add_executable(${TARGET_RCV_CELL_SCAN_REPORT} ${SOURCES})

target_compile_options(${TARGET_RCV_CELL_SCAN_REPORT} PRIVATE -std=c++11 -Wall -Werror)

target_link_libraries(${TARGET_RCV_CELL_SCAN_REPORT} ${LIB_TELUX_SECCON_LIBRARIES})

install(TARGETS ${TARGET_RCV_CELL_SCAN_REPORT}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
