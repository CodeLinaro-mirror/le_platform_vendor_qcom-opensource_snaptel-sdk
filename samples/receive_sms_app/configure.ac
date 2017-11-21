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

AC_PREREQ(2.61)
AC_INIT([receive_sms_app], 1.0.0)
AM_INIT_AUTOMAKE([-Wall gnu foreign])
AM_MAINTAINER_MODE
AC_CONFIG_HEADER([config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AM_PROG_CC_C_O
AM_PROG_AS
AC_PROG_LIBTOOL
AC_PROG_AWK
AC_PROG_CPP
AC_PROG_CXX
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
PKG_PROG_PKG_CONFIG

AC_SUBST([CPPFLAGS])
AC_SUBST([CC])

# This is required for standalone sdk_installer not for meta-qti-telematics bitbake recipe
# local-libs build
   AC_ARG_ENABLE([local-libs],
               AC_HELP_STRING([--enable-local-libs],[Enabling local-libs build]),
     [case "${enableval}" in
       yes) local_libs=true ;;
       no)  local_libs=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-local-libs]) ;;
     esac],[local_libs=false])
   if test x$local_libs = xtrue ; then
      CPPFLAGS="$CPPFLAGS -I../../include"
      LDFLAGS="$LDFLAGS -L../../.libs"
      AC_MSG_NOTICE([Enabling local-libs build.])
   fi

# Debug build
   AC_ARG_ENABLE([debug],
               AC_HELP_STRING([--enable-debug],[Enabling debug build]),
     [case "${enableval}" in
       yes) debug=true ;;
       no)  debug=false ;;
       *) AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
     esac],[debug=false])
   if test x$debug = xtrue ; then
      CPPFLAGS="$CPPFLAGS -g"
      AC_MSG_NOTICE([Enabling debug build.])
   fi

AC_CONFIG_FILES([ \
        Makefile
        ])
AC_OUTPUT
