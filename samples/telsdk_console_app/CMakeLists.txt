##############################################################################
#  Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions are
#  met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
#       copyright notice, this list of conditions and the following
#       disclaimer in the documentation and/or other materials provided
#       with the distribution.
#     * Neither the name of The Linux Foundation nor the names of its
#       contributors may be used to endorse or promote products derived
#       from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
#  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
#  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
#  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
#  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
#  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
#  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
#  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
#  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
#  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
#  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##############################################################################

cmake_minimum_required(VERSION 2.8.9)

set(TARGET_TELSDK_CONSOLE_APP telsdk_console_app)

set(TELSDK_CONSOLE_APP_SOURCES
   Phone/PhoneMenu.cpp
   Phone/MyPhoneListener.cpp
   Phone/MySubscriptionListener.cpp

   Call/CallMenu.cpp
   Call/MyCallListener.cpp

   SimCardServices/SimCardServicesMenu.cpp
   SimCardServices/Card/CardServicesMenu.cpp
   SimCardServices/Card/MyCardListener.cpp
   SimCardServices/SapCard/SapCardServicesMenu.cpp
   SimCardServices/SapCard/MySapCardListener.cpp

   Sms/SmsMenu.cpp
   Sms/MySmsListener.cpp

   Location/LocationMenu.cpp
   Location/MyLocationListener.cpp
   Location/MyLocationCommandCallback.cpp

   ECall/ECallMenu.cpp
   ECall/MsdSettings.cpp
   ECall/MyECallListener.cpp

   Data/DataMenu.cpp
   Data/DataResponseCallback.cpp
   Data/DataListener.cpp

   TelSdkConsoleApp.cpp

   console_app_framework/ConsoleAppCommand.cpp
   console_app_framework/ConsoleApp.cpp
)

macro(SYSR_INCLUDE_DIR subdir)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I =/usr/include/${subdir}")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I =/usr/include/${subdir}")
endmacro()

include_directories(BEFORE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
# add these sub-folders from /usr/include/<subdir>
SYSR_INCLUDE_DIR(telux)

# set global variables
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable (${TARGET_TELSDK_CONSOLE_APP} ${TELSDK_CONSOLE_APP_SOURCES})
target_link_libraries(${TARGET_TELSDK_CONSOLE_APP} telux_tel telux_loc telux_data)

# install to target
install ( TARGETS ${TARGET_TELSDK_CONSOLE_APP}
          RUNTIME DESTINATION bin
          ARCHIVE DESTINATION bin )
