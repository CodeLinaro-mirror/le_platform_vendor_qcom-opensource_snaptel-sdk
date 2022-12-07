cmake_minimum_required(VERSION 2.8.9)

if(NOT BUILD_FOR_SIMULATION)
    # build only if we are not building in bitbake environment
    if(BUILD_ALL_REFERENCE)
        add_subdirectory( loc_tracker_app )
    endif(BUILD_ALL_REFERENCE)

    if(WITH_LOCATION)
        add_subdirectory( chrony-sock)
    endif(WITH_LOCATION)

    if(MACHINE_HAS_CV2X_ONLY OR MACHINE_HAS_CV2X)
        add_subdirectory( rits )
    endif(MACHINE_HAS_CV2X_ONLY OR MACHINE_HAS_CV2X)

    if(NOT MACHINE_HAS_CV2X_ONLY)
        add_subdirectory( remote_sim )
        add_subdirectory( ecall_app )
    endif(NOT MACHINE_HAS_CV2X_ONLY)
endif(NOT BUILD_FOR_SIMULATION)
add_subdirectory( power_ref_app )
