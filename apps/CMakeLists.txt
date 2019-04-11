cmake_minimum_required(VERSION 2.8.9)

project (telematics-apps)

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

add_subdirectory( tests/telsdk_console_app )

# build only if we are not building in bitbake environment
if(BUILD_ALL_REFERENCE)
    add_subdirectory( reference/ecall_app )
    add_subdirectory( reference/loc_tracker_app )
endif(BUILD_ALL_REFERENCE)

if(BUILD_ALL_APPS)
    add_subdirectory( samples/make_call_app )
    add_subdirectory( samples/make_ecall_app )
    add_subdirectory( samples/card_services_app )
    add_subdirectory( samples/receive_sms_app )
    add_subdirectory( samples/send_sms_app )
    add_subdirectory( samples/simple_sap_api_app )
    add_subdirectory( samples/loc_app )
    add_subdirectory( samples/subscription_app )
    add_subdirectory( samples/data_app )
    add_subdirectory( samples/data_profile_app )
    add_subdirectory( samples/network_scan_app )
    add_subdirectory( samples/serving_system_app )
    add_subdirectory( tests/audio_test_app )
endif(BUILD_ALL_APPS)

if(MACHINE_HAS_CV2X_ONLY)
    add_subdirectory( samples/cv2x_rx_app )
    add_subdirectory( samples/cv2x_tx_app )
    add_subdirectory( samples/cv2x_get_status_app )
    add_subdirectory( samples/cv2x_get_capabilities_app )
    add_subdirectory( reference/cv2x-daemon )
endif(MACHINE_HAS_CV2X_ONLY)
