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
   Data/data_filter/MyDataFilterListener.cpp
   Data/data_filter/DataFilterMenu.cpp
   Data/data_filter/ConfigParser.cpp
   Data/bridge/BridgeMenu.cpp
   Data/l2tp/L2tpMenu.cpp

   Phone/MyServingSystemHandler.cpp
   Phone/ServingSystemMenu.cpp

   Phone/MyNetworkSelectionHandler.cpp
   Phone/NetworkMenu.cpp

   MultiSim/MyMultiSimHandler.cpp
   MultiSim/MyMultiSimListener.cpp
   MultiSim/MultiSimMenu.cpp

   TelSdkConsoleApp.cpp

   ModemStatus.cpp

   ${telematics-apps_SOURCE_DIR}/common/utils/Utils.cpp
   ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleAppCommand.cpp
   ${telematics-apps_SOURCE_DIR}/common/console_app_framework/ConsoleApp.cpp

)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${telematics-apps_SOURCE_DIR}/common
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_TELSDK_CONSOLE_APP} ${TELSDK_CONSOLE_APP_SOURCES})
target_link_libraries(${TARGET_TELSDK_CONSOLE_APP} telux_tel telux_data telux_common)

# install to target
install ( TARGETS ${TARGET_TELSDK_CONSOLE_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
