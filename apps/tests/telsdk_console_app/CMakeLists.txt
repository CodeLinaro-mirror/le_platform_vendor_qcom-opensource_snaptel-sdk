cmake_minimum_required(VERSION 2.8.9)

set(TARGET_TELSDK_CONSOLE_APP telsdk_console_app)

set(TELSDK_CONSOLE_APP_SOURCES
   Phone/PhoneMenu.cpp
   Phone/MyPhoneListener.cpp
   Phone/MySubscriptionListener.cpp
   Phone/MySignalStrengthHandler.cpp
   Phone/MyCellInfoHandler.cpp

   Call/CallMenu.cpp
   Call/MyCallListener.cpp

   SimCardServices/SimCardServicesMenu.cpp
   SimCardServices/Card/CardServicesMenu.cpp
   SimCardServices/Card/MyCardListener.cpp
   SimCardServices/SapCard/SapCardServicesMenu.cpp
   SimCardServices/SapCard/MySapCardListener.cpp

   Sms/SmsMenu.cpp
   Sms/MySmsListener.cpp

   ECall/ECallMenu.cpp
   ECall/MsdSettings.cpp
   ECall/MyECallListener.cpp

   Data/DataMenu.cpp
   Data/DataResponseCallback.cpp
   Data/DataListener.cpp
   Data/MyProfileListener.cpp
   Data/DataUtils.cpp
   Data/data_profile/DataProfileMenu.cpp
   Data/data_connection/DataConnectionMenu.cpp
   Data/data_filter/MyDataFilterListener.cpp
   Data/data_filter/DataFilterMenu.cpp
   Data/data_filter/DataConfigParser.cpp
   Data/bridge/BridgeMenu.cpp
   Data/l2tp/L2tpMenu.cpp
   Data/snat/SnatMenu.cpp
   Data/firewall/FirewallMenu.cpp
   Data/vlan/VlanMenu.cpp
   Data/socks/SocksMenu.cpp

   Phone/MyServingSystemHandler.cpp
   Phone/ServingSystemMenu.cpp

   Phone/MyNetworkSelectionHandler.cpp
   Phone/NetworkMenu.cpp

   MultiSim/MyMultiSimHandler.cpp
   MultiSim/MyMultiSimListener.cpp
   MultiSim/MultiSimMenu.cpp

   Rsp/RspListener.cpp
   Rsp/MyRspHandler.cpp
   Rsp/RspMenu.cpp

   TelSdkConsoleApp.cpp

   ModemStatus.cpp

   Audio/AudioClient.cpp
   Cellbroadcast/CellbroadcastMenu.cpp
   Cellbroadcast/CellbroadcastListener.cpp
   Cellbroadcast/CellbroadcastHandler.cpp

   ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
   ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
   ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
   ${telematics-apps_SOURCE_DIR}/common/ConfigParser.cpp
)

# To disable audio from telsdk_console_app please disable flag AUDIO_ENABLED
if (AUDIO_ENABLED)
    add_definitions(-DAUDIO_SUPPORTED)
    set(TELSDK_CONSOLE_APP_SOURCES ${TELSDK_CONSOLE_APP_SOURCES}
    ${telematics-apps_SOURCE_DIR}/common/Audio/VoiceSession.cpp
    ${telematics-apps_SOURCE_DIR}/common/Audio/AudioSession.cpp
    ${telematics-apps_SOURCE_DIR}/common/Audio/AudioHelper.cpp
    )
endif (AUDIO_ENABLED)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_TELSDK_CONSOLE_APP} ${TELSDK_CONSOLE_APP_SOURCES})

if (AUDIO_ENABLED)
target_link_libraries(${TARGET_TELSDK_CONSOLE_APP} telux_tel telux_data telux_common telux_audio)
else (!AUDIO_ENABLED)
target_link_libraries(${TARGET_TELSDK_CONSOLE_APP} telux_tel telux_data telux_common)
endif (AUDIO_ENABLED)

# install to target
install ( TARGETS ${TARGET_TELSDK_CONSOLE_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
