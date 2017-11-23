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

#include <sstream>
#include <iostream>

#include "MyCardHandler.hpp"

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::tel;
using namespace telux::common;

static bool GetSimStatus(false);
static bool IccIoResponse(false);

uint16_t iccResultCode_;
ErrorCode status_;

/**
 *  Implementation of MyOpenLogicalChannelCallback
 */
void MyOpenLogicalChannelCallback::onChannelResponse(int channel, IccResult result,
                                                     ErrorCode error) {
   std::cout << "onChannelResponse\n\terror: " << (int)error << std::endl;
   print_notification << "onChannelResponse: channel: " << channel << ", iccResult "
                      << result.toString() << std::endl;
}

/**
 *  Implementation of MyCardCommandResponseCallback
 */
void MyCardCommandResponseCallback::commandResponse(ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == ErrorCode::SUCCESS) {
      print_notification << "onCloseLogicalChannel successful." << std::endl;
   } else {
      print_notification << "onCloseLogicalChannel\n\terror: " << (int)error << std::endl;
   }
}

/**
 *  Implementation of MySapCommandCallback
 */
void MySapCommandResponseCallback::commandResponse(ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == ErrorCode::SUCCESS) {
      print_notification << " commandResponse successful." << std::endl;
   } else {
      print_notification << "commandResponse\n\terror: " << (int)error << std::endl;
   }
}

/**
 *  Implementation of MySapTransmitApduResponseCallback
 */
void MySapTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
   std::cout << std::endl << std::endl;
   iccResultCode_ = result.sw2;
   std::cout << "onResponse: error: " << (int)error << std::endl;
   print_notification << "onResponse: " << result.toString() << std::endl << std::endl;
}

/**
 *  Implementation of MyAtrResponseCallback
 */
void MyAtrResponseCallback::atrResponse(std::vector<int> responseAtr, ErrorCode error) {
   std::cout << std::endl << std::endl;
   print_notification << "atrResponse\n\terror: " << (int)error << std::endl;
   print_notification << "\tATR.data:";
   for(int val : responseAtr) {
      std::cout << " " << val;
   }
   std::cout << std::endl;
}

/**
 *  Implementation of MyTransmitApduResponseCallback
 */
void MyTransmitApduResponseCallback::onResponse(IccResult result, ErrorCode error) {
   std::cout << std::endl << std::endl;
   iccResultCode_ = result.sw2;
   std::cout << "onResponse\n\terror: " << (int)error << std::endl;
   print_notification << "onResponse: " << result.toString() << std::endl << std::endl;
}

/**
 *  Implementation of MyCardReaderCallback
 */
void MyCardReaderCallback::cardReaderResponse(CardReaderStatus readerStatus, ErrorCode error) {
   std::cout << std::endl << std::endl;
   print_notification << "onCardReaderStatus\n\terror: " << (int)error << std::endl;

   print_notification << " CardReaderStatus, id = " << readerStatus.id
                      << "\n isRemovable = " << readerStatus.isRemovable
                      << "\n isPresent = " << readerStatus.isPresent
                      << "\n isID1size = " << readerStatus.isID1size
                      << "\n isCardPresent = " << readerStatus.isCardPresent
                      << "\n isCardPoweredOn = " << readerStatus.isCardPoweredOn << "\n";
}

/**
 *  Implementation of MyCardListener
 */
void MyCardListener::onCardInfoChanged(int slotId) {
   std::cout << std::endl << std::endl;
   print_notification << "onCardInfoChange\n\t" << std::endl;
   print_notification << "\tSlotId :" << slotId << std::endl;
   auto cardMgr = PhoneFactory::getInstance().getCardManager();
   // CardState cardState = cardMgr->getCardState(slotId);
   CardState cardState;
   auto status = cardMgr->getCard(slotId)->getState(cardState);
   print_notification << "\tCardState:" << (int)cardState << std::endl;
   switch(cardState) {
      case CardState::CARDSTATE_ABSENT:
         print_notification << "Cardstate Absent" << std::endl;
         break;
      case CardState::CARDSTATE_PRESENT:
         print_notification << "Cardstate Present" << std::endl;
         break;
      case CardState::CARDSTATE_ERROR:
         print_notification << "Cardstate Error or Absent" << std::endl;
         break;
      case CardState::CARDSTATE_RESTRICTED:
         print_notification << "Cardstate Restricted" << std::endl;
         break;
      default:
         print_notification << "Unknown Card State" << std::endl;
         break;
   }
}
