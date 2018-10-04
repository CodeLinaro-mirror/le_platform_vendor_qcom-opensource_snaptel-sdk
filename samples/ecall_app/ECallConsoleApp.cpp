/*
 *  Copyright (c) 2017-2018, The Linux Foundation. All rights reserved.
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

// Config file name. Using current directory as default path.
#define MSDSETTINGS_FILE "./msdsettings.txt"
#define UPDATED_MSDSETTINGS_FILE "./updated_msdsettings.txt"

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m"

const std::string GREEN = "\033[0;32m";
const std::string RED = "\033[0;31m";
const std::string BOLD_RED = "\033[1;31m";
const std::string DONE = "\033[0m";  // No color

using namespace telux::tel;
using namespace telux::common;

ECallConsoleApp::ECallConsoleApp(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
   callListener_ = std::make_shared<MyCallListener>();
   callCommandCallback_ = std::make_shared<CallCommandCallback>();
   updateMsdCommandCallback_ = std::make_shared<UpdateMsdCommandCallback>();
   hangupCommandCallback_ = std::make_shared<HangupCommandCallback>();
   answerCommandCallback_ = std::make_shared<AnswerCommandCallback>();
}

ECallConsoleApp::~ECallConsoleApp() {
   removeCallListener(callListener_);
}

/**
 * Initializing Commands and Display
 */
