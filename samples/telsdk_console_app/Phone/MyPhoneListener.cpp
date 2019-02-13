/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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
#include <string>

extern "C" {
#include <sys/time.h>
}

#include "MyPhoneListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"
#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"

void MyPhoneListener::onServiceStateChanged(int phoneId, telux::tel::ServiceState state) {
   std::cout << "\n";
   PRINT_NOTIFICATION << "onServiceStateChanged for PhoneId = " << phoneId
                      << " , ServiceState = " << serviceStateToString(state) << std::endl;
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
   PRINT_NOTIFICATION << "onSignalStrengthChanged for PhoneId = " << phoneId << std::endl;
   if(signalStrength->getGsmSignalStrength() != nullptr) {
      PRINT_NOTIFICATION
         << "GsmSignalStrength: " << signalStrength->getGsmSignalStrength()->getGsmSignalStrength()
         << std::endl;
      PRINT_NOTIFICATION
         << "GsmBitErrorRate: " << signalStrength->getGsmSignalStrength()->getGsmBitErrorRate()
         << std::endl;
      PRINT_NOTIFICATION << "GsmDbm: " << signalStrength->getGsmSignalStrength()->getDbm()
                         << std::endl;
      PRINT_NOTIFICATION
         << "Gsm Signal Level: " << (int)signalStrength->getGsmSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getCdmaSignalStrength() != nullptr) {
      PRINT_NOTIFICATION << "Cdma/Evdo Dbm: " << signalStrength->getCdmaSignalStrength()->getDbm()
                         << std::endl;
      PRINT_NOTIFICATION << "CdmaEcio: " << signalStrength->getCdmaSignalStrength()->getCdmaEcio()
                         << std::endl;
      PRINT_NOTIFICATION << "EvdoEcio: " << signalStrength->getCdmaSignalStrength()->getEvdoEcio()
                         << std::endl;
      PRINT_NOTIFICATION << "EvdoSignalNoiseRatio: "
                         << signalStrength->getCdmaSignalStrength()->getEvdoSignalNoiseRatio()
                         << std::endl;
      PRINT_NOTIFICATION
         << "Cdma Signal Level: " << (int)signalStrength->getCdmaSignalStrength()->getLevel()
         << std::endl;
   }
   if(signalStrength->getLteSignalStrength() != nullptr) {
      PRINT_NOTIFICATION
         << "LteSignalStrength: " << signalStrength->getLteSignalStrength()->getLteSignalStrength()
         << std::endl;
      PRINT_NOTIFICATION
         << "LteReferenceSignalReceivePower: " << signalStrength->getLteSignalStrength()->getDbm()
         << std::endl;
      PRINT_NOTIFICATION
         << "LteReferenceSignalReceiveQuality: "
         << signalStrength->getLteSignalStrength()->getLteReferenceSignalReceiveQuality()
         << std::endl;
      PRINT_NOTIFICATION << "LteReferenceSignalSnr: "
                         << signalStrength->getLteSignalStrength()->getLteReferenceSignalSnr()
                         << std::endl;
      PRINT_NOTIFICATION << "LteChannelQualityIndicator: "
                         << signalStrength->getLteSignalStrength()->getLteChannelQualityIndicator()
                         << std::endl;
      PRINT_NOTIFICATION
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
   snprintf(currTime, 120, "%s.%ld", buffer, tod.tv_usec / 1000);
   return std::string(currTime);
}

void MyRadioPowerCallback::commandResponse(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Radio power request executed successfully" << std::endl;
   } else {
      PRINT_CB << "Radio power request failed" << std::endl;
   }
   PRINT_CB << "RadioPowerRequest error = " << static_cast<int>(error) << std::endl;
}

