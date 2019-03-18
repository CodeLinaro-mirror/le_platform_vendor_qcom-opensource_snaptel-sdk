cmake_minimum_required(VERSION 2.8.9)

# provides install directory variables CMAKE_INSTALL_<dir>
include(GNUInstallDirs)
# pkg-config module
include(FindPkgConfig)

add_subdirectory( telsdk_console_app )

# build only if we are not building in bitbake environment
if(BUILD_ALL_SAMPLES)
    add_subdirectory( make_call_app )
    add_subdirectory( make_ecall_app )
    add_subdirectory( card_services_app )
    add_subdirectory( ecall_app )
    add_subdirectory( receive_sms_app )
    add_subdirectory( send_sms_app )
    add_subdirectory( simple_sap_api_app )
    add_subdirectory( loc_tracker_app )
    add_subdirectory( loc_app )
    add_subdirectory( subscription_app )
    add_subdirectory( data_app )
    add_subdirectory( data_profile_app )
    add_subdirectory( network_selection_app )
    add_subdirectory( serving_system_app )
    add_subdirectory( audio_test_app )
endif(BUILD_ALL_SAMPLES)

if(MACHINE_HAS_CV2X_ONLY)
    add_subdirectory( cv2x_rx_app )
    add_subdirectory( cv2x_tx_app )
    add_subdirectory( cv2x_get_status_app )
    add_subdirectory( cv2x_get_capabilities_app )
endif(MACHINE_HAS_CV2X_ONLY)
