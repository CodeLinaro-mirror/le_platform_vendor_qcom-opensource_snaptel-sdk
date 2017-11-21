##############################################################################
#  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

##############################################################################
#
# FILE: Makefile
# DESCRIPTION: It creates an sample program by linking libtel
#
##############################################################################

#====================================================
# build tel-sdk samples and link tel-sdk library
#====================================================
GTEST_RDIR := ../../mock/googletest

app_source_files = \
   TelSdkConsoleApp.cpp \
   MyPhoneListener.cpp \
   MyCardHandler.cpp \
   MyCallListener.cpp \
   MsdSettings.cpp \
   MySmsHandler.cpp

app_header_files = \
   MyPhoneListener.hpp \
   MsdSettings.hpp \
   MyCardHandler.hpp \
   MyCallListener.hpp \
   MySmsHandler.hpp

bin_PROGRAMS = telsdk_console_app
telsdk_console_app_SOURCES = $(app_header_files) $(app_source_files)
telsdk_console_app_CPPFLAGS  := -std=c++11
telsdk_console_app_LDFLAGS   := -ltelux_tel -rdynamic

if BUILD_MOCK
telsdk_console_app_CPPFLAGS  += -I../../mock # -I$(top_srcdir)/mock
endif
if BUILD_OFF_MOCK
telsdk_console_app_CPPFLAGS  += -I../../mock # -I$(top_srcdir)/mock
telsdk_console_app_LDFLAGS += $(GTEST_RDIR)/googlemock/libgmock_main.a
endif
