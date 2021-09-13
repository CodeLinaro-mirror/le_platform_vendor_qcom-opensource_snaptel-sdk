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
 * Sample program to demonstrate Card Services APIs like get slot ids, getApplications,
 * transmit APDU and listeners
 */

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <telux/tel/CardApp.hpp>
#include <telux/tel/CardDefines.hpp>
#include <telux/tel/CardManager.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/common/CommonDefines.hpp>

#define DEFAULT_TIMEOUT_IN_SECONDS 5

/// Sample SAP APDU to open Master File
// APDU Command - 00 A4 00 04 02 3F 00
const uint8_t CLA = 0;
const uint8_t INSTRUCTION = 164;
const uint8_t P1 = 0;
const uint8_t P2 = 4;
const uint8_t P3 = 2;
const std::vector<uint8_t> DATA = {63, 0};

using namespace telux::tel;
using namespace telux::common;

// Define enum for Card Events
enum class CardEvent {
   OPEN_LOGICAL_CHANNEL = 1,  /**<  Open Logical channel */
   CLOSE_LOGICAL_CHANNEL = 2, /**<  Close Logical channel*/
   TRANSMIT_APDU_CHANNEL = 3, /**<  Transmit of APDU on channel*/
};

// condition variable to wait for an card services like open logical channel, close logical channel,
// transmit APDU
std::condition_variable eventCV;

// variable to store the expected card event
CardEvent cardEventExpected;

// Protects expected card events to avoid access from different threads
std::mutex eventMutex;

// Error code received as part of notification
ErrorCode errorCode;

// channel defined to transmit apdu over it
int openChannel = -1;

// [5.1] Implementation of ICardChannelCallback interface for receiving notifications on card event
// like open logical channel
class MyOpenLogicalChannelCallback : public ICardChannelCallback {
public:
   void onChannelResponse(int channel, IccResult result, ErrorCode error) override;
};

void MyOpenLogicalChannelCallback::onChannelResponse(int channel, IccResult result,
                                                     ErrorCode error) {
   std::cout << "onChannelResponse, error: " << (int)error << std::endl;
   std::unique_lock<std::mutex> lock(eventMutex);
   errorCode = error;
   openChannel = channel;
   std::cout << "onChannelResponse: " << result.toString() << std::endl;
   if(cardEventExpected == CardEvent::OPEN_LOGICAL_CHANNEL) {
      std::cout << "Card Event OPEN_LOGICAL_CHANNEL found with code :" << int(error) << std::endl;
      eventCV.notify_one();
   }
}

// [5.2] Implementation of ICommandResponseCallback interface for receiving notifications on card
// event like close logical channel
class MyCloseLogicalChannelCallback : public ICommandResponseCallback {
public:
   void commandResponse(ErrorCode error) override;
};

void MyCloseLogicalChannelCallback::commandResponse(ErrorCode error) {
   std::cout << "commandResponse, error: " << (int)error << std::endl;
   std::unique_lock<std::mutex> lock(eventMutex);
   errorCode = error;
   if(cardEventExpected == CardEvent::CLOSE_LOGICAL_CHANNEL) {
      std::cout << "Card Event CLOSE_LOGICAL_CHANNEL found with code :" << int(error) << std::endl;
      eventCV.notify_one();
   }
}

// [5.3] - Implementation of ICardCommandCallback interface for receiving notifications on card
// event like transmit apdu logical channel and transmit apdu basic channel
class MyTransmitApduResponseCallback : public ICardCommandCallback {
public:
   void onResponse(IccResult result, ErrorCode error) override;
};

void MyTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
   std::cout << "onResponse, error: " << (int)error << std::endl;
   std::unique_lock<std::mutex> lock(eventMutex);
   errorCode = error;
   std::cout << "onResponse:  " << result.toString() << std::endl;
   if(cardEventExpected == CardEvent::TRANSMIT_APDU_CHANNEL) {
      std::cout << "Card Event TRANSMIT_APDU_CHANNEL found with code :" << int(error) << std::endl;
      eventCV.notify_one();
   }
}

// We are making a synchronized card requests. So added wait logic using
// std::condition_variable
bool waitForCardEvent(CardEvent cardEvent, int timeout = DEFAULT_TIMEOUT_IN_SECONDS) {
   std::unique_lock<std::mutex> lock(eventMutex);
   cardEventExpected = cardEvent;
   auto cvStatus = eventCV.wait_for(lock, std::chrono::seconds(DEFAULT_TIMEOUT_IN_SECONDS));
   if(cvStatus == std::cv_status::timeout) {
      std::cout << "Event: " << (int)cardEvent << "not found with in " << DEFAULT_TIMEOUT_IN_SECONDS
                << "second(s)";
   }
   cardEventExpected = (CardEvent)0;  // reset message id to avoid further notifications
   if(cvStatus != std::cv_status::timeout) {
      if(cardEvent == CardEvent::OPEN_LOGICAL_CHANNEL
         || cardEvent == CardEvent::CLOSE_LOGICAL_CHANNEL
         || cardEvent == CardEvent::TRANSMIT_APDU_CHANNEL) {

         if(errorCode == ErrorCode::SUCCESS)
            return true;
      }
   } else {
      std::cout << "Unable to get the events, so timing out" << std::endl;
      return false;
   }
   return false;
}