void MyPhoneListener::onRadioStateChanged(int phoneId, telux::tel::RadioState state) {
   std::cout << "\n";
   PRINT_NOTIFICATION << "onRadioStateChanged for PhoneId " << phoneId
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
      PRINT_CB << "requestVoiceRadioTechnology is successful, Radio technology: "
               << radioTechToString(radioTechnology) << std::endl;
   } else {
      PRINT_CB << "Request Voice Technology failed, errorCode: " << static_cast<int>(error)
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
      PRINT_CB << "requestVoiceServiceState is successful, Service State: "
               << voiceServiceStateToString(serviceInfo->getVoiceServiceState()) << std::endl;
   } else {
      PRINT_CB << "requestVoiceServiceState is failed, errorCode: " << static_cast<int>(error)
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
   PRINT_NOTIFICATION << "Received unsol response for PhoneId " << phoneId
                      << " for change in voice radio technology" << std::endl;
   PRINT_NOTIFICATION << "Changed Radio technology " << static_cast<int>(radioTechnology)
                      << std::endl;
}

void MyPhoneListener::onVoiceServiceStateChanged(
   int phoneId, const std::shared_ptr<telux::tel::VoiceServiceInfo> &srvInfo) {
   std::cout << "\n";
   if(srvInfo) {
      PRINT_NOTIFICATION << "\nonVoiceServiceStateChanged: State: "
                         << static_cast<int>(srvInfo->getVoiceServiceState());
      if(srvInfo->isEmergency()) {
         std::cout << ", Phone is in EMERGENCY_ONLY mode" << std::endl;
      }
      if(srvInfo->isInService()) {
         std::cout << ", Phone is in HOME network mode" << std::endl;
      }
      if(srvInfo->isOutOfService()) {
         std::cout << ", Phone is in OUT_OF_SERVICE mode" << std::endl;
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
      PRINT_CB << "requestCellularCapability response is successful" << std::endl;
      PRINT_CB << "VoiceServiceTechnologiesMask: "
               << voiceServiceTechnologiesMaskToString(capabilityInfo.voiceServiceTechs)
               << std::endl;

      for(auto &simRatCap : capabilityInfo.simRatCapabilities) {
         PRINT_CB << "RATCapabilitiesMask: " << ratCapabilitiesMaskToString(simRatCap.capabilities)
                  << std::endl;
      }

      PRINT_CB << "SIM Count : " << capabilityInfo.simCount << std::endl;
      PRINT_CB << "Max Active SIMs : " << capabilityInfo.maxActiveSims << std::endl;
   } else {
      PRINT_CB << "requestCellularCapability is failed, errorCode: " << static_cast<int>(error)
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
      PRINT_CB << "requestOperatingMode response is successful" << std::endl;
      PRINT_CB << "Operating Mode: " << MyPhoneHelper::operatingModeToString(operatingMode)
               << std::endl;
   } else {
      PRINT_CB << "requestOperatingMode is failed, errorCode: " << static_cast<int>(error)
               << std::endl;
   }
}

void MyPhoneListener::onOperatingModeChanged(telux::tel::OperatingMode mode) {
   std::cout << "\n";
   PRINT_NOTIFICATION << "Received Operating Mode Change " << std::endl;
   PRINT_NOTIFICATION << "Operating Mode: " << MyPhoneHelper::operatingModeToString(mode)
                      << std::endl;
}

void MySetOperatingModeCallback::setOperatingModeResponse(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Set operating mode request executed successfully" << std::endl;
   } else {
      PRINT_CB << "Set operating mode request failed" << std::endl;
   }
   PRINT_CB << "SetOperatingModeRequest error = " << static_cast<int>(error) << std::endl;
}

void MyPhoneListener::onCellInfoListChanged(
   int phoneId, std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList) {
   PRINT_NOTIFICATION << "Received unsol response for PhoneId " << phoneId
                      << " for change in cellinfo" << std::endl;
   PRINT_NOTIFICATION << "CellInfo: " << std::endl;
   MyPhoneHelper::printCellInfoDetails(cellInfoList);
}

