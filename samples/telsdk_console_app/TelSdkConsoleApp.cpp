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

/**
 * Sample Application using Telematics SDK
 * This is a sample simple console based application to test Phone, Sms and other Misc.
 * functionalities
 */

#include <chrono>
#include <future>
#include <iostream>
#include <string>
#include <thread>

#include <telux/tel/PhoneFactory.hpp>

#include "MySmsHandler.hpp"
#include "MyCardHandler.hpp"
#include "MyCallListener.hpp"
#include "MyPhoneListener.hpp"
#include "MsdSettings.hpp"

using namespace telux::tel;
using namespace telux::common;

#define DEFAULT_PHONE_ID 1

#define print_cursor std::cout << "tel_sdk> "

/**
 * List of supported menus
 */
void telSDKMenu() {
   std::cout << "-------------------------------------------" << std::endl;
   std::cout << "               TelSDK Menu                 " << std::endl;
   std::cout << "-------------------------------------------" << std::endl;

   std::cout << "************** Call **************" << std::endl;
   std::cout << "   1 - dial <number> " << std::endl;
   std::cout << "   2 - accept_call" << std::endl;
   std::cout << "   3 - reject_call - [Rejects incoming call]" << std::endl;
   std::cout << "   4 - reject_with_sms - [Rejects incoming call]" << std::endl;
   std::cout << "   5 - hangup <index>" << std::endl;
   std::cout << "   6 - hold" << std::endl;
   std::cout << "   7 - conf" << std::endl;
   std::cout << "   8 - swap" << std::endl;
   std::cout << "   9 - resume" << std::endl;
   std::cout << "   10 - get_calls" << std::endl;
   std::cout << "   15 - ecall <Catogery> <Number>" << std::endl;
   std::cout << "   16 - update_ecall_msd" << std::endl;
   std::cout << "   l7 - call_listener [add|del|help]" << std::endl;
   std::cout << "************** Phone **************" << std::endl;
   // std::cout << "   12 - get_subscription" << std::endl;
   std::cout << "   18 - request_signal_strength" << std::endl;
   std::cout << "************** Card Services **************" << std::endl;
   std::cout << "   100 - get_card <slotId>" << std::endl;
   std::cout << "   101 - get_card_state" << std::endl;
   std::cout << "   102 - get_apps [for selected card]" << std::endl;
   std::cout << "   103 - open_logical_channel  <aid> [for selected card]" << std::endl;
   std::cout << "   104 - close_logical_channel <channel> [for selected card]" << std::endl;
   std::cout << "   105 - transmit_apdu [for selected card]" << std::endl;
   std::cout << "   106 - basic_transmit_apdu [for selected card]" << std::endl;
   std::cout << "   107 - card_listener add|del|help" << std::endl;
   std::cout << "************** Sap Connection Services **************" << std::endl;
   std::cout << "   151 - open_connection" << std::endl;
   std::cout << "   152 - close_connection" << std::endl;
   std::cout << "   153 - get_atr" << std::endl;
   std::cout << "   154 - get_state" << std::endl;
   std::cout << "   155 - transmit_sap_apdu" << std::endl;
   std::cout << "   156 - sim_power_off" << std::endl;
   std::cout << "   157 - sim_power_on" << std::endl;
   std::cout << "   158 - sim_reset" << std::endl;
   std::cout << "   159 - get_card_reader_status" << std::endl;
   std::cout << "************** SMS **************" << std::endl;
   std::cout << "   300 - get_sms_mgr <phoneId> [default phone Id is 1]" << std::endl;
   std::cout << "   301 - send_sms <receiver> <msg>" << std::endl;
   std::cout << "   302 - req_smsc_addr [get SMSC address]" << std::endl;
   std::cout << "   303 - calculate_msg_attr <message>" << std::endl;
   std::cout << "   304 - get_phone_id" << std::endl;
   std::cout << "   305 - sms_listener add|del|help [listen for Incoming SMS]" << std::endl;
   std::cout << "************** Misc. requests *************" << std::endl;
   std::cout << "   l - listener [add <flag>]|del|help" << std::endl;
   std::cout << "   ? - help" << std::endl;
   std::cout << "   q / 0 - exit / quit" << std::endl;
   std::cout << "-------------------------------------------" << std::endl;
}

/**
 * Demonstration of listner implementation
 * listener [add <flag>]|del|help
 */
void removeTerminatedCalls(std::vector<std::shared_ptr<ICall>> &callList) {
   for(auto call = std::begin(callList); call != std::end(callList);) {
      if((*call)->getCallState() == CallState::CALL_ENDED) {
         callList.erase(call);
      } else {
         ++call;
      }
   }
}

