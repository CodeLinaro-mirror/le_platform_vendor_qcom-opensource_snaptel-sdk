/*
 *  Copyright (c) 2017, The Linux Foundation. All rights reserved.
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

#ifndef ECALLCONSOLEAPP_HPP
#define ECALLCONSOLEAPP_HPP

#include <cctype>
#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <sstream>
#include <sys/time.h>
#include <vector>

#include <telux/tel/Call.hpp>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Phone.hpp>

#include "ConsoleApp.hpp"
#include "MsdSettings.hpp"

#define print_notification std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

const std::string ECALL_CATEGORY_AUTO = "auto";
const std::string ECALL_CATEGORY_MANUAL = "manual";
const std::string ECALL_VARIANT_TEST = "test";
const std::string ECALL_VARIANT_EMERGENCY = "emergency";

class ECallConsoleApp : public ConsoleApp {
public:
   bool initalizeSDK();

   /**
    * Sample dial operation
    */
   void makeCall(std::vector<std::string> inputCommand);
   /**
    * Sample hangup operation
    */
   void hangup(std::vector<std::string> inputCommand);

   /**
    * Sample eCall operation
    */
   void makeECall(std::vector<std::string> inputCommand);
   /**
    * Sample Update eCall MSD operation
    */
   void updateECallMSD(std::vector<std::string> inputCommand);
   /**
    * Sample get in progress calls operations
    */
   void getCalls(std::vector<std::string> inputCommand);

   std::string getCallDescription(std::shared_ptr<telux::tel::ICall> call);

   /**
    * Register a listener
    */
   void registerCallListener(std::shared_ptr<telux::tel::ICallListener> listener);

   /**
    * Remove a registered listener
    */
   void removeCallListener(std::shared_ptr<telux::tel::ICallListener> listener);

   /**
    * Initialize commands and SDK
    */
   void init();

   ECallConsoleApp(std::string appName, std::string cursor);

   ~ECallConsoleApp();

private:
   /**
    * This method is useful to trim the spaces in options and converting them into LOWERCASE
    */
   std::string toLowerCase(std::string inputOption);

   // Member variable to keep the Listener object alive till application ends.
   std::shared_ptr<telux::tel::ICallListener> callListener_;

   class CallCommandCallback : public telux::tel::IMakeCallCallback {
   public:
      void makeCallResponse(telux::common::ErrorCode error,
                            std::shared_ptr<telux::tel::ICall>) override;
   };
   std::shared_ptr<CallCommandCallback> callCommandCallback_;

   class UpdateMsdCommandCallback : public telux::common::ICommandResponseCallback {
   public:
      void commandResponse(telux::common::ErrorCode error) override;
   };
   std::shared_ptr<UpdateMsdCommandCallback> updateMsdCommandCallback_;

   class HangupCommandCallback : public telux::common::ICommandResponseCallback {
   public:
      void commandResponse(telux::common::ErrorCode error) override;
   };
   std::shared_ptr<HangupCommandCallback> hangupCommandCallback_;
};

#endif  // ECALLCONSOLEAPP_HPP
