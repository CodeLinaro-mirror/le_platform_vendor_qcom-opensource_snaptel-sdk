/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

/*
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <memory>

#include <telux/tel/PhoneFactory.hpp>

#include "ConfigParser.hpp"

#define DEFAULT_PHONE_NUMBER "+18583562961"

using namespace telux::tel;
using namespace telux::common;
std::shared_ptr<ICall> dialedCall = nullptr;

// ##### 5.1 Implement IMakeCallCallback interface to receive response for the dial request
// - optional
class DialCallback : public IMakeCallCallback {
public:
    void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
       std::cout << "DialCallback::makeCallResponse" << std::endl;
       std::cout << "makeCallResponse ErrorCode: " << int(error) << std::endl;
       if(call) {
          std::cout << "makeCallResponse RemotePartyNumber : " << call->getRemotePartyNumber()
                    << std::endl;
          std::cout << "makeCallResponse getCallIndex : " << call->getCallIndex() << std::endl;
          dialedCall = call;
       }
    }
};

/**
 * Main routine
 */
int main(int argc, char *argv[]) {

   // ### 1. Get the PhoneFactory and CallManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   std::promise<telux::common::ServiceStatus> cbProm = std::promise<telux::common::ServiceStatus>();
   auto callManager = phoneFactory.getCallManager([&](telux::common::ServiceStatus status) {
            cbProm.set_value(status);});
   if(callManager == nullptr) {
      std::cout << " *** ERROR - Unable to get Call Manager instance" << std::endl;
      return 1;
   }

   // ### 2. Wait for the Call Manager subsystem to be ready.
   telux::common::ServiceStatus status = cbProm.get_future().get();
   if(status == SERVICE_AVAILABLE) {
      std::cout << "Call Manager subsystem is ready" << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize Call Manager subsystem" << std::endl;
      return 1;
   }

   // ### 3. Instantiate dial callback instance - this is optional
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback>();

   // ### 4. Send a dial request
   int phoneId = 1;
   std::string phoneNumber = DEFAULT_PHONE_NUMBER;
   auto makeCallStatus = callManager->makeCall(phoneId, phoneNumber, dialCb);
   std::cout << "Dial Call Status:" << (int)makeCallStatus << std::endl;

   // ### 5. Wait for the call state to become active and hang-up the call after conversation
   sleep(10);
   if(dialedCall) {
      dialedCall->hangup();
   }

   // ### 6. Exit logic is specific to an application
   std::cout << "Press enter to exit" << std::endl;
   std::string input;
   std::getline(std::cin, input);
   std::cout << "Exiting application..." << std::endl;
   return 0;
}
