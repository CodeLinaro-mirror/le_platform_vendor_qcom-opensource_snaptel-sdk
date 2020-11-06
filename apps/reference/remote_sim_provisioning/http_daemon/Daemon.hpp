/*
 *  Copyright (c) 2021 The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       Daemon.hpp
 * @brief      This is singleton daemon class contains the functionalities required for
 *             making HTTPTransactionManager subsystem ready and register for
 *             HTTPTransactionListener to listen for HTTP transaction indication from modem.
 */
#ifndef HTTPDAEMON_HPP
#define HTTPDAEMON_HPP

#include <mutex>
#include <condition_variable>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>

#include "Daemon.hpp"
#include "HttpTransactionListener.hpp"

class HttpDaemon {
public:
   telux::common::Status init();
   int startDaemon(int argc, char **argv);
   void stopDaemon();

   static HttpDaemon &getInstance();

private:
   std::mutex mtx_;
   std::condition_variable cv_;
   bool exiting_ = false;

   // Telux HTTP Transaction
   std::shared_ptr<HttpTransactionListener> httpListener_;
   std::shared_ptr<telux::tel::IHttpTransactionManager> httpMgr_;

   static void signalHandler(int signum);
   void printUsage(char **argv);
   telux::common::Status parseArguments(int argc, char **argv);
};

#endif