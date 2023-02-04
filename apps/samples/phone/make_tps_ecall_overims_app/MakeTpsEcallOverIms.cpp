/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <memory>

#include <telux/tel/PhoneFactory.hpp>

using namespace telux::tel;
using namespace telux::common;


// ##### 6.1. implement IMakeCallCallback interface to receive response for the dial request -
// optional
class DialCallback : public IMakeCallCallback {
public:
   void makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) override;
};

void DialCallback::makeCallResponse(ErrorCode error, std::shared_ptr<ICall> call) {
   std::cout << "DialCallback::makeCallResponse" << std::endl;
   std::cout << "makeCallResponse ErrorCode: " << int(error) << std::endl;
   if(call) {
      std::cout << "makeCallResponse::onCallInfoChange: "
                << " Call Index: " << (int)call->getCallIndex()
                << " Call Direction: " << (int)call->getCallDirection()
                << " Phone Number: " << call->getRemotePartyNumber() << std::endl;
   }
}

/**
 * Main routine
 */
int main(int, char **) {

   // ### 1. Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   // ### 2. Check if telephony subsystem is ready
   bool subSystemsStatus = phoneManager->isSubsystemReady();

   // #### 2.1 If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready" << std::endl;
      std::cout << "wait unconditionally for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemsStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemsStatus) {
      std::cout << " *** Sub Systems Ready *** " << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }

   // ### 4. Instantiate Phone and call manager
   auto phone = phoneManager->getPhone();
   std::shared_ptr<ICallManager> callManager = phoneFactory.getCallManager();

   // ### 5. Get unique id of the phone
   int phoneId = DEFAULT_PHONE_ID;

   // ### 6. Instantiate dial callback instance - this is optional
   std::shared_ptr<DialCallback> dialCb = std::make_shared<DialCallback>();

   // ### 7. Create details required to make custom number eCall over IMS like dialnumber,
   // ###    msd data, Optional SIP headers.

   // Input Dialnumber
   std::string dialNumber = "";
   std::cout << "Enter phone number: ";
   std::getline(std::cin, dialNumber, delimiter);
   if (dialNumber.empty()) {
        std::cout << "No input, please provide a valid phone number" << std::endl;
    }
    // MSD data
    std::vector<uint8_t> rawData;
    rawData = {2, 41, 68, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0, 128, 4, 52, 10, 140,
              65, 89, 164, 56, 119, 207, 131, 54, 210, 63, 65, 104, 16, 24, 8, 32, 19, 198, 68, 0,
              0, 48, 20};
    // Optional SIP headers
    CustomSipHeader header;
    char delimiter = '\n';
    std::string temp = "";
    std::cout << "Enter Custom SIP Header for contentType (uses default for no input): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        contentTypeHeader = temp;
    } else {
        std::cout << "No input, proceeding with contentType: " << std::endl;
    }
    temp = "";
    std::cout << "Enter Custom SIP Header for acceptInfo (uses default for no input): ";
    std::getline(std::cin, temp, delimiter);
    if (!temp.empty()) {
        acceptInfoHeader = temp;
    } else {
        std::cout << "No input, proceeding with contentType: " << std::endl;
    }
    if ((contentType == "") && (acceptInfo == "")) {          //Default SIP headers
        header.contentType = telux::tel::CONTENT_HEADER;
        header.acceptInfo = "";
    } else if ((contentType == "") && (acceptInfo != "")) {   //Default SIP header for contentType
        header.contentType = telux::tel::CONTENT_HEADER;
        header.acceptInfo = acceptInfo;
    } else {
        header.contentType = contentType;                    //Custom SIP headers
        header.acceptInfo = acceptInfo;
    }

   // ### 8. Send a eCall request
   if(callManager) {
      auto makeCallStatus
         = callManager->makeECall(phoneId, dialNumber, rawData, header, dialCb);
      std::cout << "Dial ECall Status:" << (int)makeCallStatus << std::endl;
   }

   // ### 9. Exit logic is specific to an application
   std::cout << "Press enter to exit" << std::endl;
   std::string input;
   std::getline(std::cin, input);
   std::cout << "Exiting application..." << std::endl;
   return 0;
}
