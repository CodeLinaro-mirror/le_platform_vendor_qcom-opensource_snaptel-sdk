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
#include <memory>
#include <sstream>
#include <sys/time.h>
#include <string>

#include "MyPhoneListener.hpp"

#define print_notification std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define print_cb std::cout << "\033[1;35mCALLBACK: \033[0m"

void MyPhoneListener::onServiceStateChanged(int phoneId, telux::tel::ServiceState state) {
   std::cout << "\n";
   print_notification << "MyPhoneListener::onServiceStateChanged for PhoneId = " << phoneId
                      << " ,ServiceState = " << serviceStateToString(state) << std::endl;
}

std::string MyPhoneListener::serviceStateToString(telux::tel::ServiceState serviceState) {
   std::string state = "";
   switch(serviceState) {
      case telux::tel::ServiceState::EMERGENCY_ONLY:
         state = "Emergency Only";
         break;
      case telux::tel::ServiceState::IN_SERVICE:
         state = "In Service";
         break;
      case telux::tel::ServiceState::OUT_OF_SERVICE:
         state = "Out Of Service";
         break;
      case telux::tel::ServiceState::RADIO_OFF:
         state = "Radio Off";
         break;
      default:
         state = "Unknown";
         break;
   }
   return state;
}

void MyPhoneListener::onSignalStrengthChanged(
   int phoneId, std::shared_ptr<telux::tel::SignalStrength> signalStrength) {
   std::cout << std::endl << std::endl;
   print_notification << "MyPhoneListener::onSignalStrengthChanged for PhoneId = " << phoneId
                      << std::endl;
   if(signalStrength->getGsmSignalStrength() != nullptr) {
      print_notification
         << "GsmSignalStrength: " << signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
         << std::endl;
      print_notification
         << "GsmBitErrorRate: " << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
         << std::endl;
      print_notification << "GsmDbm: " << signalStrength->getGsmSignalStrength()->getDbm()
                         << std::endl;
      print_notification
         << "Gsm Signal Level: " << (int)signalStrength->getGsmSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getCdmaSignalStrength() != nullptr) {
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
         << "Cdma Signal Level: " << (int)signalStrength->getCdmaSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getLteSignalStrength() != nullptr) {
      print_notification
         << "LteSignalStrength: " << signalStrength->getLteSignalStrength()->getLteSignalStrength()
         << std::endl;
      print_notification
         << "LteReferenceSignalReceivePower: " << signalStrength->getLteSignalStrength()->getDbm()
         << std::endl;
      print_notification
         << "LteReferenceSignalReceiveQuality: "
         << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
         << std::endl;
      print_notification << "LteReferenceSignalSnr: "
                         << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr()
                         << std::endl;
      print_notification << "LteChannelQualityIndicator: "
                         << signalStrength->getLteSignalStrength()->getLteChannelQualityIndicator()
                         << std::endl;
      print_notification
         << "LTE Signal Level: " << (int)signalStrength->getLteSignalStrength()->getLevel()
         << std::endl;
   }
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
   std::shared_ptr<telux::tel::SignalStrength> signalStrength, telux::common::ErrorCode error) {
   std::cout << std::endl << std::endl;

   if(error != telux::common::ErrorCode::SUCCESS) {
      print_cb << " signalStrengthResponse failed with errorCode: " << static_cast<int>(error)
               << std::endl;
      return;
   }

   print_cb << "Received Signal Strength Callback" << std::endl;
   if(signalStrength->getGsmSignalStrength() != nullptr) {
      print_cb
         << "GsmSignalStrength: " << signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
         << std::endl;
      print_cb
         << "GsmBitErrorRate: " << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
         << std::endl;
      print_cb << "GsmDbm: " << signalStrength->getGsmSignalStrength()->getDbm() << std::endl;
      print_cb << "Gsm Signal Level: " << (int)signalStrength->getGsmSignalStrength()->getLevel()
               << std::endl;
   }
   if(signalStrength->getCdmaSignalStrength() != nullptr) {
      print_cb << "Cdma/Evdo Dbm: " << signalStrength->getCdmaSignalStrength()->getDbm()
               << std::endl;
      print_cb << "CdmaEcio: " << signalStrength->getCdmaSignalStrength()->getCdmaEcio()
               << std::endl;
      print_cb << "EvdoEcio: " << signalStrength->getCdmaSignalStrength()->getEvdoEcio()
               << std::endl;
      print_cb << "EvdoSignalNoiseRatio: "
               << signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio() << std::endl;
      print_cb << "Cdma Signal Level: " << (int)signalStrength->getCdmaSignalStrength()->getLevel()
               << std::endl;
   }
   if(signalStrength->getLteSignalStrength() != nullptr) {
      print_cb
         << "LteSignalStrength: " << signalStrength->getLteSignalStrength()->getLteSignalStrength()
         << std::endl;
      print_cb
         << "LteReferenceSignalReceivePower: " << signalStrength->getLteSignalStrength()->getDbm()
         << std::endl;
      print_cb << "LteReferenceSignalReceiveQuality: "
               << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
               << std::endl;
      print_cb << "LteReferenceSignalSnr: "
               << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr() << std::endl;
      print_cb << "LteChannelQualityIndicator: "
               << signalStrength->getLteSignalStrength()->getLteChannelQualityIndicator()
               << std::endl;
      print_cb << "LTE Signal Level: " << (int)signalStrength->getLteSignalStrength()->getLevel()
               << std::endl;
   }
}

void MyRadioPowerCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "Radio power request executed successfully" << std::endl;
   } else {
      print_cb << "Radio power request failed" << std::endl;
   }
   print_cb << "RadioPowerRequest error = " << static_cast<int>(error) << std::endl;
}

void MyPhoneListener::onRadioStateChanged(int phoneId, telux::tel::RadioState state) {
   std::cout << "\n";
   print_notification << "MyPhoneListener::onRadioStateChanged for PhoneId " << phoneId
                      << " , RadioState: " << radioStateToString(state) << std::endl;
}

std::string MyPhoneListener::radioStateToString(telux::tel::RadioState radioState) {
   std::string state = "";
   switch(radioState) {
      case telux::tel::RadioState::RADIO_STATE_OFF:
         state = "Off";
         break;
      case telux::tel::RadioState::RADIO_STATE_UNAVAILABLE:
         state = "Unavailable";
         break;
      case telux::tel::RadioState::RADIO_STATE_ON:
         state = "On";
         break;
      default:
         state = "Unknown";
         break;
   }
   return state;
}

void MyVoiceRadioTechnologyCallback::voiceRadioTechnologyResponse(
   telux::tel::RadioTechnology radioTechnology, telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "requestVoiceRadioTechnology is successful, Radio technology: "
               << radioTechToString(radioTechnology) << std::endl;
   } else {
      print_cb << "Request Voice Technology failed, errorCode: " << static_cast<int>(error)
               << std::endl;
   }
}
std::string
   MyVoiceRadioTechnologyCallback::radioTechToString(telux::tel::RadioTechnology radioTech) {
   std::string rtString = "";
   switch(radioTech) {
      case telux::tel::RadioTechnology::RADIO_TECH_GPRS:
         rtString = "GPRS";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_EDGE:
         rtString = "EDGE";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_UMTS:
         rtString = "UMTS";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_IS95A:
         rtString = "IS95A";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_IS95B:
         rtString = "IS95B";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_1xRTT:
         rtString = "1xRTT";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_EVDO_0:
         rtString = "EVDO_0";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_EVDO_A:
         rtString = "EVDO_A";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_HSUPA:
         rtString = "HSUPA";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_HSPA:
         rtString = "HSPA";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_EVDO_B:
         rtString = "EVDO_B";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_EHRPD:
         rtString = "EHRPD";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_LTE:
         rtString = "LTE";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_HSPAP:
         rtString = "HSPA+";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_GSM:
         rtString = "GSM";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_TD_SCDMA:
         rtString = "TD_SCDMA";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_IWLAN:
         rtString = "IWLAN";
         break;
      case telux::tel::RadioTechnology::RADIO_TECH_LTE_CA:
         rtString = "LTE_CA";
         break;
      default:
         rtString = "Unknown";
         break;
   }
   return rtString;
}

