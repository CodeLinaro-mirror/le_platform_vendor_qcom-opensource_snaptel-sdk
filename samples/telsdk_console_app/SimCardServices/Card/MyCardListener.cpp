/*
 *  Copyright (c) 2018, The Linux Foundation. All rights reserved.
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

#include "MyCardListener.hpp"

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"

/**
 *  Implementation of MyOpenLogicalChannelCallback
 */
void MyOpenLogicalChannelCallback::onChannelResponse(int channel, telux::tel::IccResult result,
                                                     telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_notification << "onChannelResponse successful, channel: " << channel << "\n iccResult "
                         << result.toString() << std::endl;
   } else {
      print_notification << "onChannelResponse failed\n error: " << static_cast<int>(error)
                         << std::endl;
   }
}

/**
 *  Implementation of MyCardCommandResponseCallback
 */
void MyCardCommandResponseCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_notification << "onCloseLogicalChannel successful." << std::endl;
   } else {
      print_notification << "onCloseLogicalChannel failed\n error: " << static_cast<int>(error)
                         << std::endl;
   }
}

/**
 *  Implementation of MyTransmitApduResponseCallback
 */
void MyTransmitApduResponseCallback::onResponse(telux::tel::IccResult result,
                                                telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_notification << "onResponse successful, " << result.toString() << std::endl
                         << std::endl;
   } else {
      print_notification << "onResponse failed\n error: " << static_cast<int>(error) << std::endl;
   }
}

/**
 *  Implementation of MyCardListener
 */
void MyCardListener::onCardInfoChanged(int slotId) {
   std::cout << std::endl << std::endl;
   print_notification << "onCardInfoChange\n\t" << std::endl;
   print_notification << "\tSlotId :" << slotId << std::endl;
   auto cardMgr = telux::tel::PhoneFactory::getInstance().getCardManager();
   // CardState cardState = cardMgr->getCardState(slotId);
   telux::tel::CardState cardState;
   cardMgr->getCard(slotId)->getState(cardState);
   print_notification << "\tCardState:" << (int)cardState << std::endl;
   switch(cardState) {
      case telux::tel::CardState::CARDSTATE_ABSENT:
         print_notification << "Card State is Absent" << std::endl;
         break;
      case telux::tel::CardState::CARDSTATE_PRESENT:
         print_notification << "Card State is  Present" << std::endl;
         break;
      case telux::tel::CardState::CARDSTATE_ERROR:
         print_notification << "Card State is either Error or Absent" << std::endl;
         break;
      case telux::tel::CardState::CARDSTATE_RESTRICTED:
         print_notification << "Card State is Restricted" << std::endl;
         break;
      default:
         print_notification << "Unknown Card State" << std::endl;
         break;
   }
}
