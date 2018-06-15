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
 * Sample program to demonstrate SAP Card Services APIs like get slot ids, getApplications,
 * get ATR, transmit APDU and listeners
 */

#include <chrono>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include <telux/tel/CardDefines.hpp>
#include <telux/tel/SapCardManager.hpp>
#include <telux/tel/PhoneFactory.hpp>

#define DEFAULT_TIMEOUT_IN_SECONDS 5

// Sample SAP APDU to open Master File
// APDU Command - 00 A4 00 04 02 3F 00
const uint8_t CLA = 0;
const uint8_t INSTRUCTION = 164;
const uint8_t P1 = 0;
const uint8_t P2 = 4;
const uint8_t LC = 2;
const std::vector<uint8_t> DATA = {63, 0};

using namespace telux::tel;
using namespace telux::common;

// [1] Define sap enum for events
enum SapEvent {
   OPEN_SAP_CONNECTION = 1,  /**<  SAP Open connection*/
   CLOSE_SAP_CONNECTION = 2, /**<  SAP disconnection*/
   SAP_GET_ATR = 3,          /**<  SAP Answer To Reset*/
   SAP_TRANSMIT_APDU = 4,    /**<  Transmit of APDU in SAP mode*/
};

// condition variable to wait for an sap events like open connection, close connection,
// request ATR and transmit APDU
std::condition_variable eventCV;

// variable to store the expected sap event
SapEvent eventExpected;

// Protects expected sap events to avoid access from different threads
std::mutex eventMutex;

// Error code received as part of notification
ErrorCode sapErrorCode;

// [4.1] - Implementation of ICommandResponseCallback interface for receiving notifications on sap
// events like open connection and close connection
class MySapCommandResponseCallback : public ICommandResponseCallback {
public:
   void commandResponse(ErrorCode error);
};

void MySapCommandResponseCallback::commandResponse(ErrorCode error) {
   std::cout << "commandResponse, error: " << (int)error << std::endl;
   std::unique_lock<std::mutex> lock(eventMutex);
   sapErrorCode = error;
   if(eventExpected == (int)SapEvent::OPEN_SAP_CONNECTION) {
      std::cout << "Sap Event OPEN_SAP_CONNECTION found with code :" << int(error) << std::endl;
      eventCV.notify_one();
   } else if(eventExpected == (int)SapEvent::CLOSE_SAP_CONNECTION) {
      std::cout << "Sap Event CLOSE_SAP_CONNECTION found with code :" << int(error) << std::endl;
      eventCV.notify_one();
   }
}

// [4.2] - Implementation of IAtrResponseCallback interface for receiving notification on sap
// event like request answer to reset(ATR)
class MyAtrResponseCallback : public IAtrResponseCallback {
public:
   void atrResponse(std::vector<int> responseAtr, ErrorCode error);
};

void MyAtrResponseCallback::atrResponse(std::vector<int> responseAtr, ErrorCode error) {
   std::cout << "atrResponse, error: " << (int)error << std::endl;
   std::unique_lock<std::mutex> lock(eventMutex);
   sapErrorCode = error;
   std::cout << "\tATR.data:";
   if(eventExpected == (int)SapEvent::SAP_GET_ATR) {
      for(int val : responseAtr) {
         std::cout << " " << val;
      }
      std::cout << "Event SAP_GET_ATR found with code :" << int(sapErrorCode) << std::endl;
      eventCV.notify_one();
   }
}

// [4.3] - Implementation of ISapCardCommandCallback interface for receiving notification
// on sap event like transmit apdu.
class MySapTransmitApduResponseCallback : public ISapCardCommandCallback {
public:
   void onResponse(IccResult result, ErrorCode error);
};

void MySapTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
   std::cout << "transmitApduResponse, error: " << (int)error << std::endl;
   std::unique_lock<std::mutex> lock(eventMutex);
   sapErrorCode = error;
   std::cout << "transmitApduResponse " << result.toString() << std::endl;
   if(eventExpected == (int)SapEvent::SAP_TRANSMIT_APDU) {
      std::cout << "Transmit Apdu SAP_TRANSMIT_APDU Event Found with code :" << int(sapErrorCode)
                << std::endl;
      eventCV.notify_one();
   }
}