void MyPhoneHelper::printCellInfoDetails(
   std::vector<std::shared_ptr<telux::tel::CellInfo>> cellInfoList) {
   for(auto cellinfo : cellInfoList) {
      PRINT_NOTIFICATION << "CellInfo Type: " << (int)cellinfo->getType() << std::endl;
      if(cellinfo->getType() == telux::tel::CellType::GSM) {
         PRINT_NOTIFICATION << "GSM Cellinfo " << std::endl;
         auto gsmCellInfo = std::static_pointer_cast<telux::tel::GsmCellInfo>(cellinfo);
         PRINT_NOTIFICATION << "GSM isRegistered: " << gsmCellInfo->isRegistered() << std::endl;
         PRINT_NOTIFICATION << "GSM mcc: " << gsmCellInfo->getCellIdentity().getMcc() << std::endl;
         PRINT_NOTIFICATION << "GSM mnc: " << gsmCellInfo->getCellIdentity().getMnc() << std::endl;
         PRINT_NOTIFICATION << "GSM lac: " << gsmCellInfo->getCellIdentity().getLac() << std::endl;
         PRINT_NOTIFICATION << "GSM cid: " << gsmCellInfo->getCellIdentity().getIdentity()
                            << std::endl;
         PRINT_NOTIFICATION << "GSM arfcn: " << gsmCellInfo->getCellIdentity().getArfcn()
                            << std::endl;
         // GSM signal strength
         PRINT_NOTIFICATION << "GSM Signal Strength: "
                            << gsmCellInfo->getSignalStrengthInfo().getGsmSignalStrength()
                            << std::endl;
         PRINT_NOTIFICATION
            << "GSM Bit error rate: " << gsmCellInfo->getSignalStrengthInfo().getGsmBitErrorRate()
            << std::endl;
         PRINT_NOTIFICATION
            << "GSM TimingAdvance: " << gsmCellInfo->getSignalStrengthInfo().getTimingAdvance()
            << std::endl;
         PRINT_NOTIFICATION
            << "GSM Signal Level: " << (int)gsmCellInfo->getSignalStrengthInfo().getLevel()
            << std::endl;
      } else if(cellinfo->getType() == telux::tel::CellType::CDMA) {
         PRINT_NOTIFICATION << "CDMA Cellinfo " << std::endl;
         auto cdmaCellInfo = std::static_pointer_cast<telux::tel::CdmaCellInfo>(cellinfo);
         PRINT_NOTIFICATION << "CDMA isRegistered: " << cdmaCellInfo->isRegistered() << std::endl;
         PRINT_NOTIFICATION << "CDMA networkId: " << cdmaCellInfo->getCellIdentity().getNid()
                            << std::endl;
         PRINT_NOTIFICATION << "CDMA SystemId: " << cdmaCellInfo->getCellIdentity().getSid()
                            << std::endl;
         PRINT_NOTIFICATION
            << "CDMA BaseStationId: " << cdmaCellInfo->getCellIdentity().getBaseStationId()
            << std::endl;
         PRINT_NOTIFICATION << "CDMA Longitude: " << cdmaCellInfo->getCellIdentity().getLongitude()
                            << std::endl;
         PRINT_NOTIFICATION << "CDMA Latitude: " << cdmaCellInfo->getCellIdentity().getLatitude()
                            << std::endl;
         // CDMA Signal Strength
         PRINT_NOTIFICATION << "CDMA Dbm: " << cdmaCellInfo->getSignalStrengthInfo().getDbm()
                            << std::endl;
         PRINT_NOTIFICATION << "CDMA Ecio: " << cdmaCellInfo->getSignalStrengthInfo().getCdmaEcio()
                            << std::endl;
         PRINT_NOTIFICATION << "EVDO Ecio: " << cdmaCellInfo->getSignalStrengthInfo().getEvdoEcio()
                            << std::endl;
         PRINT_NOTIFICATION
            << "EVDO SNR: " << cdmaCellInfo->getSignalStrengthInfo().getEvdoSignalNoiseRatio()
            << std::endl;
         PRINT_NOTIFICATION
            << "CDMA/EVDO Signal Level: " << (int)cdmaCellInfo->getSignalStrengthInfo().getLevel()
            << std::endl;
      } else if(cellinfo->getType() == telux::tel::CellType::LTE) {
         PRINT_NOTIFICATION << "LTE Cellinfo  " << std::endl;
         auto lteCellInfo = std::static_pointer_cast<telux::tel::LteCellInfo>(cellinfo);
         PRINT_NOTIFICATION << "LTE isRegistered: " << lteCellInfo->isRegistered() << std::endl;
         PRINT_NOTIFICATION << "LTE mcc: " << lteCellInfo->getCellIdentity().getMcc() << std::endl;
         PRINT_NOTIFICATION << "LTE mnc: " << lteCellInfo->getCellIdentity().getMnc() << std::endl;
         PRINT_NOTIFICATION << "LTE cid: " << lteCellInfo->getCellIdentity().getIdentity()
                            << std::endl;
         PRINT_NOTIFICATION << "LTE pid: " << lteCellInfo->getCellIdentity().getPhysicalCellId()
                            << std::endl;
         PRINT_NOTIFICATION << "LTE tac: " << lteCellInfo->getCellIdentity().getTrackingAreaCode()
                            << std::endl;
         PRINT_NOTIFICATION << "LTE arfcn: " << lteCellInfo->getCellIdentity().getEarfcn()
                            << std::endl;
         // LTE Signal Strength
         PRINT_NOTIFICATION << "LTE signal strength: "
                            << lteCellInfo->getSignalStrengthInfo().getLteSignalStrength()
                            << std::endl;
         PRINT_NOTIFICATION << "LTE Rsrp: " << lteCellInfo->getSignalStrengthInfo().getDbm()
                            << std::endl;
         PRINT_NOTIFICATION
            << "LTE Rsrq: "
            << lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalReceiveQuality()
            << std::endl;
         PRINT_NOTIFICATION
            << "LTE Rssnr: " << lteCellInfo->getSignalStrengthInfo().getLteReferenceSignalSnr()
            << std::endl;
         PRINT_NOTIFICATION
            << "LTE Cqi: " << lteCellInfo->getSignalStrengthInfo().getLteChannelQualityIndicator()
            << std::endl;
         PRINT_NOTIFICATION
            << "LTE Timing Advance: " << lteCellInfo->getSignalStrengthInfo().getTimingAdvance()
            << std::endl;
         PRINT_NOTIFICATION
            << "LTE Signal Level: " << (int)lteCellInfo->getSignalStrengthInfo().getLevel()
            << std::endl;
      } else if(cellinfo->getType() == telux::tel::CellType::WCDMA) {
         PRINT_NOTIFICATION << "WCDMA Cellinfo " << std::endl;
         auto wcdmaCellInfo = std::static_pointer_cast<telux::tel::WcdmaCellInfo>(cellinfo);
         PRINT_NOTIFICATION << "WCDMA isRegistered: " << wcdmaCellInfo->isRegistered() << std::endl;
         PRINT_NOTIFICATION << "WCDMA mcc: " << wcdmaCellInfo->getCellIdentity().getMcc()
                            << std::endl;
         PRINT_NOTIFICATION << "WCDMA mnc: " << wcdmaCellInfo->getCellIdentity().getMnc()
                            << std::endl;
         PRINT_NOTIFICATION << "WCDMA lac: " << wcdmaCellInfo->getCellIdentity().getLac()
                            << std::endl;
         PRINT_NOTIFICATION << "WCDMA cid: " << wcdmaCellInfo->getCellIdentity().getIdentity()
                            << std::endl;
         PRINT_NOTIFICATION
            << "WCDMA psc: " << wcdmaCellInfo->getCellIdentity().getPrimaryScramblingCode()
            << std::endl;
         PRINT_NOTIFICATION << "WCDMA arfcn: " << wcdmaCellInfo->getCellIdentity().getUarfcn()
                            << std::endl;
         // WCDMA Signal Strength
         PRINT_NOTIFICATION << "WCDMA Signal Strength: "
                            << wcdmaCellInfo->getSignalStrengthInfo().getSignalStrength()
                            << std::endl;
         PRINT_NOTIFICATION
            << "WCDMA bit error rate: " << wcdmaCellInfo->getSignalStrengthInfo().getBitErrorRate()
            << std::endl;
         PRINT_NOTIFICATION
            << "WCDMA Signal Level: " << (int)wcdmaCellInfo->getSignalStrengthInfo().getLevel()
            << std::endl;
      } else if(cellinfo->getType() == telux::tel::CellType::TDSCDMA) {
         PRINT_NOTIFICATION << "TDSCDMA Cellinfo " << std::endl;
         auto tdsCdmaCellInfo = std::static_pointer_cast<telux::tel::TdscdmaCellInfo>(cellinfo);
         PRINT_NOTIFICATION << "TDSCDMA isRegistered: " << tdsCdmaCellInfo->isRegistered()
                            << std::endl;
         PRINT_NOTIFICATION << "TDSCDMA MCC: " << tdsCdmaCellInfo->getCellIdentity().getMcc()
                            << std::endl;
         PRINT_NOTIFICATION << "TDSCDMA MNC: " << tdsCdmaCellInfo->getCellIdentity().getMnc()
                            << std::endl;
         PRINT_NOTIFICATION << "TDSCDMA LAC : " << tdsCdmaCellInfo->getCellIdentity().getLac()
                            << std::endl;
         PRINT_NOTIFICATION << "TDSCDMA CID: " << tdsCdmaCellInfo->getCellIdentity().getIdentity()
                            << std::endl;
         PRINT_NOTIFICATION << "TDSCDMA Cell Parameters Id : "
                            << tdsCdmaCellInfo->getCellIdentity().getParametersId() << std::endl;
         // TDSCDMA signal strength..
         PRINT_NOTIFICATION
            << "TDSCDMA power : " << tdsCdmaCellInfo->getSignalStrengthInfo().getRscp()
            << std::endl;
      }
   }
}

