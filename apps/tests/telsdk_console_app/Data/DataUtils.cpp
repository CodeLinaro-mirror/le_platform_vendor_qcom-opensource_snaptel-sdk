/*
 *  Copyright (c) 2019-2021, The Linux Foundation. All rights reserved.
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
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include <iomanip>
#include <algorithm>

#include "DataUtils.hpp"

std::string DataUtils::techPreferenceToString(telux::data::TechPreference techPref) {
   switch(techPref) {
      case telux::data::TechPreference::TP_3GPP:
         return "3gpp";
      case telux::data::TechPreference::TP_3GPP2:
         return "3gpp2";
      case telux::data::TechPreference::TP_ANY:
      default:
         return "Any";
   }
}

std::string DataUtils::ipFamilyTypeToString(telux::data::IpFamilyType ipType) {
   switch(ipType) {
      case telux::data::IpFamilyType::IPV4:
         return "IPv4";
      case telux::data::IpFamilyType::IPV6:
         return "IPv6";
      case telux::data::IpFamilyType::IPV4V6:
         return "IPv4v6";
      case telux::data::IpFamilyType::UNKNOWN:
      default:
         return "NA";
   }
}

std::string DataUtils::operationTypeToString(telux::data::OperationType oprType) {
   switch(oprType) {
      case telux::data::OperationType::DATA_LOCAL:
         return "LOCAL";
      case telux::data::OperationType::DATA_REMOTE:
         return "REMOTE";
      default:
         return "NA";
   }
}

std::string DataUtils::callEndReasonTypeToString(telux::data::EndReasonType type) {
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

int DataUtils::callEndReasonCode(telux::data::DataCallEndReason ceReason) {
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

std::string DataUtils::dataCallStatusToString(telux::data::DataCallStatus dcStatus) {
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
      default: { return "UNKNOWN"; }
   }
}

std::string DataUtils::bearerTechToString(telux::data::DataBearerTechnology bearerTech) {
   switch(bearerTech) {
      case telux::data::DataBearerTechnology::CDMA_1X:
         return "1X technology";
      case telux::data::DataBearerTechnology::EVDO_REV0:
         return "CDMA Rev 0";
      case telux::data::DataBearerTechnology::EVDO_REVA:
         return "CDMA Rev A";
      case telux::data::DataBearerTechnology::EVDO_REVB:
         return "CDMA Rev B";
      case telux::data::DataBearerTechnology::EHRPD:
         return "EHRPD";
      case telux::data::DataBearerTechnology::FMC:
         return "Fixed mobile convergence";
      case telux::data::DataBearerTechnology::HRPD:
         return "HRPD";
      case telux::data::DataBearerTechnology::BEARER_TECH_3GPP2_WLAN:
         return "3GPP2 IWLAN";
      case telux::data::DataBearerTechnology::WCDMA:
         return "WCDMA";
      case telux::data::DataBearerTechnology::GPRS:
         return "GPRS";
      case telux::data::DataBearerTechnology::HSDPA:
         return "HSDPA";
      case telux::data::DataBearerTechnology::HSUPA:
         return "HSUPA";
      case telux::data::DataBearerTechnology::EDGE:
         return "EDGE";
      case telux::data::DataBearerTechnology::LTE:
         return "LTE";
      case telux::data::DataBearerTechnology::HSDPA_PLUS:
         return "HSDPA+";
      case telux::data::DataBearerTechnology::DC_HSDPA_PLUS:
         return "DC HSDPA+.";
      case telux::data::DataBearerTechnology::HSPA:
         return "HSPA";
      case telux::data::DataBearerTechnology::BEARER_TECH_64_QAM:
         return "64 QAM";
      case telux::data::DataBearerTechnology::TDSCDMA:
         return "TDSCDMA";
      case telux::data::DataBearerTechnology::GSM:
         return "GSM";
      case telux::data::DataBearerTechnology::BEARER_TECH_3GPP_WLAN:
         return "3GPP WLAN";
      case telux::data::DataBearerTechnology::BEARER_TECH_5G:
         return "5G";
      default: { return "UNKNOWN"; }
   }
}

std::string DataUtils::protocolToString(telux::data::IpProtocol proto) {
   switch(proto) {
      case 1:
         return "ICMP";
      case 2:
         return "IGMP";
      case 6:
         return "TCP";
      case 17:
         return "UDP";
      case 50:
         return "ESP";
      default: {
         return "Unknown";
      }
   }
}

telux::data::IpProtocol DataUtils::getProtcol(std::string protoStr) {
    std::string protoStrToCompare = protoStr;
    std::transform(protoStrToCompare.begin(), protoStrToCompare.end(), protoStrToCompare.begin(),
        [](unsigned char ch) { return std::tolower(ch); });

    telux::data::IpProtocol prot = 0;
    if(protoStrToCompare.compare("udp") == 0) {
        prot = 17;
    }
    else if (protoStrToCompare.compare("tcp") == 0) {
        prot = 6;
    }
    else if (protoStrToCompare.compare("igmp") == 0) {
        prot = 2;
    }
    else if (protoStrToCompare.compare("icmp") == 0) {
        prot = 1;
    }
    else if (protoStrToCompare.compare("esp") == 0) {
        prot = 50;
    }
    else if (protoStrToCompare.compare("tcp_udp") == 0) {
        prot = 253;
    }
    else if (protoStrToCompare.compare("icmp6") == 0) {
        prot = 58;
    }
    else {
        std::cout << "Error: invalid protocol \n ";
    }
    return prot;
}

std::string DataUtils::drbStatusToString(telux::data::DrbStatus stat) {
    std::string statusStr = "UNKNOWN";
    switch (stat) {
        case telux::data::DrbStatus::DORMANT:
            statusStr = "DORMANT";
            break;
        case telux::data::DrbStatus::ACTIVE:
            statusStr = "ACTIVE";
            break;
        case telux::data::DrbStatus::UNKNOWN:
        default:
            break;
    }
    return statusStr;
}

std::string DataUtils::serviceRatToString(telux::data::NetworkRat rat) {
    std::string ratStr = "UNKNOWN";
    switch (rat) {
        case telux::data::NetworkRat::CDMA_1X:
            ratStr = "CDMA 1X";
            break;
        case telux::data::NetworkRat::CDMA_EVDO:
            ratStr = "CDMA EVDO";
            break;
        case telux::data::NetworkRat::GSM:
            ratStr = "GSM";
            break;
        case telux::data::NetworkRat::WCDMA:
            ratStr = "WCDMA";
            break;
        case telux::data::NetworkRat::LTE:
            ratStr = "LTE";
            break;
        case telux::data::NetworkRat::TDSCDMA:
            ratStr = "TDSCDMA";
            break;
        case telux::data::NetworkRat::NR5G:
            ratStr = "NR5G";
            break;
        default:
            break;
    }
    return ratStr;
}

std::string DataUtils::backhaulToString(telux::data::BackhaulType backhaul) {
   std::string retString = "UNKNOWN";
   switch(backhaul) {
      case telux::data::BackhaulType::ETH:
         retString = "ETH";
         break;
      case telux::data::BackhaulType::USB:
         retString = "USB";
         break;
      case telux::data::BackhaulType::WLAN:
         retString = "WLAN";
         break;
      case telux::data::BackhaulType::WWAN:
         retString = "WWAN";
         break;
      case telux::data::BackhaulType::BLE:
         retString = "BLE";
         break;
      default:
         break;
   }
   return retString;
}

//Retuns true if multiple backhauls are supported
bool DataUtils::populateBackhaulInfo(telux::data::BackhaulInfo& backhaulInfo) {
    int isMultiBackhauls = 1;
    std::cout << "Multiple Backhaul Supported? (0-No, 1-Yes): ";
    std::cin >> isMultiBackhauls;
    Utils::validateInput(isMultiBackhauls);
    std::cout << std::endl;

    int backhaul = 0, profileId = 0;
    if(isMultiBackhauls) {
      std::cout << "Enter Backhaul Type (0-Wlan, 1-WWAN): ";
      std::cin >> backhaul;
      Utils::validateInput(backhaul);
      std::cout << std::endl;
      if(backhaul) {
         backhaulInfo.backhaul = telux::data::BackhaulType::WWAN;
         std::cout << "Enter Profile Id: ";
         std::cin >> profileId;
         Utils::validateInput(profileId);
         backhaulInfo.profileId = profileId;
      } else {
         backhaulInfo.backhaul = telux::data::BackhaulType::WLAN;
      }
      return true;
    } else {
      std::cout << "Enter Profile Id: ";
      std::cin >> profileId;
      Utils::validateInput(profileId);
      backhaulInfo.profileId = profileId;
      return false;
    }
}