// We are making a synchronized SAP requests. So added wait logic using std::condition_variable
bool waitForSapEvent(SapEvent sapEvent, int timeout = DEFAULT_TIMEOUT_IN_SECONDS) {
   std::unique_lock<std::mutex> lock(eventMutex);
   eventExpected = sapEvent;
   auto cvStatus = eventCV.wait_for(lock, std::chrono::seconds(DEFAULT_TIMEOUT_IN_SECONDS));
   if(cvStatus == std::cv_status::timeout) {
      std::cout << "Event: " << (int)sapEvent << "not found with in " << DEFAULT_TIMEOUT_IN_SECONDS
                << "second(s)";
   }
   eventExpected = (SapEvent)0;  // reset message id to avoid further notifications
   if(cvStatus != std::cv_status::timeout) {
      if(sapEvent == SapEvent::OPEN_SAP_CONNECTION || sapEvent == SapEvent::CLOSE_SAP_CONNECTION
         || sapEvent == SapEvent::SAP_GET_ATR || sapEvent == SapEvent::SAP_TRANSMIT_APDU) {

         if(sapErrorCode == ErrorCode::SUCCESS)
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
   // [1] Get the PhoneFactory and PhoneManager instances.
   auto &phoneFactory = PhoneFactory::getInstance();
   auto phoneManager = phoneFactory.getPhoneManager();

   // [2] Wait for the telephony subsystem initialization.
   bool subSystemsStatus = phoneManager->isSubsystemReady();
   std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
   startTime = std::chrono::system_clock::now();

   if(!subSystemsStatus) {
      std::cout << "Telephony subsystem is not ready, wait for it to be ready " << std::endl;
      std::future<bool> f = phoneManager->onSubsystemReady();
      subSystemsStatus = f.get();
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

   // [3] Get default Sap Card Manager instance
   std::shared_ptr<ISapCardManager> sapCardMgr = phoneFactory.getSapCardManager();

   // [4] Instantiate ICommandResponseCallback, IAtrResponseCallback and
   // ISapCardCommandCallback
   auto mySapCmdResponseCb = std::make_shared<MySapCommandResponseCallback>();
   auto myAtrCb = std::make_shared<MyAtrResponseCallback>();
   auto myTransmitApduResponseCb = std::make_shared<MySapTransmitApduResponseCallback>();

   // [5] Open Sap connection and wait for request to complete
   sapCardMgr->openConnection(SapCondition::SAP_CONDITION_BLOCK_VOICE_OR_DATA, mySapCmdResponseCb);
   std::cout << "Opening SAP connection to Transmit the APDU..." << std::endl;
   if(!waitForSapEvent(SapEvent::OPEN_SAP_CONNECTION)) {
      std::cout << "Opening SAP connection failed " << std::endl;
      exit(1);
   }

   // [6] request sap ATR and wait for complete
   sapCardMgr->requestAtr(myAtrCb);
   if(!waitForSapEvent(SapEvent::SAP_GET_ATR)) {
      std::cout << "get SAP ATR  failed " << std::endl;
      exit(1);
   }

   // [7] send sap apdu and wait for the request to complete
   std::cout << "Transmit Sap APDU request made..." << std::endl;
   sapCardMgr->transmitApdu(CLA, INSTRUCTION, P1, P2, LC, DATA, 0, myTransmitApduResponseCb);
   if(!waitForSapEvent(SapEvent::SAP_TRANSMIT_APDU)) {
      std::cout << "Transmit Sap APDU failed " << std::endl;
      exit(1);
   }

   // [8] close sap connection and wait for the request to complete
   sapCardMgr->closeConnection(mySapCmdResponseCb);
   if(waitForSapEvent(SapEvent::CLOSE_SAP_CONNECTION)) {
      std::cout << "Close Sap Successful..." << std::endl;
   }

   return 0;
}