void MyPhoneListener::onECallOperatingModeChange(int phoneId, telux::tel::ECallModeInfo modeInfo) {

   PRINT_NOTIFICATION << "onECallOperatingModeChange for PhoneId = " << phoneId
                      << " , mode = " << MyPhoneHelper::eCallOperatingModeToString(modeInfo.mode)
                      << " , reason = " << eCallModeReasonToString(modeInfo.reason) << std::endl;
}

std::string MyPhoneHelper::eCallOperatingModeToString(telux::tel::ECallMode mode) {
   std::string eCallOprtModeString = "UNKNOWN";
   switch(mode) {
      case telux::tel::ECallMode::NORMAL:
         eCallOprtModeString = "NORMAL";
         break;
      case telux::tel::ECallMode::ECALL_ONLY:
         eCallOprtModeString = "ECALL_ONLY";
         break;
      case telux::tel::ECallMode::NONE:
         eCallOprtModeString = "NONE";
         break;
      default:
         break;
   }
   return eCallOprtModeString;
}

std::string MyPhoneListener::eCallModeReasonToString(telux::tel::ECallModeReason modeReason) {
   std::string reason = "";
   switch(modeReason) {
      case telux::tel::ECallModeReason::NORMAL:
         reason = "NORMAL";
         break;
      case telux::tel::ECallModeReason::ERA_GLONASS:
         reason = "ERA_GLONASS";
         break;
      default:
         reason = "UNKNOWN";
         break;
   }
   return reason;
}

void MySetECallOperatingModeCallback::setECallOperatingModeResponse(telux::common::ErrorCode error) {
   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "Set eCall operating mode request executed successfully" << std::endl;
   } else {
      PRINT_CB << "Set eCall operating mode request failed" << std::endl;
   }
   PRINT_CB << "SetECallOperatingModeRequest error: " << static_cast<int>(error) << std::endl;
}

void MyGetECallOperatingModeCallback::getECallOperatingModeResponse(
   telux::tel::ECallMode eCallMode, telux::common::ErrorCode error) {

   std::cout << "\n";
   if(error == telux::common::ErrorCode::SUCCESS) {
      PRINT_CB << "eCall operating mode request executed successfully" << std::endl;
      PRINT_CB << "eCall Operating Mode: " << MyPhoneHelper::eCallOperatingModeToString(eCallMode)
               << std::endl;
   } else {
      PRINT_CB << "Request eCall Operating Mode failed, errorCode: " << static_cast<int>(error)
               << std::endl;
   }
}