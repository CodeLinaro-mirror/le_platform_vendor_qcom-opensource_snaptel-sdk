/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <iostream>
#include <sstream>
#include <thread>

extern "C" {
#include <sys/time.h>
}

#include "AecsSmsListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void AecsSmsCommandCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "sendSmsResponse successfully" << std::endl;
   } else {
      PRINT_CB << "sendSmsResponse failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

// Implementation of Aecs SMS callback
void AecsSmsCommandCallback::sendSmsResponse(std::vector<int> msgRefs,
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "sendSmsResponse successfully" << std::endl;
      PRINT_CB << " MsgRefs Size: "<< msgRefs.size() << std::endl;
      for (int i: msgRefs) {
         PRINT_CB << " MsgRef : " << i << std::endl;
      }
   } else {
      PRINT_CB << "sendSmsResponse failed, errorCode: " << static_cast<int>(error)
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void AecsSmsDeliveryCallback::commandResponse(telux::common::ErrorCode error) {
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "SMS Delivered successfully" << std::endl;
   } else {
      PRINT_CB << "SMS Delivery failed, errorCode: " << (int)error
               << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
   }
}

void AecsSmsListener::onIncomingSms(int phoneId, std::shared_ptr<telux::tel::SmsMessage> smsMsg) {
   std::cout << std::endl << std::endl;
   std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg->getMessagePartInfo();
   if (partInfo) {
      PRINT_NOTIFICATION << "Received SMS on phone ID " << phoneId << " from: "
            << smsMsg->getSender() <<  " to: " << smsMsg->getReceiver()
            << "\n Message: " << smsMsg->getText() << "\n PDU: " << smsMsg->getPdu()
            << " \n RefNumber:" << static_cast <int>(partInfo->refNumber) << " NumberOfSegments:"
            << static_cast <int>(partInfo->numberOfSegments) << " SegmentNumber: "
            << static_cast <int>(partInfo->segmentNumber)
            << std::endl;
   } else {
      PRINT_NOTIFICATION << "Received SMS on phone ID " << phoneId << " from: "
            << smsMsg->getSender() <<  " to: " << smsMsg->getReceiver()
            << "\n Message: " << smsMsg->getText() << "\n PDU: " << smsMsg->getPdu()
            << std::endl;
   }
}

void AecsSmsListener::onIncomingSms(int phoneId,
   std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs) {
   std::cout << std::endl;

   std::string text = "";
   std::vector<telux::tel::SmsMessage> messages = *(msgs.get());
   if (messages.size() > 1) {
      PRINT_NOTIFICATION << " Consolidated Multipart Message: " << std::endl;
      PRINT_NOTIFICATION << " Count :" << messages.size() << std::endl;
   } else {
      PRINT_NOTIFICATION << " Message: " << std::endl;
      PRINT_NOTIFICATION << " Count :" << messages.size() << std::endl;
   }
   for (telux::tel::SmsMessage smsMsg : messages) {
      text = text + smsMsg.getText();
      std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg.getMessagePartInfo();
      if (partInfo) {
         std::cout << "\033[1;35mSegment: \033[0m" << static_cast<int>(partInfo->segmentNumber)
                << "\n SMS Part on phone ID " << phoneId << " from: "
                << smsMsg.getSender() <<  " to: " << smsMsg.getReceiver()
                << "\n Message Part: " << smsMsg.getText() << "\n PDU: " << smsMsg.getPdu()
                << "\n RefNumber:" << static_cast <int>(partInfo->refNumber)
                << " NumberOfSegments:"
                << static_cast <int>(partInfo->numberOfSegments) << " SegmentNumber: "
                << static_cast <int>(partInfo->segmentNumber) << std::endl;
      }
   }
   std::cout << "\033[1;35mComplete Message: \033[0m" <<  "\n" << text << std::endl;
}

void AecsSmsListener::onDeliveryReport(int phoneId, int msgRef, std::string receiverAddress,
   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   PRINT_NOTIFICATION << "Received delivery report from phone ID " << phoneId << " with MsgRef: "
                      << msgRef << " Receiver Address: "<< receiverAddress <<" Error Desc: "
                      << Utils::getErrorCodeAsString(error) << std::endl;
   if (error == telux::common::ErrorCode::SUCCESS) {
       PRINT_NOTIFICATION << "AECS information: data transmission completed" << std::endl;
   } else {
       PRINT_NOTIFICATION << "AECS information: data transmission failed" << std::endl;
       std::cout << std::endl << std::endl;
       std::cout << "Enter 9 to retry MSD transmission over SMS" << std::endl;
   }
}
