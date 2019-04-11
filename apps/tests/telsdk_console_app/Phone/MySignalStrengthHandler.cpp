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

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

using namespace telux::tel;
using namespace telux::common;

MySignalStrengthCallback::MySignalStrengthCallback() {
}

void MySignalStrengthCallback::signalStrengthResponse(
   std::shared_ptr<SignalStrength> signalStrength, ErrorCode error) {
   std::cout << std::endl << std::endl;
   PRINT_CB << "Received Signal Strength Callback with Error Code:" << (int)error << std::endl;
   if(signalStrength->getGsmSignalStrength() != nullptr) {
      PRINT_CB
         << "GsmSignalStrength: " << signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
         << std::endl;
      PRINT_CB
         << "GsmBitErrorRate: " << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
         << std::endl;
      PRINT_CB << "GsmDbm: " << signalStrength->getGsmSignalStrength()->getDbm() << std::endl;
      PRINT_CB << "Gsm Signal Level: " << (int)signalStrength->getGsmSignalStrength()->getLevel()
               << std::endl;
   }
   if(signalStrength->getCdmaSignalStrength() != nullptr) {
      PRINT_CB << "Cdma/Evdo Dbm: " << signalStrength->getCdmaSignalStrength()->getDbm()
               << std::endl;
      PRINT_CB << "CdmaEcio: " << signalStrength->getCdmaSignalStrength()->getCdmaEcio()
               << std::endl;
      PRINT_CB << "EvdoEcio: " << signalStrength->getCdmaSignalStrength()->getEvdoEcio()
               << std::endl;
      PRINT_CB << "EvdoSignalNoiseRatio: "
               << signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio() << std::endl;
      PRINT_CB << "Cdma Signal Level: " << (int)signalStrength->getCdmaSignalStrength()->getLevel()
               << std::endl;
   }
   if(signalStrength->getLteSignalStrength() != nullptr) {
      PRINT_CB
         << "LteSignalStrength: " << signalStrength->getLteSignalStrength()->getLteSignalStrength()
         << std::endl;
      PRINT_CB
         << "LteReferenceSignalReceivePower: " << signalStrength->getLteSignalStrength()->getDbm()
         << std::endl;
      PRINT_CB << "LteReferenceSignalReceiveQuality: "
               << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
               << std::endl;
      PRINT_CB << "LteReferenceSignalSnr: "
               << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr() << std::endl;
      PRINT_CB << "LteChannelQualityIndicator: "
               << signalStrength->getLteSignalStrength()->getLteChannelQualityIndicator()
               << std::endl;
      PRINT_CB << "LTE Signal Level: " << (int)signalStrength->getLteSignalStrength()->getLevel()
               << std::endl;
   }
   if(signalStrength->getWcdmaSignalStrength() != nullptr) {
      PRINT_CB << "WCDMA Signal Strength: "
               << signalStrength->getWcdmaSignalStrength()->getSignalStrength() << std::endl;
      PRINT_CB
         << "WCDMA bit error rate: " << signalStrength->getWcdmaSignalStrength()->getBitErrorRate()
         << std::endl;
      PRINT_CB
         << "WCDMA Signal Level: " << (int)signalStrength->getWcdmaSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getTdscdmaSignalStrength() != nullptr) {
      PRINT_CB << "TDSCDMA signal power: " << signalStrength->getTdscdmaSignalStrength()->getRscp()
               << std::endl;
   }
}
