set(INPUT_LIST "telux_common" "telux_data" "telux_loc" "telux_qmi" "telux_tel")

foreach(STUB_LIB ${INPUT_LIST})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fPIC")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--hash-style=gnu -DFNAME=so_stub_for_lib${STUB_LIB} \@lib${STUB_LIB}.sym")
    add_library (${STUB_LIB} SHARED stubs.c)
    install ( TARGETS ${STUB_LIB}
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION bin
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} )
endforeach()