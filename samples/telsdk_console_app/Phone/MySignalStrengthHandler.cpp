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

#include "iostream"
#include "MySignalStrengthHandler.hpp"

#define PRINT_CALLBACK std::cout << "\033[1;35mCALLBACK: \033[0m"

using namespace telux::tel;
using namespace telux::common;

MySignalStrengthCallback::MySignalStrengthCallback() {
}

void MySignalStrengthCallback::signalStrengthResponse(
   std::shared_ptr<SignalStrength> signalStrength, ErrorCode error) {
   std::cout << std::endl << std::endl;
   PRINT_CALLBACK << "Received Signal Strength Callback with Error Code:" << (int)error
                      << std::endl;
   if(signalStrength->getGsmSignalStrength() != nullptr) {
      PRINT_CALLBACK
         << "GsmSignalStrength: " << signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
         << std::endl;
      PRINT_CALLBACK
         << "GsmBitErrorRate: " << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
         << std::endl;
      PRINT_CALLBACK << "GsmDbm: " << signalStrength->getGsmSignalStrength()->getDbm()
                         << std::endl;
      PRINT_CALLBACK
         << "Gsm Signal Level: " << (int)signalStrength->getGsmSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getCdmaSignalStrength() != nullptr) {
      PRINT_CALLBACK << "Cdma/Evdo Dbm: " << signalStrength->getCdmaSignalStrength()->getDbm()
                         << std::endl;
      PRINT_CALLBACK << "CdmaEcio: " << signalStrength->getCdmaSignalStrength()->getCdmaEcio()
                         << std::endl;
      PRINT_CALLBACK << "EvdoEcio: " << signalStrength->getCdmaSignalStrength()->getEvdoEcio()
                         << std::endl;
      PRINT_CALLBACK << "EvdoSignalNoiseRatio: "
                         << signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio()
                         << std::endl;
      PRINT_CALLBACK
         << "Cdma Signal Level: " << (int)signalStrength->getCdmaSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getLteSignalStrength() != nullptr) {
      PRINT_CALLBACK
         << "LteSignalStrength: " << signalStrength->getLteSignalStrength()->getLteSignalStrength()
         << std::endl;
      PRINT_CALLBACK
         << "LteReferenceSignalReceivePower: " << signalStrength->getLteSignalStrength()->getDbm()
         << std::endl;
      PRINT_CALLBACK
         << "LteReferenceSignalReceiveQuality: "
         << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
         << std::endl;
      PRINT_CALLBACK << "LteReferenceSignalSnr: "
                         << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr()
                         << std::endl;
      PRINT_CALLBACK << "LteChannelQualityIndicator: "
                         << signalStrength->getLteSignalStrength()->getLteChannelQualityIndicator()
                         << std::endl;
      PRINT_CALLBACK
         << "LTE Signal Level: " << (int)signalStrength->getLteSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getWcdmaSignalStrength() != nullptr) {
      PRINT_CALLBACK << "WCDMA Signal Strength: "
                         << signalStrength->getWcdmaSignalStrength()->getSignalStrength()
                         << std::endl;
      PRINT_CALLBACK
         << "WCDMA bit error rate: " << signalStrength->getWcdmaSignalStrength()->getBitErrorRate()
         << std::endl;
      PRINT_CALLBACK
         << "WCDMA Signal Level: " << (int)signalStrength->getWcdmaSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getTdscdmaSignalStrength() != nullptr) {
      PRINT_CALLBACK << "TDSCDMA signal power: "
                         << signalStrength->getTdscdmaSignalStrength()->getRscp()
                         << std::endl;
   }
}
