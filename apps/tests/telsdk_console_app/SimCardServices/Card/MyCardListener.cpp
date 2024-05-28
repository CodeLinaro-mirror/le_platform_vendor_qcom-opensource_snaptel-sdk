/*
 *  Copyright (c) 2018,2021 The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center, Inc. are provided under the following license:
 *
 *  Copyright (c) 2023-2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#include <iostream>

#include "MyCardListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

/**
 *  Implementation of MyOpenLogicalChannelCallback
 */
void MyOpenLogicalChannelCallback::onChannelResponse(int channel, telux::tel::IccResult result,
                                                     telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "onChannelResponse successful, channel: " << channel << "\n iccResult "
                         << result.toString() << std::endl;
   } else {
      PRINT_CB << "onChannelResponse failed\n error: " << static_cast<int>(error)
                         << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

/**
 *  Implementation of MyCardCommandResponseCallback
 */
void MyCardCommandResponseCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "onCloseLogicalChannel successful." << std::endl;
   } else {
      PRINT_CB << "onCloseLogicalChannel failed\n error: " << static_cast<int>(error)
                         << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

/**
 *  Implementation of MyCardPowerResponseCallback
 */
void MyCardPowerResponseCallback::cardPowerUpResp(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Card power up request is successful \n";
   } else {
      PRINT_CB << "Card power up request failed error: " << static_cast<int>(error)
               << ", \ndescription: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}

void MyCardPowerResponseCallback::cardPowerDownResp(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Card power down request is successful \n";
   } else {
      PRINT_CB << "Card power down request failed error: " << static_cast<int>(error)
               << ", Description: " << Utils::getErrorCodeAsString(error) << "\n";
   }
}

/**
 *  Implementation of MyTransmitApduResponseCallback
 */
void MyTransmitApduResponseCallback::onResponse(telux::tel::IccResult result,
                                                telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "onResponse successful, " << result.toString() << std::endl
                         << std::endl;
   } else {
      PRINT_CB << "onResponse failed\n error: " << static_cast<int>(error)
                         << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

/**
 *  Implementation of MyCardListener
 */
void MyCardListener::onCardInfoChanged(int slotId) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "\tSlotId :" << slotId << std::endl;
   std::promise<telux::common::ServiceStatus> cardMgrprom;
   auto cardMgr = telux::tel::PhoneFactory::getInstance().
       getCardManager([&](telux::common::ServiceStatus status) {
       cardMgrprom.set_value(status);
   });

   if (!cardMgr) {
       std::cout << "Failed to get CardManager instance \n";
       return;
   }
   telux::common::ServiceStatus cardMgrStatus = cardMgr->getServiceStatus();
   cardMgrStatus = cardMgrprom.get_future().get();
   // CardState cardState = cardMgr->getCardState(slotId);
   if (cardMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
       telux::tel::CardState cardState;
       telux::common::Status status;
       auto card = cardMgr->getCard(slotId, &status);
       if (status != telux::common::Status::SUCCESS) {
           PRINT_NOTIFICATION << "\tCouldn't get get Card details" << std::endl;
           return;
       }
       card->getState(cardState);
       PRINT_NOTIFICATION << "\tCardState:" << (int)cardState << std::endl;
       switch(cardState) {
          case telux::tel::CardState::CARDSTATE_ABSENT:
             PRINT_NOTIFICATION << "Card State is Absent" << std::endl;
             break;
          case telux::tel::CardState::CARDSTATE_PRESENT:
             PRINT_NOTIFICATION << "Card State is  Present" << std::endl;
             break;
          case telux::tel::CardState::CARDSTATE_ERROR:
             PRINT_NOTIFICATION << "Card State is either Error or Absent" << std::endl;
             break;
          case telux::tel::CardState::CARDSTATE_RESTRICTED:
             PRINT_NOTIFICATION << "Card State is Restricted" << std::endl;
             break;
          default:
             PRINT_NOTIFICATION << "Unknown Card State" << std::endl;
             break;
       }
   } else {
       PRINT_NOTIFICATION <<
           " Card Manager subsystem is not ready, failed to notify card state change" << std::endl;
   }
}

// Notify CardManager subsystem status
void MyCardListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat = "";
    switch(status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat =  " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " Card onServiceStatusChange" << stat << "\n";
}
