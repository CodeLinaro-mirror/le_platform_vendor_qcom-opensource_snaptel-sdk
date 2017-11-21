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

#include <chrono>
#include <iostream>
#include <memory>
#include <sstream>
#include <sys/time.h>

#include "MyPhoneListener.hpp"

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::tel;
using namespace telux::common;

void MyPhoneListener::onServiceStateChanged(std::shared_ptr<IPhone> phone, ServiceState state) {
   print_notification << "MyPhoneListener::onServiceStateChanged, " << std::endl;
}

void MyPhoneListener::onSignalStrengthChanged(std::shared_ptr<IPhone> phone,
                                              std::shared_ptr<SignalStrength> signalStrength) {
   print_notification << "MyPhoneListener::onSignalStrengthChanged " << std::endl;
   // print_notification << "getLevel(): " << signalStrength->getLevel() << std::endl;
   print_notification
      << "GsmSignalStrength: " << signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
      << std::endl;
   print_notification
      << "GsmBitErrorRate: " << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
      << std::endl;
   print_notification << "GsmDbm: " << signalStrength->getGsmSignalStrength()->getDbm()
                      << std::endl;
   print_notification << "Cdma/Evdo Dbm: " << signalStrength->getCdmaSignalStrength()->getDbm()
                      << std::endl;
   print_notification << "CdmaEcio: " << signalStrength->getCdmaSignalStrength()->getCdmaEcio()
                      << std::endl;
   print_notification << "EvdoEcio: " << signalStrength->getCdmaSignalStrength()->getEvdoEcio()
                      << std::endl;
   print_notification << "EvdoSignalNoiseRatio: "
                      << signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio()
                      << std::endl;
   print_notification
      << "LteSignalStrength: " << signalStrength->getLteSignalStrength()->getLteSignalStrength()
      << std::endl;
   print_notification
      << "LteReferenceSignalReceivePower: " << signalStrength->getLteSignalStrength()->getDbm()
      << std::endl;
   print_notification
      << "LteReferenceSignalReceiveQuality: "
      << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality() << std::endl;
   print_notification << "LteReferenceSignalSnr: "
                      << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr()
                      << std::endl;
   print_notification << "LteChannelQualityIndicator: "
                      << signalStrength->getLteSignalStrength()->getLteChannelQualityIndicator()
                      << std::endl;
   print_notification
      << "TdScdmaReceivedSignalCodePower: "
      << signalStrength->getCdmaSignalStrength()->getTdScdmaReceivedSignalCodePower() << std::endl;
   print_notification
      << "Gsm Signal Level: " << (int)signalStrength->getGsmSignalStrength()->getLevel()
      << std::endl;
   print_notification
      << "Cdma Signal Level: " << (int)signalStrength->getCdmaSignalStrength()->getLevel()
      << std::endl;
   print_notification
      << "LTE Signal Level: " << (int)signalStrength->getLteSignalStrength()->getLevel()
      << std::endl;
}

std::string MyPhoneListener::getCurrentTime() {
   timeval tod;
   gettimeofday(&tod, NULL);
   std::stringstream ss;
   time_t tt = tod.tv_sec;
   char buffer[100];
   std::strftime(buffer, 100, "%Y-%m-%d %H:%M:%S", localtime(&tt));
   char currTime[120];
   sprintf(currTime, "%s.%ld", buffer, tod.tv_usec / 1000);
   return std::string(currTime);
}

// Implementation of Signal strength callback

MySignalStrengthCallback::MySignalStrengthCallback() {
}

void MySignalStrengthCallback::signalStrengthResponse(
   std::shared_ptr<SignalStrength> signalStrength, ErrorCode error) {
   print_notification << "Received Signal Strength Callback with Error Code:" << (int)error
                      << std::endl;
   // print_notification << "getLevel(): " << signalStrength->getLevel() << std::endl;
   print_notification
      << "GsmSignalStrength: " << signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
      << std::endl;
   print_notification
      << "GsmBitErrorRate: " << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
      << std::endl;
   print_notification << "GsmDbm: " << signalStrength->getGsmSignalStrength()->getDbm()
                      << std::endl;
   print_notification << "Cdma/Evdo Dbm: " << signalStrength->getCdmaSignalStrength()->getDbm()
                      << std::endl;
   print_notification << "CdmaEcio: " << signalStrength->getCdmaSignalStrength()->getCdmaEcio()
                      << std::endl;
   print_notification << "EvdoEcio: " << signalStrength->getCdmaSignalStrength()->getEvdoEcio()
                      << std::endl;
   print_notification << "EvdoSignalNoiseRatio: "
                      << signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio()
                      << std::endl;
   print_notification
      << "LteSignalStrength: " << signalStrength->getLteSignalStrength()->getLteSignalStrength()
      << std::endl;
   print_notification
      << "LteReferenceSignalReceivePower: " << signalStrength->getLteSignalStrength()->getDbm()
      << std::endl;
   print_notification
      << "LteReferenceSignalReceiveQuality: "
      << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality() << std::endl;
   print_notification << "LteReferenceSignalSnr: "
                      << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr()
                      << std::endl;
   print_notification << "LteChannelQualityIndicator: "
                      << signalStrength->getLteSignalStrength()->getLteChannelQualityIndicator()
                      << std::endl;
   print_notification
      << "TdScdmaReceivedSignalCodePower: "
      << signalStrength->getCdmaSignalStrength()->getTdScdmaReceivedSignalCodePower() << std::endl;
   print_notification
      << "Gsm Signal Level: " << (int)signalStrength->getGsmSignalStrength()->getLevel()
      << std::endl;
   print_notification
      << "Cdma Signal Level: " << (int)signalStrength->getCdmaSignalStrength()->getLevel()
      << std::endl;
   print_notification
      << "LTE Signal Level: " << (int)signalStrength->getLteSignalStrength()->getLevel()
      << std::endl;
}
