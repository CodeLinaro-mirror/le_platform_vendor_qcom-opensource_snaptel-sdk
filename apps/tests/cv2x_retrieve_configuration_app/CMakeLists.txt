cmake_minimum_required(VERSION 2.8.9)

set(TARGET_CV2X_RETRIEVAL_CONFIG_APP cv2x_retrieve_configuration)

set(CV2X_RETRIEVAL_CONFIG_SOURCES Cv2xRetrieveConfigurationApp.cpp)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_CV2X_RETRIEVAL_CONFIG_APP} ${CV2X_RETRIEVAL_CONFIG_SOURCES})
target_link_libraries(${TARGET_CV2X_RETRIEVAL_CONFIG_APP} telux_cv2x)

# install to target
install ( TARGETS ${TARGET_CV2X_RETRIEVAL_CONFIG_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
