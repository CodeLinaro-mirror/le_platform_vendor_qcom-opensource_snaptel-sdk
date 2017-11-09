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

#include <iostream>

#include <telux/tel/PhoneFactory.hpp>

#include "ECallConsoleApp.hpp"
#include "MyCallListener.hpp"

using namespace telux::tel;
using namespace telux::common;

ECallConsoleApp::ECallConsoleApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   callListener_ = std::make_shared<MyCallListener>();
   callCommandCallback_ = std::make_shared<CallCommandCallback>();
   updateMsdCommandCallback_ = std::make_shared<UpdateMsdCommandCallback>();
   hangupCommandCallback_ = std::make_shared<HangupCommandCallback>();
}

ECallConsoleApp::~ECallConsoleApp() {
   removeCallListener(callListener_);
}

/**
 * Initializing Commands and Display..
 */
void ECallConsoleApp::init() {
   std::shared_ptr<ConsoleAppCommand> dial = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
      "1", "dial", {"number"}, std::bind(&ECallConsoleApp::makeCall, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> hangup
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "hangup", {}, std::bind(&ECallConsoleApp::hangup, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getCalls
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "getCalls", {}, std::bind(&ECallConsoleApp::getCalls, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> eCall = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("4", "eCall", {ECALL_CATEGORY_AUTO + " | " + ECALL_CATEGORY_MANUAL,
                                       ECALL_VARIANT_TEST + " | " + ECALL_VARIANT_EMERGENCY},
                        std::bind(&ECallConsoleApp::makeECall, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> update_eCall_msd = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("5", "update_ecall_msd", {},
                        std::bind(&ECallConsoleApp::updateECallMSD, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
      = {dial, hangup, getCalls, eCall, update_eCall_msd};

   addCommands(commandsList);

   if(ECallConsoleApp::initalizeSDK()) {
      ConsoleApp::displayMenu();
   }
}

void ECallConsoleApp::registerCallListener(std::shared_ptr<ICallListener> listener) {
   auto &phoneFactory = PhoneFactory::getInstance();
   auto callManager = phoneFactory.getCallManager();
   callManager->registerListener(listener);
}

void ECallConsoleApp::removeCallListener(std::shared_ptr<ICallListener> listener) {
   auto &phoneFactory = PhoneFactory::getInstance();
   auto callManager = phoneFactory.getCallManager();
   callManager->removeListener(listener);
}

bool ECallConsoleApp::initalizeSDK() {
   bool sdkStatus = ConsoleApp::initializeSDK();
   if(sdkStatus) {
      // Registering for Call state events
      registerCallListener(callListener_);
      return true;
   } else {
      std::cout << "Unable to initialize SDK, Exiting!! " << std::endl;
      exit(1);
   }
}

/**
 * Sample dial application
 */
void ECallConsoleApp::makeCall(std::vector<std::string> inputCommand) {
   auto &phoneFactory = PhoneFactory::getInstance();
   auto callManager = phoneFactory.getCallManager();
   auto phoneManager = phoneFactory.getPhoneManager();
   auto spDefaultPhone = phoneManager->getPhone();
   std::cout << "dialing " << inputCommand[1] << std::endl;  // Phone Number entered by user
   std::shared_ptr<ICall> spCall;
   const std::string phoneNumber = inputCommand[1];  // Phone Number mandatory
   int phoneId;
   spDefaultPhone->getPhoneId(phoneId);
   Status status = callManager->makeCall(phoneId, phoneNumber, callCommandCallback_);
   if(status == Status::SUCCESS) {
      std::cout << "Call is successful" << std::endl;
   } else {
      std::cout << "Call failed" << std::endl;
   }
}

/**
 * Sample hangup operation
 */
void ECallConsoleApp::hangup(std::vector<std::string> inputCommand) {
   auto &phoneFactory = PhoneFactory::getInstance();
   try {
      std::shared_ptr<ICall> spCall = nullptr;
      // Iterate through the call list in the application and hangup the first Call that is
      // Active or on Hold
      auto &phoneFactory = PhoneFactory::getInstance();
      std::vector<std::shared_ptr<ICall>> callList
         = phoneFactory.getCallManager()->getInProgressCalls();
      for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
          ++callIterator) {
         if(((*callIterator)->getCallState() == CallState::CALL_ACTIVE)
            || ((*callIterator)->getCallState() == CallState::CALL_ON_HOLD)) {
            spCall = *callIterator;
            break;
         }
      }
      // If no Active or OnHold call is found in the current call list, get the in progress
      // call list from CallMgr
      // Hangup the first Active or OnHold call from that list
      if(spCall == nullptr) {
         std::vector<std::shared_ptr<ICall>> inProgressCalls
            = phoneFactory.getCallManager()->getInProgressCalls();
         for(auto callIterator = std::begin(inProgressCalls);
             callIterator != std::end(inProgressCalls); ++callIterator) {
            if(((*callIterator)->getCallState() == CallState::CALL_ACTIVE)
               || ((*callIterator)->getCallState() == CallState::CALL_ON_HOLD)) {
               spCall = *callIterator;
               break;
            }
         }
      }
      if(spCall != nullptr) {
         std::cout << "Sending request to hangup call " << std::endl;
         spCall->hangup(hangupCommandCallback_);
      } else {
         std::cout << "No active or on-hold call found in the list to hangup " << std::endl;
      }
   } catch(const std::exception &e) {
      std::cout << "ERROR: Exception caught -" << e.what();
   }
}

/**
 * Sample eCall operation
 */
void ECallConsoleApp::makeECall(std::vector<std::string> inputCommand) {
   // Get Phone from PhoneFactory
   auto &phoneFactory = PhoneFactory::getInstance();
   auto spDefaultPhone = phoneFactory.getPhoneManager()->getPhone();

   // Fetch eCall category and variant
   std::string category = toLowerCase(inputCommand[1]);
   std::string variant = toLowerCase(inputCommand[2]);
   std::cout << "eCall variant :" << variant << std::endl;
   std::cout << "eCall category:" << category << std::endl;

   ECallCategory emergencyCategory;
   ECallVariant eCallVariant;

   bool validCategory = false;
   bool validVariant = false;
   if(category == ECALL_CATEGORY_AUTO) {  // Automatically triggered eCall.
      emergencyCategory = ECallCategory::VOICE_EMER_CAT_AUTO_ECALL;
      validCategory = true;
   } else if(category == ECALL_CATEGORY_MANUAL) {  // Manually triggered eCall.
      emergencyCategory = ECallCategory::VOICE_EMER_CAT_MANUAL;
      validCategory = true;
   } else {
      std::cout << "Invalid Emergency Call Category --Look Help for usage" << std::endl;
      return;
   }
   if(variant == ECALL_VARIANT_TEST) {  // Will use the PSAP number configured in NV settings
      eCallVariant = ECallVariant::ECALL_TEST;
      validVariant = true;
   } else if(variant
             == ECALL_VARIANT_EMERGENCY) {  // Will use the emergency number configured in FDN
                                            // i.e. 112.
      eCallVariant = ECallVariant::ECALL_EMERGENCY;
      validVariant = true;
   } else {
      std::cout << "Invalid Emergency Call Variant--Look Help for usage" << std::endl;
      return;
   }

   MsdSettings msdSettings;
   bool msdStatus;
   auto eCallMsdData = msdSettings.readMsdFromFile();
   auto callManager = phoneFactory.getCallManager();
   int phoneId;
   spDefaultPhone->getPhoneId(phoneId);
   auto ret = callManager->makeECall(phoneId, eCallMsdData, (int)emergencyCategory,
                                     (int)eCallVariant, callCommandCallback_);
   if(ret == Status::SUCCESS) {
      std::cout << "eCall is successful" << std::endl;
   } else {
      std::cout << "eCall failed" << std::endl;
   }
}

/**
 * Sample Update eCall MSD operation
 */
void ECallConsoleApp::updateECallMSD(std::vector<std::string> inputCommand) {
   MsdSettings msdSettings;
   auto &phoneFactory = PhoneFactory::getInstance();
   auto spDefaultPhone = phoneFactory.getPhoneManager()->getPhone();
   bool msdStatus;
   auto eCallMsdData = msdSettings.readMsdFromFile();
   auto callManager = phoneFactory.getCallManager();
   int phoneId;
   spDefaultPhone->getPhoneId(phoneId);
   auto ret = callManager->updateECallMsd(phoneId, eCallMsdData, updateMsdCommandCallback_);
   if(ret == Status::SUCCESS) {
      std::cout << "Update_ecall_msd is successful" << std::endl;
   } else {
      std::cout << "Update_ecall_msd failed" << std::endl;
   }
}

/**
 * Sample get in progress calls operations
 */
void ECallConsoleApp::getCalls(std::vector<std::string> inputCommand) {
   auto &phoneFactory = PhoneFactory::getInstance();
   std::vector<std::shared_ptr<ICall>> callList
      = phoneFactory.getCallManager()->getInProgressCalls();
   if(callList.size() == 0) {
      std::cout << "No calls detected in the system" << std::endl;
   } else {
      for(auto call : callList) {
         std::cout << getCallDescription(call) << std::endl;
      }
   }
}

/**
 * Get a human readable description of this call - such as phone numbers, call state
 * Useful for display or debugging
 */
std::string ECallConsoleApp::getCallDescription(std::shared_ptr<ICall> call) {
   std::string callDesc;
   callDesc += "Call Index: " + std::to_string(call->getCallIndex()) + ", ";
   callDesc += "Phone Number: " + call->getRemotePartyNumber() + ", Call State: ";
   callDesc += std::to_string(static_cast<int>(call->getCallState())) + ", Call Type: "
               + std::to_string(static_cast<int>(call->getCallDirection()));
   return callDesc;
}

/**
 * This method is useful to trim the spaces in options and converting them to lower case
 */
std::string ECallConsoleApp::toLowerCase(std::string inputOption) {
   std::string convertedString;
   convertedString
      = inputOption.erase(0, inputOption.find_first_not_of(" \n\r\t"));  // trim std::string
   std::transform(convertedString.begin(), convertedString.end(), convertedString.begin(),
                  [](unsigned char c) { return std::tolower(c); });
   return convertedString;
}

void ECallConsoleApp::CallCommandCallback::makeCallResponse(ErrorCode errorCode,
                                                            std::shared_ptr<ICall> call) {
   print_notification << "Call command returned code:  " << static_cast<int>(errorCode)
                      << std::endl;
   if(call) {
      print_notification << "Call Id received:  " << call->getCallIndex() << std::endl;
   }
}

void ECallConsoleApp::UpdateMsdCommandCallback::commandResponse(ErrorCode errorCode) {
   print_notification << "Update MSD command returned code:  " << static_cast<int>(errorCode)
                      << std::endl;
}

void ECallConsoleApp::HangupCommandCallback::commandResponse(ErrorCode errorCode) {
   print_notification << "Hangup command returned code:  " << static_cast<int>(errorCode)
                      << std::endl;
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {

   ECallConsoleApp eCallConsoleApp("eCall App", "eCall");

   eCallConsoleApp.init();  // initialize commands and display

   return eCallConsoleApp.mainLoop();  // Main loop to continuously read and execute commands
}
