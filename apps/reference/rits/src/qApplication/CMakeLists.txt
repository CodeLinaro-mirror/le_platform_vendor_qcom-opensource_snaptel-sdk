macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

SYSR_INCLUDE_DIR(telux)

set(LIBQAPPLICATION_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/./Application/ApplicationBase.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./Application/SaeApplication.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./Application/NullSecurity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./Ldm/Ldm.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/./SafetyApps/safetyapp_util.cpp
)
if(DEFINED ENV{AEROLINK_PATH} OR DEFINED WITH_AEROLINK)
    set(LIBQAPPLICATION_SOURCES ${LIBQAPPLICATION_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/./Application/AerolinkSecurity.cpp)
    set(LIBAERO libviicsec.so)
endif()

if(DEFINED ENV{ASN1C_PATH} OR DEFINED ASN1C_PATH)
    set(LIBQAPPLICATION_SOURCES ${LIBQAPPLICATION_SOURCES}
        ${CMAKE_CURRENT_SOURCE_DIR}/./Application/EtsiApplication.cpp)
endif()

add_library(qapplication ${LIBQAPPLICATION_SOURCES})

target_link_libraries(qapplication ${GLIB_LIBRARIES} telux_cv2x telux_loc telux-squish v2xcodec ${LIBAERO} rt pthread)

install(TARGETS qapplication
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
