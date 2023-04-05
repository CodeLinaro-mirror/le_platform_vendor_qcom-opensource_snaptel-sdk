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

/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:

 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * Call Menu class provides dialer functionality of the SDK
 * it has menu options for dial, answer, hangup, reject, conference and swap calls
 */

#include <chrono>
#include <iostream>

#include <telux/tel/PhoneFactory.hpp>

#include "CallMenu.hpp"

CallMenu::CallMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {

   std::chrono::time_point<std::chrono::steady_clock> startTime, endTime;
   startTime = std::chrono::steady_clock::now();
   // Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = telux::tel::PhoneFactory::getInstance();
   phoneManager_ = phoneFactory.getPhoneManager();

   // Check if telephony subsystem is ready
   bool subSystemStatus = phoneManager_->isSubsystemReady();

   // If telephony subsystem is not ready, wait for it to be ready
   if(!subSystemStatus) {
      std::cout << "\nTelephony subsystem is not ready, Please wait" << std::endl;
      std::future<bool> f = phoneManager_->onSubsystemReady();
      // Wait unconditionally for telephony subsystem to be ready
      subSystemStatus = f.get();
   }

   // Exit the application, if SDK is unable to initialize telephony subsystems
   if(subSystemStatus) {
      endTime = std::chrono::steady_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count() << "s"
                << std::endl;
      phoneManager_->getPhoneIds(phoneIds_);
   } else {
      std::cout << "ERROR - Unable to initialize subSystem" << std::endl;
      exit(0);
   }
   std::promise<telux::common::ServiceStatus> prom;
   //  Get the PhoneFactory and CallManager instances.
   callManager_ = phoneFactory.getCallManager([&](telux::common::ServiceStatus status) {
   if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      prom.set_value(telux::common::ServiceStatus::SERVICE_AVAILABLE);
   } else {
      prom.set_value(telux::common::ServiceStatus::SERVICE_FAILED);
   }
   });
   if(!callManager_) {
      std::cout << "ERROR - Failed to get CallManager instance \n";
      exit(1);
    }

    telux::common::ServiceStatus callMgrsubSystemStatus = callManager_->getServiceStatus();
    if(callMgrsubSystemStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       std::cout << "CallManager subsystem is not ready "
                  << ", Please wait " << std::endl;
    }
    callMgrsubSystemStatus = prom.get_future().get();
   if(callMgrsubSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
      myDialCallCmdCb_ = std::make_shared<MyDialCallback>();
      myHangupCb_ = std::make_shared<MyCallCommandCallback>("Hang");
      myHoldCb_ = std::make_shared<MyCallCommandCallback>("Hold");
      myResumeCb_ = std::make_shared<MyCallCommandCallback>("Resume");
      myAnswerCb_ = std::make_shared<MyCallCommandCallback>("Answer");
      myRejectCb_ = std::make_shared<MyCallCommandCallback>("Reject");
      myConferenceCb_ = std::make_shared<MyCallCommandCallback>("Conference");
      mySwapCb_ = std::make_shared<MyCallCommandCallback>("Swap");
      myPlayTonesCb_ = std::make_shared<MyCallCommandCallback>("Play Tone");
      myStartToneCb_ = std::make_shared<MyCallCommandCallback>("Start Tone");
      myStopToneCb_ = std::make_shared<MyCallCommandCallback>("Stop Tone");
      callListener_ = std::make_shared<MyCallListener>();
      // registering listener
      telux::common::Status status = callManager_->registerListener(callListener_);
      if(status != telux::common::Status::SUCCESS) {
         std::cout << "Unable to register Call Manager listener" << std::endl;
         exit(1);
      }
    } else {
       std::cout << "Unable to initialise CallManager subsystem " << std::endl;
       exit(1);
    }
}

CallMenu::~CallMenu() {
   callManager_->removeListener(callListener_);
   myDialCallCmdCb_ = nullptr;
   myHangupCb_ = nullptr;
   myHoldCb_ = nullptr;
   myResumeCb_ = nullptr;
   myAnswerCb_ = nullptr;
   myRejectCb_ = nullptr;
   myConferenceCb_ = nullptr;
   mySwapCb_ = nullptr;
   myPlayTonesCb_ = nullptr;
   myStartToneCb_ = nullptr;
   myStopToneCb_ = nullptr;
}

