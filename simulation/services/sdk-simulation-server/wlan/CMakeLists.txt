cmake_minimum_required(VERSION 3.10.2)

set(TARGET_SIMULATION_SERVER_APP_SRC
    wlan/WlanDeviceManagerServerImpl.cpp
    wlan/ApInterfaceManagerServerImpl.cpp
    wlan/StaInterfaceManagerServerImpl.cpp
    wlan/WlanServerUtils.cpp
    ../../libs/wlan/WlanCommonUtilsStub.cpp
)

target_sources (${TARGET_SIMULATION_SERVER_APP} PRIVATE ${TARGET_SIMULATION_SERVER_APP_SRC})
