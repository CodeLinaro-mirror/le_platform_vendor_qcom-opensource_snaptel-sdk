/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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

#include <iostream>

#include "MySmsListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MySmsListener::onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> smsMsg) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "Received SMS from phone ID " << phoneId << " from: "
                      << smsMsg->getSender() << "\n message: " << smsMsg->getText() << std::endl;
}

// Implementation of My SMS callback
void MySmsCommandCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "sendSmsResponse successfully" << std::endl;
   } else {
      PRINT_CB << "sendSmsResponse failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

// Implementation of SMSC Address callback
void MySmscAddressCallback::smscAddressResponse(const std::string &address,
                                                telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "requestSmscAddress smscAddressResponse: " << address << std::endl;
   } else {
      PRINT_CB << "requestSmscAddress failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

// Implementation of set SMSC Address callback
void MySetSmscAddressResponseCallback::setSmscResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "setSmscAddress sent successfully" << std::endl;
   } else {
      PRINT_CB << "setSmscAddress failed with errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void MySmsDeliveryCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "SMS Delivered successfully" << std::endl;
   } else {
      PRINT_CB << "SMS Delivery failed, errorCode: " << (int)error
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}
