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
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <iomanip>

#include "DataUtils.hpp"
#include "../../common/utils/Utils.hpp"

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

std::string DataUtils::networkModuleToString(telux::data::DataStallNetworkModule module) {
    switch (module) {
        case telux::data::DataStallNetworkModule::ETHERNET: return "ETHERNET";
        case telux::data::DataStallNetworkModule::IPA: return "IPA";
        case telux::data::DataStallNetworkModule::WWAN: return "WWAN";
        case telux::data::DataStallNetworkModule::WLAN: return "WLAN";
        default: return "UNKNOWN";
    }
}

std::string DataUtils::stallReasonToString(telux::data::DataStallReason reason) {
    switch (reason) {
        case telux::data::DataStallReason::INVALID: return "INVALID";
        case telux::data::DataStallReason::Q6_DATA_STALL: return "Q6_DATA_STALL";
        case telux::data::DataStallReason::ETH_IPA_DATA_STALL: return "ETH_IPA_DATA_STALL";
        case telux::data::DataStallReason::ETH_SW_DATA_STALL: return "ETH_SW_DATA_STALL";
        case telux::data::DataStallReason::ETH_MAC_HW_DATA_STALL: return "ETH_MAC_HW_DATA_STALL";
        case telux::data::DataStallReason::IPA_ETH_DATA_STALL: return "IPA_ETH_DATA_STALL";
        case telux::data::DataStallReason::IPA_AP_DATA_STALL: return "IPA_AP_DATA_STALL";
        case telux::data::DataStallReason::IPA_STA_DATA_STALL: return "IPA_STA_DATA_STALL";
        case telux::data::DataStallReason::IPA_WLAN_DATA_STALL: return "IPA_WLAN_DATA_STALL";
        case telux::data::DataStallReason::IPA_CONF_ETH_DATA_STALL: return "IPA_CONF_ETH_DATA_STALL";
        case telux::data::DataStallReason::IPA_CONF_DATA_STALL: return "IPA_CONF_DATA_STALL";
        case telux::data::DataStallReason::WLAN_AP_DATA_STALL: return "WLAN_AP_DATA_STALL";
        case telux::data::DataStallReason::WLAN_STA_DATA_STALL: return "WLAN_STA_DATA_STALL";
        default: return "UNKNOWN";
    }
}

std::string DataUtils::recoveryActionToString(telux::data::DataStallRecoveryAction action) {
    switch (action) {
        case telux::data::DataStallRecoveryAction::NO_ACTION: return "NO_ACTION";
        case telux::data::DataStallRecoveryAction::LPM_TOGGLE: return "LPM_TOGGLE";
        case telux::data::DataStallRecoveryAction::MODEM_SSR: return "MODEM_SSR";
        case telux::data::DataStallRecoveryAction::ETH_IPA_SUSPEND_RESUME: return "ETH_IPA_SUSPEND_RESUME";
        case telux::data::DataStallRecoveryAction::ETH_SW_TX_CHNL_REINIT: return "ETH_SW_TX_CHNL_REINIT";
        case telux::data::DataStallRecoveryAction::ETH_SW_RX_CHNL_REINIT: return "ETH_SW_RX_CHNL_REINIT";
        case telux::data::DataStallRecoveryAction::ETH_REINIT: return "ETH_REINIT";
        case telux::data::DataStallRecoveryAction::WLAN_RECONNECT: return "WLAN_RECONNECT";
        case telux::data::DataStallRecoveryAction::WLAN_AP_RECONNECT: return "WLAN_AP_RECONNECT";
        case telux::data::DataStallRecoveryAction::WLAN_STA_RECONNECT: return "WLAN_STA_RECONNECT";
        case telux::data::DataStallRecoveryAction::WLAN_DRIVER_RELOAD: return "WLAN_DRIVER_RELOAD";
        case telux::data::DataStallRecoveryAction::IPACM_RESTART: return "IPACM_RESTART";
        default: return "UNKNOWN";
    }
}

std::string DataUtils::recoveryResultToString(telux::data::DataStallRecoveryResult result) {
    switch (result) {
        case telux::data::DataStallRecoveryResult::INVALID: return "No recovery action";
        case telux::data::DataStallRecoveryResult::RECOVERY_SUCCESS:
            return "Recovery action is success and data stall is recovered";
        case telux::data::DataStallRecoveryResult::RECOVERY_ACTION_FAILURE:
            return "Recovery action is failed and data stall is not recovered";
        case telux::data::DataStallRecoveryResult::RECOVERY_ACTION_SUCCESS:
            return "Recovery action is success but data stall is not recovered";
        default: return "UNKNOWN";
    }
}

