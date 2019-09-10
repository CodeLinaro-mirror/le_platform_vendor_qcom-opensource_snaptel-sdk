cmake_minimum_required(VERSION 2.8.9)

set(TARGET_CV2X_TX_SERVICE_STATUS_APP cv2x_tx_service_status_app)

set(CV2X_TX_SERVICE_STATUS_SOURCES
    Cv2xTxServiceStatusApp.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g")

add_executable (${TARGET_CV2X_TX_SERVICE_STATUS_APP} ${CV2X_TX_SERVICE_STATUS_SOURCES})
target_link_libraries(${TARGET_CV2X_TX_SERVICE_STATUS_APP} telux_cv2x)

# install to target
install ( TARGETS ${TARGET_CV2X_TX_SERVICE_STATUS_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
