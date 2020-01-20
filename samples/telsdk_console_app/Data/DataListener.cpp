/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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

#include "DataListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

void DataListener::onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   logDataCallDetails(dataCall);
}

void DataListener::logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
   std::cout << "\n\n";
   PRINT_NOTIFICATION << " ** DataCall Details **\n";
   std::cout << " ProfileID: " << dataCall->getProfileId() << std::endl;
   std::cout << " interfaceName: " << dataCall->getInterfaceName() << std::endl;
   std::cout << " DataCallStatus: " << dataCallStatusToString(dataCall->getDataCallStatus())
             << std::endl;
   std::cout << " DataCallEndReason:\n   Type: "
             << callEndReasonTypeToString(dataCall->getDataCallEndReason().type)
             << ", Code: " << callEndReasonCode(dataCall->getDataCallEndReason()) << std::endl;
   std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
   for(auto &it : ipAddrList) {
      std::cout << "\n ifAddress: " << it.ifAddress
                << "\n primaryDnsAddress: " << it.primaryDnsAddress
                << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << '\n';
   }
   std::cout << "\n APN: " << dataCall->getApnName() << '\n';
   std::cout << " IpFamilyType: " << ipFamilyTypeToString(dataCall->getIpFamilyType()) << '\n';
   std::cout << " TechPreference: " << techPreferenceToString(dataCall->getTechPreference())
             << '\n';
   std::cout << " DataBearerTechnology: " << static_cast<int>(dataCall->getCurrentBearerTech())
             << '\n';
}

std::string DataListener::techPreferenceToString(telux::data::TechPreference techPref) {
   switch(techPref) {
      case telux::data::TechPreference::TECH_PREFERENCE_3GPP:
         return "3gpp";
      case telux::data::TechPreference::TECH_PREFERENCE_3GPP2:
         return "3gpp2";
      case telux::data::TechPreference::TECH_PREFERENCE_ANY:
      default:
         return "Any";
   }
}

std::string DataListener::ipFamilyTypeToString(telux::data::IpFamilyType ipType) {
   switch(ipType) {
      case telux::data::IpFamilyType::IP_FAMILY_TYPE_V4:
         return "IPv4";
      case telux::data::IpFamilyType::IP_FAMILY_TYPE_V6:
         return "IPv6";
      case telux::data::IpFamilyType::IP_FAMILY_TYPE_V4V6:
         return "IPv4v6";
      case telux::data::IpFamilyType::IP_FAMILY_TYPE_UNKNOWN:
      default:
         return "NA";
   }
}

std::string DataListener::callEndReasonTypeToString(telux::data::EndReasonType type) {
   switch(type) {
      case telux::data::EndReasonType::CE_MOBILE_IP:
         return "CE_MOBILE_IP";
      case telux::data::EndReasonType::CE_INTERNAL:
         return "CE_INTERNAL";
      case telux::data::EndReasonType::CE_CALL_MANAGER_DEFINED:
         return "CE_CALL_MANAGER_DEFINED";
      case telux::data::EndReasonType::CE_3GPP_SPEC_DEFINED:
         return "CE_3GPP_SPEC_DEFINED";
      case telux::data::EndReasonType::CE_PPP:
         return "CE_PPP";
      case telux::data::EndReasonType::CE_EHRPD:
         return "CE_EHRPD";
      case telux::data::EndReasonType::CE_IPV6:
         return "CE_IPV6";
      case telux::data::EndReasonType::CE_UNKNOWN:
         return "CE_UNKNOWN";
      default: { return "CE_UNKNOWN"; }
   }
}

int DataListener::callEndReasonCode(telux::data::DataCallEndReason ceReason) {
   switch(ceReason.type) {
      case telux::data::EndReasonType::CE_MOBILE_IP:
         return static_cast<int>(ceReason.IpCode);
      case telux::data::EndReasonType::CE_INTERNAL:
         return static_cast<int>(ceReason.internalCode);
      case telux::data::EndReasonType::CE_CALL_MANAGER_DEFINED:
         return static_cast<int>(ceReason.cmCode);
      case telux::data::EndReasonType::CE_3GPP_SPEC_DEFINED:
         return static_cast<int>(ceReason.specCode);
      case telux::data::EndReasonType::CE_PPP:
         return static_cast<int>(ceReason.pppCode);
      case telux::data::EndReasonType::CE_EHRPD:
         return static_cast<int>(ceReason.ehrpdCode);
      case telux::data::EndReasonType::CE_IPV6:
         return static_cast<int>(ceReason.ipv6Code);
      case telux::data::EndReasonType::CE_UNKNOWN:
         return -1;
      default: { return -1; }
   }
}

std::string DataListener::dataCallStatusToString(telux::data::DataCallStatus dcStatus) {
   switch(dcStatus) {
      case telux::data::DataCallStatus::NET_CONNECTED:
         return "CONNECTED";
      case telux::data::DataCallStatus::NET_NO_NET:
         return "NO_NET";
      case telux::data::DataCallStatus::NET_IDLE:
         return "IDLE";
      case telux::data::DataCallStatus::NET_CONNECTING:
         return "CONNECTING";
      case telux::data::DataCallStatus::NET_DISCONNECTING:
         return "DISCONNECTING";
      case telux::data::DataCallStatus::NET_RECONFIGURED:
         return "RECONFIGURED";
      case telux::data::DataCallStatus::NET_NEWADDR:
         return "NEWADDR";
      case telux::data::DataCallStatus::NET_DELADDR:
         return "DELADDR";
      default: { return "INVALID"; }
   }
}