void smsListener(std::shared_ptr<MySmsListener> mySmsListener, std::string args, int phoneId) {
   auto smsMgr = PhoneFactory::getInstance().getSmsManager(phoneId);
   if(args == " add") {
      smsMgr->registerListener(mySmsListener);
   } else if(args == " del") {
      smsMgr->removeListener(mySmsListener);
   } else if(args == " help") {
      std::cout << "sms_listener add - to add listener" << std::endl;
      std::cout << "sms_listener del - to delete listener" << std::endl;
      std::cout << "sms_listener help  - to list listener command help" << std::endl;
   } else {
      std::cout << "unsupported sms_listener command with args:" << args;
   }
}

void cardListener(std::shared_ptr<MyCardListener> myCardListener, std::string args) {
   auto cardMgr = PhoneFactory::getInstance().getCardManager();
   if(args == " del") {
      cardMgr->removeListener(myCardListener);
   } else if(args == " help") {
      std::cout << "card_listener add - to add listener" << std::endl;
      std::cout << "card_listener del - to delete listener" << std::endl;
      std::cout << "card_listener help  - to list listener command help" << std::endl;
   } else if(args == " add") {
      cardMgr->registerListener(myCardListener);
   } else {
      std::cout << "unsupported listener command with args:" << args;
   }
}

void callListener(std::shared_ptr<MyCallListener> myCallListener, std::string args) {
   auto callMgr = PhoneFactory::getInstance().getCallManager();
   if(args == " del") {
      callMgr->removeListener(myCallListener);
   } else if(args == " help") {
      std::cout << "call_listener add - to add listener" << std::endl;
      std::cout << "call_listener del - to delete listener" << std::endl;
      std::cout << "call_listener help  - to list listener command help" << std::endl;
      std::cout << "<flag> can be a 32 bit integer representing ListenType enumeration :";
      std::cout << "\n\tCALL_STATE = 1,";
      std::cout << "\n\tECALL_STATE = 2,";
      std::cout << "\nExample:\n\tlistener add 1 - To register for CALL_STATE changes";
      std::cout << "\nExample:\n\tlistener add 3 - To register for both CALL_STATE and ECALL_STATE "
                   "changes";
   } else if(args == " add") {
      callMgr->registerListener(myCallListener);
   } else {
      std::cout << "unsupported listener command with args:" << args;
   }
}

void listener(std::shared_ptr<MyPhoneListener> myPhListener, std::string args) {
   std::shared_ptr<IPhoneManager> phMgr = PhoneFactory::getInstance().getPhoneManager();
   if(args == " del") {
      phMgr->removeListener(myPhListener);
   } else if(args == " help") {
      std::cout << "listener add <flag>" << std::endl;
      std::cout << "listener del  - to delete listener" << std::endl;
      std::cout << "listener help  - to list listener command help" << std::endl;
      std::cout << "<flag> can be a 32 bit integer representing ListenType enumeration :";
      std::cout << "\n\tSERVICE_STATE = 1,";
      std::cout << "\n\tSIGNAL_STRENGTH = 2,";
      std::cout << "\n\tlistener add 3 - To listen for all the events";
   } else {
      std::string::size_type cmd_start = 1, cmd_end = args.find(' ', cmd_start);
      std::string sub_cmd = args.substr(0, cmd_end);
      if((sub_cmd == " add") && (cmd_end != std::string::npos)) {
         int opt = std::stoi(args.substr(cmd_end));
         phMgr->registerListener((ListenType)opt, myPhListener);
      } else
         std::cout << "unsupported listener command with args:" << args;
   }
}

void logSapState(SapState sapState) {
   if(sapState == SapState::SAP_STATE_NOT_ENABLED) {
      std::cout << "Sap state not enabled" << std::endl;
   } else if(sapState == SapState::SAP_STATE_CONNECTING) {
      std::cout << "Sap state connecting" << std::endl;
   } else if(sapState == SapState::SAP_STATE_CONNECTED_SUCCESSFULLY) {
      std::cout << "Sap state connected successfully" << std::endl;
   } else if(sapState == SapState::SAP_STATE_CONNECTION_ERROR) {
      std::cout << "Sap state connection error" << std::endl;
   } else if(sapState == SapState::SAP_STATE_DISCONNECTING) {
      std::cout << "Sap state disconnecting" << std::endl;
   } else if(sapState == SapState::SAP_STATE_DISCONNECTED_SUCCESSFULLY) {
      std::cout << "Sap state disconnected successfully" << std::endl;
   }
}