void ECallConsoleApp::init() {
   // below commands are used to add menu options like below
   //
   // 1 - eCall-SOS
   // 2 - eCall <auto | manual> <test | emergency>
   // 3 - update_ecall_msd
   // 4 - dial <number>
   // 5 - hangup
   // 6 - get_calls
   // 7 - answer_call
   //

   std::shared_ptr<ConsoleAppCommand> eCallSos = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("1", BOLD_RED + "eCall-SOS" + DONE, {},
                        std::bind(&ECallConsoleApp::eCallSOS, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> eCall = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("2", "eCall", {ECALL_CATEGORY_AUTO + " | " + ECALL_CATEGORY_MANUAL,
                                       ECALL_VARIANT_TEST + " | " + ECALL_VARIANT_EMERGENCY},
                        std::bind(&ECallConsoleApp::makeECall, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> updateMsd = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("3", "update_ecall_msd", {},
                        std::bind(&ECallConsoleApp::updateECallMSD, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> dial = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
      "4", "dial", {"number"}, std::bind(&ECallConsoleApp::makeCall, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> hangup
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "5", "hangup", {}, std::bind(&ECallConsoleApp::hangup, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> getCalls
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "6", "get_calls", {}, std::bind(&ECallConsoleApp::getCalls, this, std::placeholders::_1)));

   std::shared_ptr<ConsoleAppCommand> answerCall = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("7", "answer_call", {},
                        std::bind(&ECallConsoleApp::answerCall, this, std::placeholders::_1)));

   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsList
      = {eCallSos, eCall, updateMsd, dial, hangup, getCalls, answerCall};

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
   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();
   //  Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   //  Check if telephony subsystem is ready
   bool subSystemStatus = phoneManager->isSubsystemReady();

   //  If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::future<bool> f = phoneManager->onSubsystemReady();
      // If we want to wait unconditionally for telephony subsystem to be ready
      subSystemStatus = f.get();
   }

   //  Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemStatus) {
      endTime = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
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
   int phoneId = DEFAULT_PHONE_ID;
   Status status = callManager->makeCall(phoneId, phoneNumber, callCommandCallback_);
   if(status == Status::SUCCESS) {
      std::cout << GREEN << "  Dial request is successful" << DONE << std::endl;
   } else {
      std::cout << RED << "  Dial request failed" << DONE << std::endl;
   }
}

void ECallConsoleApp::answerCall(std::vector<std::string> inputCommand) {
   auto &phoneFactory = PhoneFactory::getInstance();
   try {
      std::shared_ptr<ICall> spCall = nullptr;
      std::vector<std::shared_ptr<ICall>> inProgressCalls
         = phoneFactory.getCallManager()->getInProgressCalls();
      // Fetch the list of in prgress calls from CallManager and accept the incoming call.
      for(auto callIterator = std::begin(inProgressCalls);
          callIterator != std::end(inProgressCalls); ++callIterator) {
         if((*callIterator)->getCallState() == CallState::CALL_INCOMING) {
            spCall = *callIterator;
            break;
         }
      }
      if(spCall != nullptr) {
         std::cout << "Sending request to accept call " << std::endl;
         spCall->answer(answerCommandCallback_);
      } else {
         std::cout << "No incoming call to accept " << std::endl;
      }
   } catch(const std::exception &e) {
      std::cout << "ERROR: Exception caught -" << e.what() << std::endl;
   }
}

/**
 * Sample hangup operation
 */
void ECallConsoleApp::hangup(std::vector<std::string> inputCommand) {
   try {

      std::shared_ptr<ICall> spCall = nullptr;
      // Iterate through the call list in the application and hangup the first Call that is
      // Active or on Hold
      auto &phoneFactory = PhoneFactory::getInstance();
      std::vector<std::shared_ptr<ICall>> callList
         = phoneFactory.getCallManager()->getInProgressCalls();
      for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
          ++callIterator) {
         CallState callState = (*callIterator)->getCallState();
         if((callState == CallState::CALL_ACTIVE) || (callState == CallState::CALL_DIALING)
            || (callState == CallState::CALL_ALERTING) || (callState == CallState::CALL_ON_HOLD)) {
            spCall = *callIterator;
            break;
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

void ECallConsoleApp::eCallSOS(std::vector<std::string> inputCommand) {
   // Get Phone from PhoneFactory
   auto &phoneFactory = PhoneFactory::getInstance();
   auto spDefaultPhone = phoneFactory.getPhoneManager()->getPhone();

   ECallCategory emergencyCategory = ECallCategory::VOICE_EMER_CAT_AUTO_ECALL;
   ECallVariant eCallVariant = ECallVariant::ECALL_EMERGENCY;

   MsdSettings msdSettings;
   auto eCallMsdData = msdSettings.readMsdFromFile(MSDSETTINGS_FILE);
   auto callManager = phoneFactory.getCallManager();
   int phoneId = DEFAULT_PHONE_ID;
   auto ret = callManager->makeECall(phoneId, eCallMsdData, (int)emergencyCategory,
                                     (int)eCallVariant, callCommandCallback_);
   if(ret == Status::SUCCESS) {
      std::cout << GREEN << "  eCall request is successful" << DONE << std::endl;
   } else {
      std::cout << RED << "  eCall request failed" << DONE << std::endl;
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

   ECallCategory emergencyCategory;
   ECallVariant eCallVariant;

   if(category == ECALL_CATEGORY_AUTO) {  // Automatically triggered eCall.
      emergencyCategory = ECallCategory::VOICE_EMER_CAT_AUTO_ECALL;
   } else if(category == ECALL_CATEGORY_MANUAL) {  // Manually triggered eCall.
      emergencyCategory = ECallCategory::VOICE_EMER_CAT_MANUAL;
   } else {
      std::cout << "Invalid Emergency Call Category --Look Help for usage" << std::endl;
      return;
   }
   if(variant == ECALL_VARIANT_TEST) {  // Will use the PSAP number configured in NV settings
      eCallVariant = ECallVariant::ECALL_TEST;
   } else if(variant
             == ECALL_VARIANT_EMERGENCY) {  // Will use the emergency number configured in FDN
                                            // i.e. 112.
      eCallVariant = ECallVariant::ECALL_EMERGENCY;
   } else {
      std::cout << "Invalid Emergency Call Variant--Look Help for usage" << std::endl;
      return;
   }

   MsdSettings msdSettings;
   auto eCallMsdData = msdSettings.readMsdFromFile(MSDSETTINGS_FILE);
   auto callManager = phoneFactory.getCallManager();
   int phoneId = DEFAULT_PHONE_ID;
   auto ret = callManager->makeECall(phoneId, eCallMsdData, (int)emergencyCategory,
                                     (int)eCallVariant, callCommandCallback_);
   if(ret == Status::SUCCESS) {
      std::cout << GREEN << "  eCall request is successful" << DONE << std::endl;
   } else {
      std::cout << RED << "  eCall request failed" << DONE << std::endl;
   }
}

/**
 * Sample Update eCall MSD operation
 */
void ECallConsoleApp::updateECallMSD(std::vector<std::string> inputCommand) {
   MsdSettings msdSettings;
   auto &phoneFactory = PhoneFactory::getInstance();
   auto spDefaultPhone = phoneFactory.getPhoneManager()->getPhone();
   auto eCallMsdData = msdSettings.readMsdFromFile(UPDATED_MSDSETTINGS_FILE);
   auto callManager = phoneFactory.getCallManager();
   int phoneId = DEFAULT_PHONE_ID;
   auto ret = callManager->updateECallMsd(phoneId, eCallMsdData, updateMsdCommandCallback_);
   if(ret == Status::SUCCESS) {
      std::cout << GREEN << "  Update MSD request is successful" << DONE << std::endl;
   } else {
      std::cout << RED << "  Update MSD request failed" << DONE << std::endl;
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
   std::string infoStr = "";
   if(errorCode == ErrorCode::SUCCESS) {
      infoStr.append("Call is successful ");
   } else {
      infoStr.append("Call failed with error code: " + static_cast<int>(errorCode));
   }

   PRINT_NOTIFICATION << infoStr << std::endl;
}

void ECallConsoleApp::UpdateMsdCommandCallback::commandResponse(ErrorCode errorCode) {
   std::string infoStr = "";
   if(errorCode == ErrorCode::SUCCESS) {
      infoStr.append(" MSD Update is successful");
   } else {
      infoStr.append("Update MSD failed with error code: " + static_cast<int>(errorCode));
   }
   PRINT_NOTIFICATION << infoStr << std::endl;
}

void ECallConsoleApp::HangupCommandCallback::commandResponse(ErrorCode errorCode) {
   std::string infoStr = "";
   if(errorCode == ErrorCode::SUCCESS) {
      infoStr.append(" Hangup is successful");
   } else {
      infoStr.append(" Hangup failed with error code: " + static_cast<int>(errorCode));
   }
   PRINT_NOTIFICATION << infoStr << std::endl;
}

void ECallConsoleApp::AnswerCommandCallback::commandResponse(ErrorCode errorCode) {
   std::string infoStr = "";
   if(errorCode == ErrorCode::SUCCESS) {
      infoStr.append(" Answer Call is successful");
   } else {
      infoStr.append(" Answer call failed with error code: " + static_cast<int>(errorCode));
   }
   PRINT_NOTIFICATION << infoStr << std::endl;
}

// Main function that displays the console and processes user input
int main(int argc, char **argv) {

   ECallConsoleApp eCallConsoleApp("eCall App", GREEN + "eCall> " + DONE);

   eCallConsoleApp.init();  // initialize commands and display

   return eCallConsoleApp.mainLoop();  // Main loop to continuously read and execute commands
}
