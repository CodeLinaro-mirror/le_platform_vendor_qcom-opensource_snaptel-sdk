/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * @file       WlanUtils.cpp
 *
 * @brief      This class class performs common functions in Wlan.
 */

#include <iostream>
#include "WlanUtils.hpp"

std::string WlanUtils::getWlanDeviceName(telux::wlan::HwDeviceType device) {
   std::string retStr;

   switch(device) {
      case telux::wlan::HwDeviceType::UNKNOWN:
         retStr = "UNKNOWN";
         break;
      case telux::wlan::HwDeviceType::QCA6574:
         retStr = "QCA6574";
         break;
      case telux::wlan::HwDeviceType::QCA6696:
         retStr = "QCA6696";
         break;
      case telux::wlan::HwDeviceType::QCA6595:
         retStr = "QCA6595";
         break;
      default:
         retStr = "CUSTOM";
         break;
   }
   return retStr;
}

std::string WlanUtils::getWlanApType(telux::wlan::ApType apType) {
   std::string retStr;
   switch(apType) {
      case telux::wlan::ApType::UNKNOWN:
         retStr = "UNKNOWN";
         break;
      case telux::wlan::ApType::PRIVATE:
         retStr = "PRIVATE AP";
         break;
      case telux::wlan::ApType::GUEST:
         retStr = "GUEST AP";
         break;
      default:
         break;
   }
   return retStr;
}

std::string WlanUtils::getWlanId(telux::wlan::Id id) {
   std::string retStr;
   switch(id) {
      case telux::wlan::Id::PRIMARY:
         retStr = "PRIMARY";
         break;
      case telux::wlan::Id::SECONDARY:
         retStr = "SECONDARY";
         break;
      case telux::wlan::Id::TERTIARY:
         retStr = "TERTIARY";
         break;
      case telux::wlan::Id::QUATERNARY:
         retStr = "QUATERNARY";
         break;
   }
   return retStr;
}

std::string WlanUtils::getStaConnectionStatus(telux::wlan::StaInterfaceStatus status) {
   std::string retStr = "";
   switch(status) {
      case telux::wlan::StaInterfaceStatus::UNKNOWN:
         retStr = "UNKNOWN";
         break;
      case telux::wlan::StaInterfaceStatus::CONNECTING:
         retStr = "CONNECTING";
         break;
      case telux::wlan::StaInterfaceStatus::CONNECTED:
         retStr = "CONNECTED";
         break;
      case telux::wlan::StaInterfaceStatus::DISCONNECTED:
         retStr = "DISCONNECTED";
         break;
      case telux::wlan::StaInterfaceStatus::ASSOCIATION_FAILED:
         retStr = "ASSOCIATION_FAILED";
         break;
      case telux::wlan::StaInterfaceStatus::IP_ASSIGNMENT_FAILED:
         retStr = "IP_ASSIGNMENT_FAILED";
         break;
      default:
         break;
   }
   return retStr;
}

void WlanUtils::printAPStatus(std::vector<telux::wlan::ApStatus>& apStatus) {
   if(apStatus.size() > 0) {
       std::cout << "List of APs:" << std::endl;
       for(auto& ap:apStatus) {
           std::cout << "--------------------------------------------" << std::endl;
           std::cout << "Id                 : " << WlanUtils::getWlanId(ap.id) << std::endl;
           std::cout << "Network Interface  : " << ap.name << std::endl;
           std::cout << "IPv4 Addr          : " << ap.ipv4Address << std::endl;
           std::cout << "MAC Addr           : " << ap.macAddress << std::endl;
           for(auto& netInfo:ap.network) {
               std::cout << "AP Type            : "
                         << WlanUtils::getWlanApType(netInfo.info.apType) << std::endl;
           }
           std::cout << std::endl;
       }
   } else {
       std::cout << "No AP is currently active" << std::endl;
   }
}

void WlanUtils::printStaStatus(std::vector<telux::wlan::StaStatus>& staStatus) {
   if(staStatus.size() > 0) {
       std::cout << "List of Stations:" << std::endl;
       for(auto& sta:staStatus) {
           std::cout << "--------------------------------------------" << std::endl;
           std::cout << "Id                : " << WlanUtils::getWlanId(sta.id) << std::endl;
           std::cout << "Network Interface : " << sta.name << std::endl;
           std::cout << "IPv4 Addr         : " << sta.ipv4Address << std::endl;
           std::cout << "IPv6 Addr         : " << sta.ipv6Address << std::endl;
           std::cout << "MAC Addr          : " << sta.macAddress  << std::endl;
           std::cout << "Status            : "
                     << WlanUtils::getStaConnectionStatus(sta.status) << std::endl;
       }
       std::cout << std::endl;
   } else {
       std::cout << "No Station is currently active" << std::endl;
   }
}

void WlanUtils::printDeviceInfo(std::vector<telux::wlan::DeviceInfo>& info) {
   if(info.size() > 0) {
       std::cout << "List of connected devices:" << std::endl;
       for(auto& dev:info) {
           std::cout << "----------------------------------------------" << std::endl;
           std::cout << "Associated AP       : " << WlanUtils::getWlanId(dev.id) << std::endl;
           std::cout << "Device Name         : " << dev.name << std::endl;
           std::cout << "Device IPv4 Address : " << dev.ipv4Address << std::endl;
           std::cout << "Device IPv6 Address : " << dev.ipv6Address << std::endl;
           std::cout << "Device MAC Address  : " << dev.macAddress << std::endl;
       }
    } else {
        std::cout << "No Devices are currently connected to any AP" << std::endl;
    }
}

