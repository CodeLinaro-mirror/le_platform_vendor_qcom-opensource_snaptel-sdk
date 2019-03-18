#
# This file creates stub libraries for Telematics SDK
#

# NOTE: Please update newly added library in the list
set(INPUT_LIST "telux_common" "telux_data" "telux_loc" "telux_qmi" "telux_tel" "telux_cv2x" "telux_audio")

# Loop to create stub library from the INPUT_LIST
foreach(STUB_LIB ${INPUT_LIST})
    add_library (${STUB_LIB} SHARED stubs.c)

    # link the sym file for the given target
    set_target_properties(${STUB_LIB} PROPERTIES LINK_FLAGS "-DFNAME=so_stub_for_lib${STUB_LIB} ${CMAKE_SOURCE_DIR}/stubs.c \@${CMAKE_SOURCE_DIR}/lib${STUB_LIB}.sym")

    install ( TARGETS ${STUB_LIB}
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
endforeach()
