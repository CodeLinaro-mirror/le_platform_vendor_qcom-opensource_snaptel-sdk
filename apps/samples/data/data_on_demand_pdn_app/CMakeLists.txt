cmake_minimum_required(VERSION 2.8.9)

set(TARGET_DATA_ON_DEMAND_PDN_APP data_on_demand_pdn_app)

set(SOURCES
    DataOnDemandPdnApp.cpp
)

# Glib for strlcpy and strlcat
pkg_check_modules(GLIB REQUIRED glib-2.0)
add_compile_options(${GLIB_CFLAGS})

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_DATA_ON_DEMAND_PDN_APP} ${SOURCES})
target_link_libraries(${TARGET_DATA_ON_DEMAND_PDN_APP} ${GLIB_LIBRARIES} telux_data)

# install to target
install ( TARGETS ${TARGET_DATA_ON_DEMAND_PDN_APP}
          RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR} )