/**
 * Get a human readable description of this call - such as phone numbers, call state
 * Useful for display or debugging
 */
std::string getCallDescription(std::shared_ptr<ICall> call) {
   std::string callDesc;
   callDesc += " Call Index: " + std::to_string(call->getCallIndex()) + ",";
   callDesc += " Phone Number: " + call->getRemotePartyNumber() + ", Call State: ";
   callDesc += std::to_string(static_cast<int>(call->getCallState())) + ", Call Direction: "
               + std::to_string(static_cast<int>(call->getCallDirection()));
   return callDesc;
}

/**
 * Main routine
 * Initialize and process user input to test sdk
 */
int main(int, char **) {
   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();
   // singleton gets created here
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   bool subSystemsStatus = phoneManager->isSubsystemReady();
   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready, wait for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      // std::future_status status;
      // status = f.wait_for(std::chrono::seconds(20));
      // if (status != std::future_status::ready) {
      //    std::cout << "ERROR " << std::endl;
      //    return 0;
      // }
      subSystemsStatus = f.get();
   }

   if(subSystemsStatus) {
      endTime = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "\nElapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 0;
   }

   auto cardMgr = phoneFactory.getCardManager();

   std::vector<int> phoneIds;
   phoneManager->getPhoneIds(phoneIds);
   std::cout << "\n----------Size of phones = " << phoneIds.size() << std::endl;
   auto myCardListener = std::make_shared<MyCardListener>();
   auto myCallListener = std::make_shared<MyCallListener>();

   auto myOpenLogicalChannelCb = std::make_shared<MyOpenLogicalChannelCallback>();
   auto myTransmitApduCb = std::make_shared<MyTransmitApduResponseCallback>();
   auto myCloseLogicalChannelCb = std::make_shared<MyCardCommandResponseCallback>();

   auto mySapCmdResponseCb = std::make_shared<MySapCommandResponseCallback>();
   auto myAtrCb = std::make_shared<MyAtrResponseCallback>();
   auto myTransmitApduResponseCb = std::make_shared<MySapTransmitApduResponseCallback>();
   auto myCardReaderCb = std::make_shared<MyCardReaderCallback>();

   auto myPhListener = std::make_shared<MyPhoneListener>();
   auto myDialCallCmdCb = std::make_shared<MyDialCallback>();
   auto myECallCmdCb = std::make_shared<MyDialCallback>();
   auto myUpdateMsdCmdCb = std::make_shared<MyUpdateMsdCommandCallback>();

   auto mySignalStrengthCb = std::make_shared<MySignalStrengthCallback>();
   auto myHangupCb = std::make_shared<MyHangupCallback>();
   auto myHoldCb = std::make_shared<MyHoldCallback>();
   auto myResumeCb = std::make_shared<MyResumeCallback>();
   auto myAnswerCb = std::make_shared<MyAnswerCallback>();
   auto myRejectCb = std::make_shared<MyRejectCallback>();
   auto myConferenceCb = std::make_shared<MyConferenceCallback>();
   auto mySwapCb = std::make_shared<MySwapCallback>();
   auto spDefaultPhone = phoneManager->getPhone();
   int defaultPhoneId;
   spDefaultPhone->getPhoneId(defaultPhoneId);
   std::vector<std::shared_ptr<ICall>> callList;

   std::shared_ptr<ICardManager> cardManager = PhoneFactory::getInstance().getCardManager();
   std::shared_ptr<ICallManager> callManager = PhoneFactory::getInstance().getCallManager();
   std::shared_ptr<ISapCardManager> sapCardMgr = PhoneFactory::getInstance().getSapCardManager();
   std::shared_ptr<ICard> card = nullptr;

   // SMS instances
   std::shared_ptr<ISmsManager> smsManager = phoneFactory.getSmsManager();
   int smsPhoneId = DEFAULT_PHONE_ID;
   auto mySmsListener = std::make_shared<MySmsListener>();
   auto mySmsCmdCb = std::make_shared<MySmsCommandCallback>();
   auto mySmscAddrCb = std::make_shared<MySmscAddressCallback>();

   telSDKMenu();

   for(bool is_alive = true; is_alive == true;) {
      // Remove terminated calls after each iteration so that
      // those stale call objects are not used to make any further call operations
      if(!callList.size() > 0) {
         removeTerminatedCalls(callList);
      }

      print_cursor;
      std::shared_ptr<SignalStrength> dummy_str = 0;
      // phoneManager.notifySignalStrengthChange(s_phone,dummy_str);
      std::string input, cmd, args;
      std::getline(std::cin, input);
      std::string::size_type cmd_start = 0, cmd_end = input.find(' ', cmd_start);
      cmd = input.substr(0, cmd_end);
      if(cmd_end != std::string::npos)
         args = input.substr(cmd_end);
      else
         args = "";

      // SMS Menu options
      if(cmd == "get_sms_mgr" || cmd == "300") {
         std::cout << "get SMS manager " << input << std::endl;
         if(args.length() > 0) {
            std::string phId = std::string(args);
            phId = phId.erase(0, phId.find_first_not_of(" \n\r\t"));
            smsPhoneId = std::stoi(phId);
         } else {
            smsPhoneId = DEFAULT_PHONE_ID;
         }
         smsManager = phoneFactory.getSmsManager(smsPhoneId);
      } else if(cmd == "send_sms" || cmd == "301") {
         std::string::size_type cmd_start = 1, cmd_end = args.find(' ', cmd_start);
         if(cmd_end != std::string::npos) {
            std::string smsReceiver = args.substr(0, cmd_end);
            smsReceiver = smsReceiver.erase(0, smsReceiver.find_first_not_of(" \n\r\t"));
            std::string message = args.substr(cmd_end);
            message = message.erase(0, message.find_first_not_of(" \n\r\t"));  // trim std::string

            std::cout << "send sms " << input;
            auto ret = smsManager->sendSms(message, smsReceiver, mySmsCmdCb);
            if(ret == Status::SUCCESS) {
               std::cout << "send sms successfully";
            } else {
               std::cout << "send sms failed";
            }
         } else {
            std::cout << "Invalid argument" << std::endl;
         }
      } else if(cmd == "req_smsc_addr" || cmd == "302") {
         auto ret = smsManager->requestSmscAddress(mySmscAddrCb);
      } else if(cmd == "calculate_msg_attr" || cmd == "303") {
         std::cout << "Calculating Message size " << input;
         if(args.length() > 0) {
            std::string msg = std::string(args);
            msg = msg.substr(1, msg.size());
            std::cout << "message = " << msg;
            auto msgAttributes = smsManager->calculateMessageAttributes(msg);
            std::cout << "msgAttributes encoding = " << (int)msgAttributes.encoding << std::endl;
            std::cout << "msgAttributes numberOfSegments = " << msgAttributes.numberOfSegments
                      << std::endl;
            std::cout << "msgAttributes segmentSize = " << msgAttributes.segmentSize << std::endl;
            std::cout << "msgAttributes numberOfCharsLeftInLastSegment = "
                      << msgAttributes.numberOfCharsLeftInLastSegment << std::endl;
         } else {
            std::cout << "Invalid input" << std::endl;
         }
      } else if(cmd == "get_phone_id" || cmd == "304") {
         std::cout << "Associated PhoneID for this smsManager " << std::endl;
         std::cout << "SMS PhoneId : " << smsManager->getPhoneId() << std::endl;
      } else if(cmd == "sms_listener" || cmd == "305") {
         smsListener(mySmsListener, args, smsPhoneId);
      }

      // DIAL requests
      if(cmd == "dial" || cmd == "1") {
         std::cout << "dialing " << input << std::endl;
         int phoneId;
         spDefaultPhone->getPhoneId(phoneId);
         std::string phoneNumber = "";
         if(args.length() > 0) {
            phoneNumber = std::string(args);
         } else {
            phoneNumber = "8583562961";
         }
         auto makeCallStatus = callManager->makeCall(phoneId, phoneNumber, myDialCallCmdCb);
         std::cout << "Dial Call Status:" << (int)makeCallStatus << std::endl;
         myDialCallCmdCb->waitForResponse(5);  // wait for makeCall response
         callList.emplace_back(myDialCallCmdCb->getCallObj());
         // std::cout << spCall;
      } else if(cmd == "accept_call" || cmd == "2") {
         std::cout << "request accept_call " << input;
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
               spCall->answer(myAnswerCb);
            } else {
               std::cout << "No incoming call to accept " << std::endl;
            }
         } catch(const std::exception &e) {
            std::cout << "ERROR: Exception caught -" << e.what();
         }
      } else if(cmd == "reject_call" || cmd == "3") {
         try {
            std::shared_ptr<ICall> spCall = nullptr;
            // Fetch the list of in prgress calls from CallManager and reject the incoming call.
            std::vector<std::shared_ptr<ICall>> inProgressCalls
               = phoneFactory.getCallManager()->getInProgressCalls();
            for(auto callIterator = std::begin(inProgressCalls);
                callIterator != std::end(inProgressCalls); ++callIterator) {
               if((*callIterator)->getCallState() == CallState::CALL_INCOMING) {
                  spCall = *callIterator;
                  break;
               }
            }
            if(spCall != nullptr) {
               std::cout << "Sending request to reject call " << std::endl;
               spCall->reject(myRejectCb);
            } else {
               std::cout << "No incoming call to reject " << std::endl;
            }
         } catch(const std::exception &e) {
            std::cout << "ERROR: Exception caught -" << e.what();
         }
      } else if(cmd == "reject_with_sms" || cmd == "4") {
         try {
            std::shared_ptr<ICall> spCall = nullptr;
            // Fetch the list of in prgress calls from CallManager and reject the incoming call with
            // sms.
            std::vector<std::shared_ptr<ICall>> inProgressCalls
               = phoneFactory.getCallManager()->getInProgressCalls();
            for(auto callIterator = std::begin(inProgressCalls);
                callIterator != std::end(inProgressCalls); ++callIterator) {
               if((*callIterator)->getCallState() == CallState::CALL_INCOMING) {
                  spCall = *callIterator;
                  break;
               }
            }
            if(spCall != nullptr) {
               std::cout << "Sending request to reject call " << std::endl;
               spCall->reject("Testing reject with reason", myRejectCb);
            } else {
               std::cout << "No incoming call to reject " << std::endl;
            }
         } catch(const std::exception &e) {
            std::cout << "ERROR: Exception caught -" << e.what();
         }
      } else if(cmd == "hangup" || cmd == "5") {
         try {
            std::shared_ptr<ICall> spCall = nullptr;
            // Iterate through the call list in the application and hangup the first Call that is
            // Active or on Hold
            for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
                ++callIterator) {
               if(((*callIterator)->getCallState() == CallState::CALL_ACTIVE)
                  || ((*callIterator)->getCallState() == CallState::CALL_DIALING)
                  || ((*callIterator)->getCallState() == CallState::CALL_ALERTING)
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
                     || ((*callIterator)->getCallState() == CallState::CALL_DIALING)
                     || ((*callIterator)->getCallState() == CallState::CALL_ALERTING)
                     || ((*callIterator)->getCallState() == CallState::CALL_ON_HOLD)) {
                     spCall = *callIterator;
                     break;
                  }
               }
            }
            if(spCall != nullptr) {
               std::cout << "Sending request to hangup call " << std::endl;
               spCall->hangup(myHangupCb);
            } else {
               std::cout << "No active or on-hold call found in the list to hangup " << std::endl;
            }
         } catch(const std::exception &e) {
            std::cout << "ERROR: Exception caught -" << e.what();
         }
      } else if(cmd == "hold" || cmd == "6") {
         try {
            std::cout << "request hold active call" << input;
            std::shared_ptr<ICall> spCall = nullptr;
            // Iterate through the call list in the application and put the first Call that is
            // Active on hold
            for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
                ++callIterator) {
               if((*callIterator)->getCallState() == CallState::CALL_ACTIVE) {
                  spCall = *callIterator;
                  break;
               }
            }
            if(spCall == nullptr) {
               // If no Active or OnHold call is found in the current call list, get the in progress
               // call list from CallMgr
               // Put the Active call from the list on hold
               std::vector<std::shared_ptr<ICall>> inProgressCalls
                  = phoneFactory.getCallManager()->getInProgressCalls();
               for(auto callIterator = std::begin(inProgressCalls);
                   callIterator != std::end(inProgressCalls); ++callIterator) {
                  if((*callIterator)->getCallState() == CallState::CALL_ACTIVE) {
                     spCall = *callIterator;
                     break;
                  }
               }
            }
            if(spCall != nullptr) {
               spCall->hold(myHoldCb);
            } else {
               std::cout << "No active call found in the list to hold " << std::endl;
            }
         } catch(const std::exception &e) {
            std::cout << "ERROR: Exception caught -" << e.what();
         }
      } else if(cmd == "resume" || cmd == "9") {
         try {
            std::cout << "resume a call which is on hold" << input;
            std::shared_ptr<ICall> spCall = nullptr;
            // Iterate through the call list in the application and resume the
            // call which is on hold
            for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
                ++callIterator) {
               if((*callIterator)->getCallState() == CallState::CALL_ON_HOLD) {
                  spCall = *callIterator;
                  break;
               }
            }
            if(spCall == nullptr) {
               // If no Active or OnHold call is found in the current call list, get the in progress
               // call list from CallMgr
               // Resume the on-hold call
               std::vector<std::shared_ptr<ICall>> inProgressCalls
                  = phoneFactory.getCallManager()->getInProgressCalls();
               for(auto callIterator = std::begin(inProgressCalls);
                   callIterator != std::end(inProgressCalls); ++callIterator) {
                  if((*callIterator)->getCallState() == CallState::CALL_ON_HOLD) {
                     spCall = *callIterator;
                     break;
                  }
               }
            }
            if(spCall != nullptr) {
               spCall->resume(myResumeCb);
            } else {
               std::cout << "No active call found in the list to hold " << std::endl;
            }
         } catch(const std::exception &e) {
            std::cout << "ERROR: Exception caught -" << e.what();
         }
      } else if(cmd == "conference" || cmd == "7") {
         if(callList.size() < 2) {
            std::cout << "CallList does not have 2 calls to conference" << std::endl;
         }
         try {
            // Iterate through the call list find the call that is active and the first call that is
            // on hold
            // Conference both the calls
            std::shared_ptr<ICall> spCall1, spCall2;
            for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
                ++callIterator) {
               if((*callIterator)->getCallState() == CallState::CALL_ACTIVE) {
                  spCall1 = *callIterator;
                  continue;
               }

               if((*callIterator)->getCallState() == CallState::CALL_ON_HOLD) {
                  spCall2 = *callIterator;
                  continue;
               }
            }
            if(spCall1 != nullptr && spCall2 != nullptr) {
               std::cout << "Conferencing active and hold calls " << input;
               phoneFactory.getCallManager()->conference(spCall1, spCall2, myConferenceCb);
            } else {
               std::cout << "Need 1 active and 1 hold call to conference" << std::endl;
            }
         } catch(const std::exception &e) {
            std::cout << "ERROR: Exception caught -" << e.what();
         }
      } else if(cmd == "swap" || cmd == "8") {
         if(callList.size() < 2) {
            std::cout << "CallList does not have 2 calls to swap" << std::endl;
         }
         try {
            // Iterate through the call list find the call that is active and the first call that is
            // on hold
            // Swap the answer and on-hold calls
            std::shared_ptr<ICall> spCall1, spCall2;
            for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
                ++callIterator) {
               if((*callIterator)->getCallState() == CallState::CALL_ACTIVE) {
                  spCall1 = *callIterator;
                  continue;
               }

               if((*callIterator)->getCallState() == CallState::CALL_ON_HOLD) {
                  spCall2 = *callIterator;
                  continue;
               }
            }
            if(spCall1 != nullptr && spCall2 != nullptr) {
               std::cout << "Swapping active and hold calls " << input;
               phoneFactory.getCallManager()->swap(spCall1, spCall2, mySwapCb);
            } else {
               std::cout << "Need 1 active and 1 hold call to swap " << std::endl;
            }
         } catch(const std::exception &e) {
            std::cout << "ERROR: Exception caught - " << e.what();
         }
      } else if(cmd == "get_calls" || cmd == "10") {
         std::cout << "Get current calls " << std::endl;
         callList = callManager->getInProgressCalls();
         for(auto callIterator = std::begin(callList); callIterator != std::end(callList);
             ++callIterator) {
            std::cout << getCallDescription(*callIterator) << std::endl;
         }
      } else if(cmd == "ecall" || cmd == "15") {
         std::string ecallMsd;
         int emergencyCategory, ecallMsdLen, eCallVariant;
         if(args.length() > 0) {
            std::string category = std::string(args);
            category = category.erase(0, category.find_first_not_of(" \n\r\t"));
            emergencyCategory = std::stoi(category);
         } else {
            std::cout << "Invalid Input--Look Help for usage";
            break;
         }
         eCallVariant = 1;
         MsdSettings msdSettings;
         auto eCallMsdData = msdSettings.readMsdFromFile();
         auto eCallStatus = callManager->makeECall(defaultPhoneId, eCallMsdData, emergencyCategory,
                                                   eCallVariant, myECallCmdCb);
         std::cout << "Make eCall Status:" << (int)eCallStatus << std::endl;
      } else if(cmd == "update_ecall_msd" || cmd == "16") {
         MsdSettings msdSettings;
         auto eCallMsdData = msdSettings.readMsdFromFile();
         auto ret = callManager->updateECallMsd(defaultPhoneId, eCallMsdData, myUpdateMsdCmdCb);
         if(ret == Status::SUCCESS) {
            std::cout << "update_ecall_msd is successful";
         } else {
            std::cout << "update_ecall_msd failed";
         }
      } else if(cmd == "call_listener" || cmd == "17") {
         callListener(myCallListener, args);
      } else if(cmd == "request_signal_strength" || cmd == "18") {
         auto ret = spDefaultPhone->requestSignalStrength(mySignalStrengthCb);
      }

      // Card Services
      if(cmd == "get_card" || cmd == "100") {
         std::cout << "get_card :" << args << std::endl;
         if(args.length() > 0) {
            card = cardManager->getCard(std::stoi(args));
            std::cout << "get_card :" << std::stoi(args) << std::endl;
         }
      } else if(cmd == "get_card_state" || cmd == "101") {
         std::cout << "get_card_state :" << args << std::endl;
         if(card) {
            CardState cardState;
            auto status = card->getState(cardState);
            std::cout << "\ngetCardState : " << (int)cardState << std::endl;
            switch(cardState) {
               case CardState::CARDSTATE_ABSENT:
                  std::cout << "Card state Absent" << std::endl;
                  break;
               case CardState::CARDSTATE_PRESENT:
                  std::cout << "Card state Present" << std::endl;
                  break;
               case CardState::CARDSTATE_ERROR:
                  std::cout << "Card state Error or Absent" << std::endl;
                  break;
               case CardState::CARDSTATE_RESTRICTED:
                  std::cout << "Card state Restricted" << std::endl;
                  break;
               default:
                  std::cout << "Unknown Card State" << std::endl;
                  break;
            }
         }
      } else if(cmd == "get_apps" || cmd == "102") {
         if(card) {
            std::vector<std::shared_ptr<ICardApp>> applications;
            applications = card->getApplications();
            for(auto cardApp : applications) {
               std::cout << "\nAppType : " << cardApp->getAppType() << std::endl;
               std::cout << "AppState : " << cardApp->getAppState() << std::endl;
               std::cout << "AppId : " << cardApp->getAppId() << std::endl;
            }
         }

      } else if(cmd == "open_logical_channel" || cmd == "103") {
         if(card) {
            std::string aid;
            if(args.length() > 0) {
               aid = std::string(args);
               aid = aid.erase(0, aid.find_first_not_of(" \n\r\t"));
            } else {
               std::cout << "invalid aid" << std::endl;
            }
            std::cout << "open_logical_channel with aid:" << aid << std::endl;
            card->openLogicalChannel(aid, myOpenLogicalChannelCb);
         }
      } else if(cmd == "close_logical_channel" || cmd == "104") {
         if(card) {
            int channel = 0;
            if(args.length() > 0) {
               channel = std::stoi(args);
            } else {
               std::cout << "using default channel value" << std::endl;
            }
            std::cout << "close_logical_channel with channel:" << channel << std::endl;
            card->closeLogicalChannel(channel, myCloseLogicalChannelCb);
         }
      } else if(cmd == "transmit_apdu" || cmd == "105") {
         if(card) {
            int channel;
            int cla, instruction, p1, p2, p3;
            std::vector<uint8_t> data;

            cla = 0;
            instruction = 0;
            p1 = 0;
            p2 = 0;
            p3 = 0;

            std::string user_input;
            std::cout << std::endl;
            std::cout << "Enter the channel : ";
            std::cin >> channel;
            std::cout << "Enter CLA : ";
            std::cin >> cla;
            std::cout << "Enter INS : ";
            std::cin >> instruction;
            std::cout << "Enter P1 : ";
            std::cin >> p1;
            std::cout << "Enter P2 : ";
            std::cin >> p2;
            std::cout << "Enter P3 : ";
            std::cin >> p3;
            int tmpInp;
            for(int i = 0; i < p3; i++) {
               std::cout << "Enter DATA (" << i + 1 << ") :";
               std::cin >> tmpInp;
               data.emplace_back((uint8_t)tmpInp);
            }
            auto ret = card->transmitApduLogicalChannel(channel, (uint8_t)cla, (uint8_t)instruction,
                                                        (uint8_t)p1, (uint8_t)p2, (uint8_t)p3, data,
                                                        myTransmitApduCb);
            if(ret == Status::SUCCESS) {
               std::cout << "transmit_apdu is successful";
            } else {
               std::cout << "transmit_apdu failed";
            }
         }
      } else if(cmd == "basic_transmit_apdu" || cmd == "106") {
         if(card) {
            int cla, instruction, p1, p2, p3;
            std::vector<uint8_t> data;

            cla = 0;
            instruction = 0;
            p1 = 0;
            p2 = 0;
            p3 = 0;

            std::string user_input;
            std::cout << std::endl;
            std::cout << "Enter CLA : ";
            std::cin >> cla;
            std::cout << "Enter INS : ";
            std::cin >> instruction;
            std::cout << "Enter P1 : ";
            std::cin >> p1;
            std::cout << "Enter P2 : ";
            std::cin >> p2;
            std::cout << "Enter P3 : ";
            std::cin >> p3;
            int tmpInp;
            for(int i = 0; i < p3; i++) {
               std::cout << "Enter DATA (" << i + 1 << ") :";
               std::cin >> tmpInp;
               data.emplace_back((uint8_t)tmpInp);
            }
            auto ret
               = card->transmitApduBasicChannel((uint8_t)cla, (uint8_t)instruction, (uint8_t)p1,
                                                (uint8_t)p2, (uint8_t)p3, data, myTransmitApduCb);
            if(ret == Status::SUCCESS) {
               std::cout << "basic_transmit_apdu is successful";
            } else {
               std::cout << "basic_transmit_apdu failed";
            }
         }
      } else if(cmd == "card_listener" || cmd == "107") {
         cardListener(myCardListener, args);
      }

      // SAP Services
      if(cmd == "open_connection" || cmd == "151") {
         if(sapCardMgr) {
            sapCardMgr->openConnection(SapCondition::SAP_CONDITION_BLOCK_VOICE_OR_DATA,
                                       mySapCmdResponseCb);
         }
      } else if(cmd == "close_connection" || cmd == "152") {
         if(sapCardMgr) {
            sapCardMgr->closeConnection(mySapCmdResponseCb);
         }
      } else if(cmd == "get_atr" || cmd == "153") {
         if(sapCardMgr)
            sapCardMgr->requestAtr(myAtrCb);
      } else if(cmd == "get_state" || cmd == "154") {
         if(sapCardMgr) {
            SapState sapstate;
            if(sapCardMgr->getState(sapstate) == Status::SUCCESS) {
               std::cout << "get_sap_state success" << std::endl;
               logSapState(sapstate);
            } else {
               std::cout << "get_sap_state failed";
            }
         }
      } else if(cmd == "transmit_sap_apdu" || cmd == "155") {
         if(sapCardMgr) {
            int cla, instruction, p1, p2, lc, tmpInp;
            std::vector<uint8_t> data;

            cla = 0;
            instruction = 0;
            p1 = 0;
            p2 = 0;
            lc = 0;

            std::string user_input;
            std::cout << std::endl;
            std::cout << "Enter CLA : ";
            std::cin >> cla;
            std::cout << "Enter INS : ";
            std::cin >> instruction;
            std::cout << "Enter P1 : ";
            std::cin >> p1;
            std::cout << "Enter P2 : ";
            std::cin >> p2;
            std::cout << "Enter Lc : ";
            std::cin >> lc;
            for(int i = 0; i < lc; i++) {
               std::cout << "Enter DATA (" << i + 1 << ") :";
               std::cin >> tmpInp;
               data.emplace_back((uint8_t)tmpInp);
            }
            auto ret = sapCardMgr->transmitApdu((uint8_t)cla, (uint8_t)instruction, (uint8_t)p1,
                                                (uint8_t)p2, (uint8_t)lc, data, 0,
                                                myTransmitApduResponseCb);
            if(ret == Status::SUCCESS) {
               std::cout << "Sending of sap_transmit_apdu command is successful";
            } else {
               std::cout
                  << "Unable to send sap_transmit_apdu command, status : " << static_cast<int>(ret)
                  << std::endl;
            }
         }
      } else if(cmd == "sim_power_off" || cmd == "156") {
         if(sapCardMgr) {
            sapCardMgr->requestSimPowerOff(mySapCmdResponseCb);
         }
      } else if(cmd == "sim_power_on" || cmd == "157") {
         if(sapCardMgr) {
            sapCardMgr->requestSimPowerOn(mySapCmdResponseCb);
         }
      } else if(cmd == "sim_reset" || cmd == "158") {
         if(sapCardMgr) {
            sapCardMgr->requestSimReset(mySapCmdResponseCb);
         }
      } else if(cmd == "get_card_reader_status" || cmd == "159") {
         if(sapCardMgr) {
            sapCardMgr->requestCardReaderStatus(myCardReaderCb);
         }
      }

      if(cmd == "listener" || cmd == "l") {  // Listener
         std::cout << "listener";
         listener(myPhListener, args);
      } else if(cmd == "help" || cmd == "?") {
         telSDKMenu();
      } else if(cmd == "quit" || cmd == "exit" || cmd == "0" || cmd == "q") {
         std::cout << "exit" << std::endl;
         is_alive = false;
         return 0;
      }
   }
}
