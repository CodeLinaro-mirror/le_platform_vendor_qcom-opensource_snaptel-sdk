cmake_minimum_required(VERSION 2.8.9)

set(TARGET_DATA_LIBRARY telux_data)

set(DATA_STUB_SOURCES
    DataFactoryImplStub.cpp
    DataSubSystemStub.cpp
)
macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -Wall -Werror -pthread")

add_library (${TARGET_DATA_LIBRARY} SHARED ${DATA_STUB_SOURCES})

install ( TARGETS ${TARGET_DATA_LIBRARY}
    LIBRARY DESTINATION  ${CMAKE_INSTALL_LIBDIR} )