void CallMenu::init() {
   std::shared_ptr<ConsoleAppCommand> dialCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "1", "Dial", {"number"}, std::bind(&CallMenu::dial, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> acceptCallCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "2", "Accept_call", {}, std::bind(&CallMenu::acceptCall, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> rejectCallCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "3", "Reject_call", {}, std::bind(&CallMenu::rejectCall, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> rejectWithSmsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("4", "Reject_call_with_sms", {},
                        std::bind(&CallMenu::rejectWithSms, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> hangupWithCallIndexCommand
      = std::make_shared<ConsoleAppCommand>(
         ConsoleAppCommand("5", "Hangup", {"index"},
                           std::bind(&CallMenu::hangupWithCallIndex, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> hangupDialingOrAlertingCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "6", "Hangup", {},
         std::bind(&CallMenu::hangupDialingOrAlerting, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> holdCallCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "7", "Hold_call", {}, std::bind(&CallMenu::holdCall, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> resumeCallCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "8", "Resume_call", {}, std::bind(&CallMenu::resumeCall, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> conferenceCommand
      = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
         "9", "Conference", {}, std::bind(&CallMenu::conference, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> swapCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("10", "Swap", {}, std::bind(&CallMenu::swap, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> getCallsCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("11", "Get_InProgress_Calls", {},
                        std::bind(&CallMenu::getCalls, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> playDtmfTonesCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("12", "Play_DTMF_tone", {"number * #"},
                        std::bind(&CallMenu::playDtmfTone, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> startDtmfToneCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("13", "Start_DTMF_tone", {},
                        std::bind(&CallMenu::startDtmfTone, this, std::placeholders::_1)));
   std::shared_ptr<ConsoleAppCommand> stopDtmfToneCommand = std::make_shared<ConsoleAppCommand>(
      ConsoleAppCommand("14", "Stop_DTMF_tone", {},
                        std::bind(&CallMenu::stopDtmfTone, this, std::placeholders::_1)));
   std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListCallSubMenu
      = {dialCommand,
         acceptCallCommand,
         rejectCallCommand,
         rejectWithSmsCommand,
         hangupWithCallIndexCommand,
         hangupDialingOrAlertingCommand,
         holdCallCommand,
         resumeCallCommand,
         conferenceCommand,
         swapCommand,
         getCallsCommand,
         playDtmfTonesCommand,
         startDtmfToneCommand,
         stopDtmfToneCommand};
   addCommands(commandsListCallSubMenu);
   ConsoleApp::displayMenu();
}

void CallMenu::dial(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   const std::string phoneNumber = userInput[1];
   int phoneId = DEFAULT_PHONE_ID;

   if (phoneIds_.size() > 1) {
       std::string slotSelection;
       char delimiter = '\n';

       std::cout << "Enter the desired Phone ID / SIM slot: ";
       std::getline(std::cin, slotSelection, delimiter);

       if (!slotSelection.empty()) {
          try {
             phoneId = std::stoi(slotSelection);
             if (phoneId > 2) {
                std::cout << "Invalid slot entered, using default slot" << std::endl;
                phoneId = DEFAULT_SLOT_ID;
             }
          } catch (const std::exception &e) {
             std::cout << "ERROR: invalid input, please enter a numerical value. INPUT: "
                << slotSelection << std::endl;
             return;
          }
       } else {
          std::cout << "Empty input, enter the correct slot" << std::endl;
       }
   }
   telux::common::Status makeCallStatus
      = callManager_->makeCall(phoneId, phoneNumber, myDialCallCmdCb_);
   std::cout << (makeCallStatus == telux::common::Status::SUCCESS ? "MakeCall is successful"
                                                                  : "MakeCall failed")
             << '\n';
}

void CallMenu::acceptCall(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   // Fetch the list of in progress calls from CallManager and accept the incoming/waiting call.
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if(((*callIterator)->getCallState() == telux::tel::CallState::CALL_INCOMING)
         ||((*callIterator)->getCallState() == telux::tel::CallState::CALL_WAITING)) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      spCall->answer(myAnswerCb_);
   } else {
      std::cout << "No incoming/waiting call" << std::endl;
   }
}

void CallMenu::rejectCall(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   // Fetch the list of in progress calls from CallManager and reject the incoming/waiting call.
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_INCOMING
         || (*callIterator)->getCallState() == telux::tel::CallState::CALL_WAITING) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      spCall->reject(myRejectCb_);
   } else {
      std::cout << "No incoming/waiting call" << std::endl;
   }
}

void CallMenu::rejectWithSms(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   // Fetch the list of in progress calls from CallManager and reject the incoming/waiting call with
   // sms.
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_INCOMING
         ||(*callIterator)->getCallState() == telux::tel::CallState::CALL_WAITING) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      spCall->reject("Testing reject with reason", myRejectCb_);
   } else {
      std::cout << "No incoming/waiting call" << std::endl;
   }
}

void CallMenu::hangupDialingOrAlerting(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   int noOfExistingCalls = 0;
   // Iterate through the call list in the application and hangup the first Call that is
   // in Dialing or Alerting state
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() != telux::tel::CallState::CALL_ENDED) {
         noOfExistingCalls++;
         spCall = *callIterator;
      }
   }
   if(noOfExistingCalls > 1) {
      std::cout << "More than one call: use Hangup cmd with Index " << std::endl;
      return;
   }
   if(spCall) {
      spCall->hangup(myHangupCb_);
   } else {
      std::cout << "No dialing or alerting call found" << std::endl;
   }
}

void CallMenu::hangupWithCallIndex(std::vector<std::string> userInput) {
   int callIndex;
   try {
      callIndex = std::stoi(userInput[1]);
   } catch(std::invalid_argument) {
      std::cout << "Invalid index" << std::endl;
      return;
   }

   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   // Iterate through the call list in the application and hangup the first Call that is
   // in Dialing or Alerting state
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallIndex() == callIndex) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      spCall->hangup(myHangupCb_);
   } else {
      std::cout << "No call found with given index" << std::endl;
   }
}

void CallMenu::holdCall(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ACTIVE) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      spCall->hold(myHoldCb_);
   } else {
      std::cout << "No active call found" << std::endl;
   }
}

void CallMenu::conference(std::vector<std::string> userInput) {
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   if(inProgressCalls.size() < 2) {
      std::cout << "getInProgressCalls does not have 2 calls" << std::endl;
   }
   // Iterate through the call list find the call that is active and the first call that is
   // on hold then conference both the calls
   std::shared_ptr<telux::tel::ICall> spCall1, spCall2;
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ACTIVE) {
         spCall1 = *callIterator;
         continue;
      }

      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ON_HOLD) {
         spCall2 = *callIterator;
      }
      if(spCall1 != nullptr && spCall2 != nullptr) {
         break;
      }
   }
   if(spCall1 != nullptr && spCall2 != nullptr) {
      callManager_->conference(spCall1, spCall2, myConferenceCb_);
   } else {
      std::cout << "Need 1 active and 1 hold call to conference" << std::endl;
   }
}

void CallMenu::swap(std::vector<std::string> userInput) {
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   if(inProgressCalls.size() < 2) {
      std::cout << "call list does not have 2 calls" << std::endl;
   }
   // Iterate through the call list find the call that is active and the first call that is
   // on hold
   // Swap the answer and on-hold calls
   std::shared_ptr<telux::tel::ICall> spCall1, spCall2;
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ACTIVE) {
         spCall1 = *callIterator;
         continue;
      }
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ON_HOLD) {
         spCall2 = *callIterator;
      }
      if(spCall1 != nullptr && spCall2 != nullptr) {
         break;
      }
   }
   if(spCall1 != nullptr && spCall2 != nullptr) {
      callManager_->swap(spCall1, spCall2, mySwapCb_);
   } else {
      std::cout << "Need 1 active and 1 hold call to swap" << std::endl;
   }
}