void MyVoiceServiceStateCallback::voiceServiceStateResponse(
   const std::shared_ptr<telux::tel::VoiceServiceInfo> &serviceInfo,
   telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "requestVoiceServiceState is successful, Service State: "
               << voiceServiceStateToString(serviceInfo->getVoiceServiceState()) << std::endl;
   } else {
      print_cb << "requestVoiceServiceState is failed, errorCode: " << static_cast<int>(error)
               << std::endl;
   }
}
std::string MyVoiceServiceStateCallback::voiceServiceStateToString(
   telux::tel::VoiceServiceState vocSrvState) {
   std::string state = "";
   switch(vocSrvState) {
      case telux::tel::VoiceServiceState::NOT_REG_AND_NOT_SEARCHING:
         state = "NOT_REG_AND_NOT_SEARCHING";
         break;
      case telux::tel::VoiceServiceState::REG_HOME:
         state = "REG_HOME";
         break;
      case telux::tel::VoiceServiceState::NOT_REG_AND_SEARCHING:
         state = "NOT_REG_AND_SEARCHING";
         break;
      case telux::tel::VoiceServiceState::REG_DENIED:
         state = "REG_DENIED";
         break;
      case telux::tel::VoiceServiceState::UNKNOWN:
         state = "UNKNOWN";
         break;
      case telux::tel::VoiceServiceState::REG_ROAMING:
         state = "REG_ROAMING";
         break;
      case telux::tel::VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_NOT_SEARCHING:
         state = "NOT_REG_AND_EMERGENCY_AVAILABLE_AND_NOT_SEARCHING";
         break;
      case telux::tel::VoiceServiceState::NOT_REG_AND_EMERGENCY_AVAILABLE_AND_SEARCHING:
         state = "NOT_REG_AND_EMERGENCY_AVAILABLE_AND_SEARCHING";
         break;
      case telux::tel::VoiceServiceState::REG_DENIED_AND_EMERGENCY_AVAILABLE:
         state = "REG_DENIED_AND_EMERGENCY_AVAILABLE";
         break;
      case telux::tel::VoiceServiceState::UNKNOWN_AND_EMERGENCY_AVAILABLE:
         state = "UNKNOWN_AND_EMERGENCY_AVAILABLE";
         break;
      default:
         state = "Unknown";
         break;
   }
   return state;
}

void MyPhoneListener::onVoiceRadioTechnologyChanged(int phoneId,
                                                    telux::tel::RadioTechnology radioTechnology) {
   std::cout << "\n";
   print_notification << "Received unsol response for PhoneId " << phoneId
                      << " for change in voice radio technology" << std::endl;
   print_notification << "Changed Radio technology " << static_cast<int>(radioTechnology)
                      << std::endl;
}

void MyPhoneListener::onVoiceServiceStateChanged(
   int phoneId, const std::shared_ptr<telux::tel::VoiceServiceInfo> &srvInfo) {
   std::cout << "\n";
   print_notification << "Received unsol response for PhoneId " << phoneId
                      << " for change in voice service state" << std::endl;
   if(srvInfo) {
      print_notification
         << "VoiceRegistrationState: " << static_cast<int>(srvInfo->getVoiceServiceState())
         << std::endl;
      if(srvInfo->isEmergency()) {
         print_notification << "Phone is in EMERGENCY_ONLY mode" << std::endl;
      }
      if(srvInfo->isInService()) {
         print_notification << "Phone is in HOME network mode" << std::endl;
      }
      if(srvInfo->isOutOfService()) {
         print_notification << "Phone is in OUT_OF_SERVICE mode" << std::endl;
      }
   }
}

std::string MyCellularCapabilityCallback::voiceServiceTechnologiesMaskToString(
   telux::tel::VoiceServiceTechnologiesMask vstMask) {
   std::string vocSrvTechStr = "Unknown";
   if(vstMask[static_cast<int>(telux::tel::VoiceServiceTechnology::VOICE_TECH_GW_CSFB)]) {
      vocSrvTechStr = "GW_CSFB";
   }
   if(vstMask[static_cast<int>(telux::tel::VoiceServiceTechnology::VOICE_TECH_1x_CSFB)]) {
      vocSrvTechStr = "1x_CSFB";
   }
   if(vstMask[static_cast<int>(telux::tel::VoiceServiceTechnology::VOICE_TECH_VOLTE)]) {
      vocSrvTechStr = "VOLTE";
   }
   return vocSrvTechStr;
}

