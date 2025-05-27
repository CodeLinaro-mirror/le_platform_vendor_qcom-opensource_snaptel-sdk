cmake_minimum_required(VERSION 2.8.9)

include(GNUInstallDirs)
include(FindPkgConfig)
include(CMakePrintHelpers)
cmake_print_variables(telematics-apps_SOURCE_DIR)

#Print telematics-apps_SOURCE_DIR
message(STATUS "telematics-apps_SOURCE_DIR=${telematics-apps_SOURCE_DIR}")

# Assign a variable to a specific directory
set(APPS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../..")

set(TARGET_TELSDK_CONSOLE_APP telsdk_console_app)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    #${telematics-apps_SOURCE_DIR}/common
    #${telematics-apps_SOURCE_DIR}/common/utils
    ${APPS_SOURCE_DIR}/common
    ${APPS_SOURCE_DIR}/common/utils
)

if (TELSDK_FEATURE_TEL)
set(TELSDK_CONSOLE_APP_SOURCES ${TELSDK_CONSOLE_APP_SOURCES}
   #Phone/PhoneMenu.cpp
   #Phone/MyPhoneListener.cpp
   #Phone/MySubscriptionListener.cpp
   #Phone/MySignalStrengthHandler.cpp
   #Phone/MyCellInfoHandler.cpp
   #Phone/SuppServicesMenu.cpp
   #Phone/SuppServicesHandler.cpp
   #Call/CallMenu.cpp
   #Call/MyCallListener.cpp
   #Call/conference/ConferenceMenu.cpp
   #Call/realTimeText/RttMenu.cpp

   #SimCardServices/SimCardServicesMenu.cpp
   #SimCardServices/Card/CardServicesMenu.cpp
   #SimCardServices/Card/MyCardListener.cpp
   #SimCardServices/Card/CardFileMenu.cpp
   #SimCardServices/SapCard/SapCardServicesMenu.cpp
   #SimCardServices/SapCard/MySapCardListener.cpp

   #Sms/SmsMenu.cpp
   #Sms/MySmsListener.cpp

   #ECall/ECallMenu.cpp
   #ECall/MsdSettings.cpp
   #ECall/MyECallListener.cpp

   #Phone/MyServingSystemHandler.cpp
   #Phone/ServingSystemMenu.cpp

   #Phone/MyNetworkSelectionHandler.cpp
   #Phone/NetworkMenu.cpp

   #MultiSim/MyMultiSimHandler.cpp
   #MultiSim/MyMultiSimListener.cpp
   #MultiSim/MultiSimMenu.cpp

   #Rsp/RspListener.cpp
   #Rsp/MyRspHandler.cpp
   #Rsp/RspMenu.cpp

   #Cellbroadcast/CellbroadcastMenu.cpp
   #Cellbroadcast/CellbroadcastListener.cpp
   #Cellbroadcast/CellbroadcastHandler.cpp

   #ImsSettings/MyImsSettingsHandler.cpp
   #ImsSettings/ImsSettingsListener.cpp
   #ImsSettings/ImsSettingsMenu.cpp

   #ImsServingSystem/ImsServingSystemMenu.cpp
   #ImsServingSystem/MyImsServSysListener.cpp

   #ModemStatus.cpp
)
endif (TELSDK_FEATURE_TEL)

if (TELSDK_FEATURE_DATA)

set(TELSDK_CONSOLE_APP_SOURCES ${TELSDK_CONSOLE_APP_SOURCES}

   Data/DataMenu.cpp
   Data/DataResponseCallback.cpp
   Data/DataListener.cpp
   Data/MyProfileListener.cpp
   Data/DataUtils.cpp
   Data/data_profile/DataProfileMenu.cpp
   Data/serving_system/ServingSystemMenu.cpp
   Data/serving_system/ServingSystemListener.cpp
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
   Data/data_settings/DataSettingsMenu.cpp
   Data/client/ClientMenu.cpp
   Data/client/ClientListener.cpp
   Data/dual_data/DualDataManagementMenu.cpp
   Data/data_control/DataControlMenu.cpp
   Data/network_setting/NetworkSettingMenu.cpp
)
endif (TELSDK_FEATURE_DATA)

# To disable audio from telsdk_console_app please disable flag AUDIO_ENABLED
if (TELSDK_FEATURE_AUDIO)
    set(TELSDK_CONSOLE_APP_SOURCES ${TELSDK_CONSOLE_APP_SOURCES}
        Audio/AudioClient.cpp
        ${telematics-apps_SOURCE_DIR}/common/Audio/VoiceSession.cpp
        ${telematics-apps_SOURCE_DIR}/common/Audio/AudioSession.cpp
        ${telematics-apps_SOURCE_DIR}/common/Audio/AudioHelper.cpp
    )
endif (TELSDK_FEATURE_AUDIO)

set(TELSDK_CONSOLE_APP_SOURCES ${TELSDK_CONSOLE_APP_SOURCES}
   TelSdkConsoleApp.cpp
   #${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
   #${telematics-apps_SOURCE_DIR}/common/utils/SignalHandler.cpp
   #${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
   #${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
   #${telematics-apps_SOURCE_DIR}/common/ConfigParser.cpp
   ${APPS_SOURCE_DIR}/common/utils/Utils.cpp
   ${APPS_SOURCE_DIR}/common/utils/SignalHandler.cpp
   ${APPS_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
   ${APPS_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp
   ${APPS_SOURCE_DIR}/common/ConfigParser.cpp
)

# set global variables
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Wno-class-memaccess")

add_executable (${TARGET_TELSDK_CONSOLE_APP} ${TELSDK_CONSOLE_APP_SOURCES})


set (REQUIRED_LIBS telux_common pthread)

if (TELSDK_FEATURE_TEL)
set (REQUIRED_LIBS ${REQUIRED_LIBS} telux_tel)
endif (TELSDK_FEATURE_TEL)
if (TELSDK_FEATURE_DATA)
set (REQUIRED_LIBS ${REQUIRED_LIBS} telux_data telux_common)
endif (TELSDK_FEATURE_DATA)
if (TELSDK_FEATURE_AUDIO)
set (REQUIRED_LIBS ${REQUIRED_LIBS} telux_audio)
endif (TELSDK_FEATURE_AUDIO)

target_link_libraries(${TARGET_TELSDK_CONSOLE_APP} ${REQUIRED_LIBS})

# install to target
install ( TARGETS ${TARGET_TELSDK_CONSOLE_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
install ( FILES config_files/telsdk_app.conf
          DESTINATION ${CMAKE_INSTALL_SYSCONFDIR}
          PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ WORLD_READ )