void CallMenu::getCalls(std::vector<std::string> userInput) {
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   if(inProgressCalls.size() == 0) {
       std::cout << "No calls detected in the system" << std::endl;
   }
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      std::cout << " Call State: "
                << (std::dynamic_pointer_cast<MyCallListener>(callListener_))
                      ->getCallStateString((*callIterator)->getCallState())
                << " Call Index: " << (int)(*callIterator)->getCallIndex()
                << " Call Direction: " << (int)(*callIterator)->getCallDirection()
                << " Phone Number: " << (*callIterator)->getRemotePartyNumber() << std::endl;
   }
}

void CallMenu::resumeCall(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   // Iterate through the call list in the application and resume the
   // call which is on hold
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ON_HOLD) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      spCall->resume(myResumeCb_);
   } else {
      std::cout << "No call to resume which is on hold " << std::endl;
   }
}

void CallMenu::playDtmfTone(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   // Fetch the list of in progress calls from CallManager and accept the
   // incoming call.
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      telux::tel::CallState callState = (*callIterator)->getCallState();
      if(callState == telux::tel::CallState::CALL_ACTIVE
         || callState == telux::tel::CallState::CALL_ALERTING) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      std::string dtmfString = userInput[1];
      if(dtmfString.length() > 0) {
         dtmfString = dtmfString.erase(0, dtmfString.find_first_not_of(" \n\r\t"));
         std::cout << "DTMF string length " << dtmfString.length() << std::endl;
      }
      if(dtmfString.length() == 0) {
         std::cout << "Invalid DTMF String\n";
      } else {
         auto ret = spCall->playDtmfTone(dtmfString[0], myPlayTonesCb_);
         std::cout << (ret == telux::common::Status::SUCCESS ? "Play tone request sent successfully"
                                                             : "Play tone request failed")
                   << '\n';
      }
   } else {
      std::cout << "No active call found" << std::endl;
   }
}

void CallMenu::startDtmfTone(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   // Fetch the list of in progress calls from CallManager and accept the
   // incoming call.
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ACTIVE
         || (*callIterator)->getCallState() == telux::tel::CallState::CALL_ALERTING) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      spCall->startDtmfTone('1', myStartToneCb_);
   } else {
      std::cout << "No active call found" << std::endl;
   }
}

void CallMenu::stopDtmfTone(std::vector<std::string> userInput) {
   std::shared_ptr<telux::tel::ICall> spCall = nullptr;
   std::vector<std::shared_ptr<telux::tel::ICall>> inProgressCalls
      = callManager_->getInProgressCalls();
   // Fetch the list of in progress calls from CallManager and accept the
   // incoming call.
   for(auto callIterator = std::begin(inProgressCalls); callIterator != std::end(inProgressCalls);
       ++callIterator) {
      if((*callIterator)->getCallState() == telux::tel::CallState::CALL_ACTIVE
         || (*callIterator)->getCallState() == telux::tel::CallState::CALL_ALERTING) {
         spCall = *callIterator;
         break;
      }
   }
   if(spCall) {
      spCall->stopDtmfTone(myStopToneCb_);
   } else {
      std::cout << "No active call found" << std::endl;
   }
}
