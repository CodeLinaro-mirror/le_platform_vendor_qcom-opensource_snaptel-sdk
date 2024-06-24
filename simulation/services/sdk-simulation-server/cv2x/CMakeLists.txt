cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    cv2x/Cv2xManagerServerImpl.cpp
    cv2x/Cv2xConfigServerImpl.cpp
    cv2x/Cv2xRadioServer.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})