std::string DataUtils::disablementReasonToString(telux::data::DataStallDisablementReason reason) {
    switch (reason) {
        case telux::data::DataStallDisablementReason::INVALID: return "INVALID";
        case telux::data::DataStallDisablementReason::MAX_INSTANCE: return "MAX_INSTANCE";
        case telux::data::DataStallDisablementReason::DURATION_EXPIRY: return "DURATION_EXPIRY";
        case telux::data::DataStallDisablementReason::USER_TRIGGERED: return "USER_TRIGGERED";
        case telux::data::DataStallDisablementReason::FACTORY_RESET: return "FACTORY_RESET";
        case telux::data::DataStallDisablementReason::ETH_DISABLED: return "ETH_DISABLED";
        case telux::data::DataStallDisablementReason::WLAN_DISABLED: return "WLAN_DISABLED";
        default: return "UNKNOWN";
    }
}

std::string DataUtils::restartTimerStatusToString(telux::data::DataStallRestartTimerStatus status) {
    switch (status) {
        case telux::data::DataStallRestartTimerStatus::INVALID: return "INVALID";
        case telux::data::DataStallRestartTimerStatus::STARTED: return "STARTED";
        case telux::data::DataStallRestartTimerStatus::INTERNAL_ERROR: return "INTERNAL_ERROR";
        case telux::data::DataStallRestartTimerStatus::DURATION_NOT_SUPPORTED: return "DURATION_NOT_SUPPORTED";
        case telux::data::DataStallRestartTimerStatus::STOPPED: return "STOPPED";
        case telux::data::DataStallRestartTimerStatus::EXPIRED: return "EXPIRED";
        default: return "UNKNOWN";
    }
}

void DataUtils::printDataStallConfig(const telux::data::DataStallConfig &cfg) {
    std::cout << "\n========================================" << std::endl;
    std::cout << "Data Stall Configuration:" << std::endl;
    std::cout << "Network Module: " << DataUtils::networkModuleToString(cfg.module) << std::endl;

    if (cfg.module == telux::data::DataStallNetworkModule::WWAN) {
        std::cout << "Slot Id: " << cfg.slotId << std::endl;
        if (cfg.profileId != -1) {
            std::cout << "Profile Id: " << cfg.profileId << std::endl;
        }
    }

    std::cout << "Enable Recovery: " << (cfg.enableRecovery ? "Yes" : "No") << std::endl;
    std::cout << "Enable Recovery Restart: " << (cfg.enableRecoveryRestart ? "Yes" : "No") << std::endl;

    if (cfg.enableRecoveryRestart) {
        std::cout << "Recovery Restart Time Duration: " << cfg.recoveryRestartTimeDuration
                  << " seconds" << std::endl;
    }

    std::cout << "Packet Stats Timer Interval: " << cfg.packetStatsTimerInterval
              << " seconds" << std::endl;

    // WWAN‑only extended detection config
    if (cfg.module == telux::data::DataStallNetworkModule::WWAN) {
        std::cout << "\nExtended Detection Config:" << std::endl;
        std::cout << "Active Connectivity Check: "
                  << (cfg.extendedConfig.activeConnCheckEnable ? "Enabled" : "Disabled")
                  << std::endl;

        if (cfg.extendedConfig.activeConnCheckEnable) {
            if (!cfg.extendedConfig.pingServerAddressV4.empty()) {
                std::cout << "IPv4 Ping Server: " << cfg.extendedConfig.pingServerAddressV4
                          << std::endl;
            }
            if (!cfg.extendedConfig.pingServerAddressV6.empty()) {
                std::cout << "IPv6 Ping Server: " << cfg.extendedConfig.pingServerAddressV6
                          << std::endl;
            }
        }

        std::cout << "TX Threshold: " << static_cast<int>(cfg.extendedConfig.txThreshold)
                  << std::endl;
        std::cout << "RX Threshold: " << static_cast<int>(cfg.extendedConfig.rxThreshold)
                  << std::endl;
    }
    std::cout << "========================================\n" << std::endl;
}