std::string MyCellularCapabilityCallback::ratCapabilitiesMaskToString(
   telux::tel::RATCapabilitiesMask ratCapabilitiesMask) {
   std::string ratCapStr = "Unknown";
   if(ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::AMPS)]) {
      ratCapStr = "AMPS ";
   }
   if(ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::CDMA)]) {
      ratCapStr = "CDMA ";
   }
   if(ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::HDR)]) {
      ratCapStr = "HDR ";
   }
   if(ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::GSM)]) {
      ratCapStr = "GSM ";
   }
   if(ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::WCDMA)]) {
      ratCapStr = "WCDMA ";
   }
   if(ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::LTE)]) {
      ratCapStr = "LTE ";
   }
   if(ratCapabilitiesMask[static_cast<int>(telux::tel::RATCapability::TDS)]) {
      ratCapStr = "TDS ";
   }
   return ratCapStr;
}

void MyCellularCapabilityCallback::cellularCapabilityResponse(
   telux::tel::CellularCapabilityInfo capabilityInfo, telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "requestCellularCapability response is successful" << std::endl;
      print_cb << "VoiceServiceTechnologiesMask: "
               << voiceServiceTechnologiesMaskToString(capabilityInfo.voiceServiceTechs)
               << std::endl;

      for(auto &simRatCap : capabilityInfo.simRatCapabilities) {
         print_cb << "RATCapabilitiesMask: " << ratCapabilitiesMaskToString(simRatCap.capabilities)
                  << std::endl;
      }

      print_cb << "SIM Count : " << capabilityInfo.simCount << std::endl;
      print_cb << "Max Active SIMs : " << capabilityInfo.maxActiveSims << std::endl;
   } else {
      print_cb << "requestCellularCapability is failed, errorCode: " << static_cast<int>(error)
               << std::endl;
   }
}

std::string MyPhoneHelper::operatingModeToString(telux::tel::OperatingMode operatingMode) {
   std::string mode = "";
   switch(operatingMode) {
      case telux::tel::OperatingMode::ONLINE:
         mode = "ONLINE";
         break;
      case telux::tel::OperatingMode::AIRPLANE:
         mode = "AIRPLANE";
         break;
      case telux::tel::OperatingMode::FACTORY_TEST:
         mode = "FACTORY_TEST";
         break;
      case telux::tel::OperatingMode::OFFLINE:
         mode = "OFFLINE";
         break;
      case telux::tel::OperatingMode::RESETTING:
         mode = "RESETTING";
         break;
      case telux::tel::OperatingMode::SHUTTING_DOWN:
         mode = "SHUTTING_DOWN";
         break;
      case telux::tel::OperatingMode::PERSISTENT_LOW_POWER:
         mode = "PERSISTENT_LOW_POWER";
         break;
      default:
         mode = "Unknown";
         break;
   }
   return mode;
}

void MyGetOperatingModeCallback::operatingModeResponse(telux::tel::OperatingMode operatingMode,
                                                       telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "requestOperatingMode response is successful" << std::endl;
      print_cb << "Operating Mode: " << MyPhoneHelper::operatingModeToString(operatingMode)
               << std::endl;
   } else {
      print_cb << "requestOperatingMode is failed, errorCode: " << static_cast<int>(error)
               << std::endl;
   }
}

void MyPhoneListener::onOperatingModeChanged(telux::tel::OperatingMode mode) {
   std::cout << "\n";
   print_notification << "Received Operating Mode Change " << std::endl;
   print_notification << "Operating Mode: " << MyPhoneHelper::operatingModeToString(mode)
                      << std::endl;
}

void MySetOperatingModeCallback::setOperatingModeResponse(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      print_cb << "Set operating mode request executed successfully" << std::endl;
   } else {
      print_cb << "Set operating mode request failed" << std::endl;
   }
   print_cb << "SetOperatingModeRequest error = " << static_cast<int>(error) << std::endl;
}