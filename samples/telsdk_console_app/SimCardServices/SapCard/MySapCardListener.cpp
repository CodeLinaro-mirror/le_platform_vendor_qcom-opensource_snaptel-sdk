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

#include "MySapCardListener.hpp"

#define print_cb std::cout << "\033[1;35mCALLBACK: \033[0m"

/**
 *  Implementation of MySapCommandCallback
 */
void MySapCommandResponseCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << " commandResponse successful." << std::endl;
   } else {
      print_cb << "commandResponse failed\n error: " << static_cast<int>(error) << std::endl;
   }
}

/**
 *  Implementation of MyCardReaderCallback
 */
void MyCardReaderCallback::cardReaderResponse(telux::tel::CardReaderStatus readerStatus,
                                              telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;

   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "cardReaderResponse successful\n";
      print_cb << " CardReaderStatus, id = " << readerStatus.id
               << "\n isRemovable = " << readerStatus.isRemovable
               << "\n isPresent = " << readerStatus.isPresent
               << "\n isID1size = " << readerStatus.isID1size
               << "\n isCardPresent = " << readerStatus.isCardPresent
               << "\n isCardPoweredOn = " << readerStatus.isCardPoweredOn << "\n";
   } else {
      print_cb << "cardReaderResponse failed\n error: " << static_cast<int>(error) << std::endl;
   }
}

/**
 *  Implementation of MySapTransmitApduResponseCallback
 */
void MySapTransmitApduResponseCallback::onResponse(telux::tel::IccResult result,
                                                   telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "onResponse successful, " << result.toString() << std::endl << std::endl;
   } else {
      print_cb << "onResponse failed\n error: " << static_cast<int>(error) << std::endl;
   }
}

/**
 *  Implementation of MyAtrResponseCallback
 */
void MyAtrResponseCallback::atrResponse(std::vector<int> responseAtr,
                                        telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "atrResponse successful\n";
      print_cb << "\tATR.data:";
      for(int val : responseAtr) {
         std::cout << " " << val;
      }
   } else {
      print_cb << "atrResponse failed\n error: " << static_cast<int>(error) << std::endl;
   }
   std::cout << std::endl;
}