// Main routine performs operations required to transmit Sap Apdu
int main(int, char **) {
   // [1] Get the PhoneFactory and CardManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   std::shared_ptr<ICardManager> cardManager = phoneFactory.getCardManager();

   // [2] Wait for the telephony subsystem initialization.
   bool subSystemsStatus = cardManager->isSubsystemReady();
   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();

   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready, wait for it to be ready " << std::endl;
      std::future<bool> f = cardManager->onSubsystemReady();
      auto status = f.wait_for(std::chrono::seconds(5));
      if(status == std::future_status::ready) {
         subSystemsStatus = true;
      }
   }

   if(subSystemsStatus) {
      endTime = std::chrono::system_clock::now();
      std::chrono::duration<double> elapsedTime = endTime - startTime;
      std::cout << "\nElapsed Time for Subsystems to ready : " << elapsedTime.count() << "s\n"
                << std::endl;
   } else {
      std::cout << " *** ERROR - Unable to initialize telephony subsystem" << std::endl;
      return 1;
   }

   // [3] Get SlotCount, SlotIds and Card instance
   int slotCount;
   cardManager->getSlotCount(slotCount);
   std::cout << "Slots Count is :" << slotCount << std::endl;
   std::vector<int> slotIds;
   cardManager->getSlotIds(slotIds);
   std::cout << "Slot Ids are : { ";
   for(auto id : slotIds) {
      std::cout << id << " ";
   }
   std::cout << "}" << std::endl;
   std::shared_ptr<ICard> cardImpl = cardManager->getCard(slotIds.front());

   // [4] Get supported applications from the card
   std::vector<std::shared_ptr<ICardApp>> applications;
   if(cardImpl) {
      std::cout << "\nApplications available are : " << std::endl;
      applications = cardImpl->getApplications();
      for(auto cardApp : applications) {
         std::cout << "AppId : " << cardApp->getAppId() << std::endl;
      }
   }

   // [5] Instantiate optional IOpenLogicalChannelCallback, ICommandResponseCallback and
   // ITransmitApduResponseCallback
   auto myOpenLogicalCb = std::make_shared<MyOpenLogicalChannelCallback>();
   auto myCloseLogicalCb = std::make_shared<MyCloseLogicalChannelCallback>();
   auto myTransmitApduResponseCb = std::make_shared<MyTransmitApduResponseCallback>();

   // [6] Open Logical Channel and wait for request to complete
   std::string aid;
   for(auto app : applications) {
      if(app->getAppType() == APPTYPE_USIM) {
         aid = app->getAppId();
         break;
      }
   }
   cardImpl->openLogicalChannel(aid, myOpenLogicalCb);
   std::cout << "Opening Logical Channel to Transmit the APDU..." << std::endl;

   if(!waitForCardEvent(CardEvent::OPEN_LOGICAL_CHANNEL)) {
      std::cout << "Opening Logical Channel failed " << std::endl;
      exit(1);
   }

   // [7] Transmit Apdu on Logical Channel, wait for request to complete
   cardImpl->transmitApduLogicalChannel(openChannel, CLA, INSTRUCTION, P1, P2, P3, DATA,
                                        myTransmitApduResponseCb);
   std::cout << "Transmit APDU request made..." << std::endl;

   if(!waitForCardEvent(CardEvent::TRANSMIT_APDU_CHANNEL)) {
      std::cout << "Transmit APDU failed " << std::endl;
      exit(1);
   }

   // [8] Close the opened logical channel and wait for the completion
   cardImpl->closeLogicalChannel(openChannel, myCloseLogicalCb);
   std::cout << "Close the Logical Channel..." << std::endl;
   if(waitForCardEvent(CardEvent::CLOSE_LOGICAL_CHANNEL)) {
      std::cout << "Close Logical Channel Successful..." << std::endl;
   }

   // [9] Transmit Apdu on Basic Channel and wait for completion
   cardImpl->transmitApduBasicChannel(CLA, INSTRUCTION, P1, P2, P3, DATA, myTransmitApduResponseCb);
   std::cout << "Transmit APDU request on Basic channel made..." << std::endl;

   if(!waitForCardEvent(CardEvent::TRANSMIT_APDU_CHANNEL)) {
      std::cout << "Transmit APDU failed " << std::endl;
      exit(1);
   }

   return 0;
}