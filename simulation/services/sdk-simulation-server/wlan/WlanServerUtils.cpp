/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file
 *   This file implements the APIs to convert between protobuf and Telux Wlan
 * data types.
 */

#include "WlanServerUtils.hpp"
#include <algorithm> // For std::remove_if, std::isspace
#include <arpa/inet.h>
#include <fstream>
#include <ifaddrs.h>
#include <sstream>
#include <string>

#include "event/EventService.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/SimulationConfigParser.hpp"
#include "libs/wlan/WlanCommonUtilsStub.hpp"

namespace telux {
namespace wlan {
namespace server {

#ifndef STA_FILTER
#define STA_FILTER "wlan_sta"
#endif

// Helper function to find a STA status entry by ID within a Json::Value array.
Json::Value *WlanServerUtils::findStaStatusEntry(Json::Value &staStatusArray,
                                                 telux::wlan::Id staId) {
  for (Json::ArrayIndex i = 0; i < staStatusArray.size(); ++i) {
    if (WlanServerUtils::convertIdFromGrpc(
            WlanServerUtils::getTeluxIdFromString(
                staStatusArray[i]["id"].asString())) == staId) {
      return &staStatusArray[i];
    }
  }
  return nullptr;
}

// Helper function to send an OnStaStatusChanged event.
void WlanServerUtils::sendStaStatusChangedEvent(
    telux::wlan::Id staId, telux::wlan::StaInterfaceStatus status,
    telux::wlan::StaConnectionStatus connStatus, const std::string &name,
    const std::string &ipv4, const std::string &mac) {
  wlanStub::OnStaStatusChanged statusChangedEvent;
  telux::wlan::StaStatus teluxStaStatus;
  teluxStaStatus.id = staId;
  teluxStaStatus.name = name;
  teluxStaStatus.ipv4Address = ipv4;
  teluxStaStatus.ipv6Address =
      ""; // Simplification, not dynamically updated for now
  teluxStaStatus.macAddress = mac;
  teluxStaStatus.status = status;
  teluxStaStatus.connectionStatus = connStatus;
  *statusChangedEvent.add_sta_status() =
      telux::wlan::WlanCommonUtilsStub::convertStaStatusToGrpc(teluxStaStatus);

  ::eventService::EventResponse anyResponse;
  anyResponse.set_filter(STA_FILTER);
  anyResponse.mutable_any()->PackFrom(statusChangedEvent);
  EventService::getInstance().updateEventQueue(anyResponse);
  LOG(DEBUG, "sendStaStatusChangedEvent",
      " Posted OnStaStatusChanged event for STA ID: ", static_cast<int>(staId),
      ", status: ", WlanServerUtils::getStaInterfaceStatusString(status),
      ", connectionStatus: ",
      WlanServerUtils::getStaConnectionStatusString(connStatus));
}

// Utility function to get real network interface details
std::string WlanServerUtils::getIpv4Address(const std::string &ifaceName) {
  std::string ipAddress = "";
  struct ifaddrs *ifaceAddresses;

  if (getifaddrs(&ifaceAddresses) < 0) {
    LOG(ERROR, __FUNCTION__, " Failure in fetching n/w interfaces");
    return ipAddress;
  }
  struct ifaddrs *ifaddr;
  for (ifaddr = ifaceAddresses; ifaddr != NULL; ifaddr = ifaddr->ifa_next) {
    if (ifaddr->ifa_addr != NULL && ifaddr->ifa_addr->sa_family == AF_INET) {
      std::string currentIfName(ifaddr->ifa_name);
      if (ifaceName == currentIfName) {
        struct sockaddr_in *ipAddr = (struct sockaddr_in *)ifaddr->ifa_addr;
        char ipAddrStr[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &ipAddr->sin_addr, ipAddrStr, INET_ADDRSTRLEN);
        ipAddress = ipAddrStr;
        LOG(DEBUG, __FUNCTION__, " Found IPv4 address for interface ",
            ifaceName, ": ", ipAddress);
        break;
      }
    }
  }
  freeifaddrs(ifaceAddresses);
  return ipAddress;
}

std::string WlanServerUtils::getIpv6Address(const std::string &ifaceName) {
  std::string ipAddress = "";
  struct ifaddrs *ifaceAddresses;

  if (getifaddrs(&ifaceAddresses) < 0) {
    LOG(ERROR, __FUNCTION__, " Failure in fetching n/w interfaces");
    return ipAddress;
  }
  struct ifaddrs *ifaddr;
  for (ifaddr = ifaceAddresses; ifaddr != NULL; ifaddr = ifaddr->ifa_next) {
    if (ifaddr->ifa_addr != NULL && ifaddr->ifa_addr->sa_family == AF_INET6) {
      std::string currentIfName(ifaddr->ifa_name);
      if (ifaceName == currentIfName) {
        struct sockaddr_in6 *ipAddr = (struct sockaddr_in6 *)ifaddr->ifa_addr;
        if (!IN6_IS_ADDR_LINKLOCAL(
                &ipAddr->sin6_addr)) { // Skip link-local addresses
          char ipAddrStr[INET6_ADDRSTRLEN];
          inet_ntop(AF_INET6, &ipAddr->sin6_addr, ipAddrStr, INET6_ADDRSTRLEN);
          ipAddress = ipAddrStr;
          LOG(DEBUG, __FUNCTION__, " Found IPv6 address for interface ",
              ifaceName, ": ", ipAddress);
          break;
        }
      }
    }
  }
  freeifaddrs(ifaceAddresses);
  return ipAddress;
}

std::string WlanServerUtils::getMacAddress(const std::string &ifaceName) {
  std::string macAddress = "";
  std::string path = "/sys/class/net/" + ifaceName + "/address";
  std::ifstream macFile(path);
  if (macFile.is_open()) {
    std::getline(macFile, macAddress);
    macFile.close();
    LOG(DEBUG, __FUNCTION__, " Found MAC address for interface ", ifaceName,
        ": ", macAddress);
  } else {
    LOG(WARNING, __FUNCTION__, " Could not read MAC address for interface ",
        ifaceName, " from ", path);
  }
  return macAddress;
}

std::vector<std::string>
WlanServerUtils::getInterfaceNames(const std::string &configKey) {
  std::vector<std::string> ifaceNames;
  std::shared_ptr<SimulationConfigParser> config =
      std::make_shared<SimulationConfigParser>();
  std::string configValue = config->getValue(configKey);
  LOG(DEBUG, __FUNCTION__, " Retrieved config value for key '", configKey,
      "': '", configValue, "'");

  std::stringstream ss(configValue);
  std::string item;
  while (std::getline(ss, item, ',')) {
    item.erase(std::remove_if(item.begin(), item.end(), ::isspace),
               item.end()); // Remove whitespace
    if (!item.empty()) {
      ifaceNames.push_back(item);
      LOG(DEBUG, __FUNCTION__, " Added interface name: ", item);
    }
  }
  return ifaceNames;
}

// Converts a Telux Wlan ID enum to its string representation.
std::string WlanServerUtils::getTeluxIdString(telux::wlan::Id id) {
  std::string result;
  switch (id) {
  case telux::wlan::Id::PRIMARY:
    result = "PRIMARY";
    break;
  case telux::wlan::Id::SECONDARY:
    result = "SECONDARY";
    break;
  case telux::wlan::Id::TERTIARY:
    result = "TERTIARY";
    break;
  case telux::wlan::Id::QUATERNARY:
    result = "QUATERNARY";
    break;
  default:
    result = "PRIMARY"; // Default behavior
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux ID (enum) ", static_cast<int>(id),
      " to string: ", result);
  return result;
}

// Converts a string representation of a Wlan ID to its Protobuf enum value.
wlanStub::Id WlanServerUtils::getTeluxIdFromString(const std::string &idStr) {
  wlanStub::Id result;
  if (idStr == "PRIMARY")
    result = wlanStub::PRIMARY;
  else if (idStr == "SECONDARY")
    result = wlanStub::SECONDARY;
  else if (idStr == "TERTIARY")
    result = wlanStub::TERTIARY;
  else if (idStr == "QUATERNARY")
    result = wlanStub::QUATERNARY;
  else
    result = wlanStub::ID_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted Telux ID (string) '", idStr,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Protobuf Wlan ID enum to its Telux Wlan ID enum value.
telux::wlan::Id WlanServerUtils::convertIdFromGrpc(wlanStub::Id id) {
  telux::wlan::Id result;
  switch (id) {
  case wlanStub::PRIMARY:
    result = telux::wlan::Id::PRIMARY;
    break;
  case wlanStub::SECONDARY:
    result = telux::wlan::Id::SECONDARY;
    break;
  case wlanStub::TERTIARY:
    result = telux::wlan::Id::TERTIARY;
    break;
  case wlanStub::QUATERNARY:
    result = telux::wlan::Id::QUATERNARY;
    break;
  default:
    result = telux::wlan::Id::PRIMARY; // Default Telux enum value
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Protobuf ID (enum) ", id,
      " to Telux ID (enum): ", static_cast<int>(result));
  return result;
}

// Converts a Telux Wlan ID enum to its Protobuf Wlan ID enum value.
wlanStub::Id WlanServerUtils::convertIdToGrpc(telux::wlan::Id id) {
  wlanStub::Id result;
  switch (id) {
  case telux::wlan::Id::PRIMARY:
    result = wlanStub::PRIMARY;
    break;
  case telux::wlan::Id::SECONDARY:
    result = wlanStub::SECONDARY;
    break;
  case telux::wlan::Id::TERTIARY:
    result = wlanStub::TERTIARY;
    break;
  case telux::wlan::Id::QUATERNARY:
    result = wlanStub::QUATERNARY;
    break;
  default:
    result = wlanStub::ID_UNKNOWN;
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux ID (enum) ", static_cast<int>(id),
      " to Protobuf ID (enum): ", result);
  return result;
}

// Converts a Telux BandType enum to its string representation.
std::string WlanServerUtils::getBandTypeString(telux::wlan::BandType type) {
  std::string result;
  if (type == telux::wlan::BandType::BAND_5GHZ)
    result = "BAND_5GHZ";
  else if (type == telux::wlan::BandType::BAND_2GHZ)
    result = "BAND_2GHZ";
  else if (type == telux::wlan::BandType::BAND_6GHZ)
    result = "BAND_6GHZ";
  else
    result = "BAND_5GHZ"; // Default behavior
  LOG(DEBUG, __FUNCTION__, " Converted Telux BandType (enum) ",
      static_cast<int>(type), " to string: ", result);
  return result;
}

// Converts a string representation of a BandType to its Protobuf enum value.
wlanStub::BandType
WlanServerUtils::getBandTypeFromString(const std::string &str) {
  wlanStub::BandType result;
  if (str == "BAND_5GHZ")
    result = wlanStub::BAND_5GHZ;
  else if (str == "BAND_2GHZ")
    result = wlanStub::BAND_2GHZ;
  else if (str == "BAND_6GHZ")
    result = wlanStub::BAND_6GHZ;
  else
    result = wlanStub::BAND_TYPE_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted BandType (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Protobuf BandType enum to its Telux BandType enum value.
telux::wlan::BandType
WlanServerUtils::convertBandTypeFromGrpc(wlanStub::BandType type) {
  telux::wlan::BandType result;
  switch (type) {
  case wlanStub::BAND_5GHZ:
    result = telux::wlan::BandType::BAND_5GHZ;
    break;
  case wlanStub::BAND_2GHZ:
    result = telux::wlan::BandType::BAND_2GHZ;
    break;
  case wlanStub::BAND_6GHZ:
    result = telux::wlan::BandType::BAND_6GHZ;
    break;
  default:
    result = telux::wlan::BandType::BAND_5GHZ; // Default Telux enum value
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Protobuf BandType (enum) ", type,
      " to Telux BandType (enum): ", static_cast<int>(result));
  return result;
}

// Converts a Telux BandType enum to its Protobuf BandType enum value.
wlanStub::BandType
WlanServerUtils::convertBandTypeToGrpc(telux::wlan::BandType type) {
  wlanStub::BandType result;
  switch (type) {
  case telux::wlan::BandType::BAND_5GHZ:
    result = wlanStub::BAND_5GHZ;
    break;
  case telux::wlan::BandType::BAND_2GHZ:
    result = wlanStub::BAND_2GHZ;
    break;
  case telux::wlan::BandType::BAND_6GHZ:
    result = wlanStub::BAND_6GHZ;
    break;
  default:
    result = wlanStub::BAND_TYPE_UNKNOWN; // Default gRPC enum value
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux BandType (enum) ",
      static_cast<int>(type), " to Protobuf BandType (enum): ", result);
  return result;
}

// Converts a Telux ApType enum to its string representation.
std::string WlanServerUtils::getApTypeString(telux::wlan::ApType type) {
  std::string result;
  if (type == telux::wlan::ApType::PRIVATE)
    result = "PRIVATE";
  else if (type == telux::wlan::ApType::GUEST)
    result = "GUEST";
  else
    result = "PRIVATE"; // Default behavior
  LOG(DEBUG, __FUNCTION__, " Converted Telux ApType (enum) ",
      static_cast<int>(type), " to string: ", result);
  return result;
}

// Converts a string representation of an ApType to its Protobuf enum value.
wlanStub::ApType WlanServerUtils::getApTypeFromString(const std::string &str) {
  wlanStub::ApType result;
  if (str == "PRIVATE")
    result = wlanStub::PRIVATE;
  else if (str == "GUEST")
    result = wlanStub::GUEST;
  else
    result = wlanStub::AP_TYPE_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted ApType (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Telux SecMode enum to its string representation.
std::string WlanServerUtils::getSecModeString(telux::wlan::SecMode mode) {
  std::string result;
  if (mode == telux::wlan::SecMode::OPEN)
    result = "OPEN";
  else if (mode == telux::wlan::SecMode::WEP)
    result = "WEP";
  else if (mode == telux::wlan::SecMode::WPA)
    result = "WPA";
  else if (mode == telux::wlan::SecMode::WPA2)
    result = "WPA2";
  else if (mode == telux::wlan::SecMode::WPA3)
    result = "WPA3";
  else
    result = "OPEN"; // Default behavior
  LOG(DEBUG, __FUNCTION__, " Converted Telux SecMode (enum) ",
      static_cast<int>(mode), " to string: ", result);
  return result;
}

// Converts a string representation of a SecMode to its Protobuf enum value.
wlanStub::SecMode
WlanServerUtils::getSecModeFromString(const std::string &str) {
  wlanStub::SecMode result;
  if (str == "OPEN")
    result = wlanStub::OPEN;
  if (str == "WEP")
    result = wlanStub::WEP;
  else if (str == "WPA")
    result = wlanStub::WPA;
  else if (str == "WPA2")
    result = wlanStub::WPA2;
  else if (str == "WPA3")
    result = wlanStub::WPA3;
  else
    result = wlanStub::OPEN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted SecMode (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Telux SecAuth enum to its string representation.
std::string WlanServerUtils::getSecAuthString(telux::wlan::SecAuth auth) {
  std::string result;
  if (auth == telux::wlan::SecAuth::NONE)
    result = "NONE";
  else if (auth == telux::wlan::SecAuth::PSK)
    result = "PSK";
  else if (auth == telux::wlan::SecAuth::EAP_SIM)
    result = "EAP_SIM";
  else if (auth == telux::wlan::SecAuth::EAP_AKA)
    result = "EAP_AKA";
  else if (auth == telux::wlan::SecAuth::EAP_LEAP)
    result = "EAP_LEAP";
  else if (auth == telux::wlan::SecAuth::EAP_TLS)
    result = "EAP_TLS";
  else if (auth == telux::wlan::SecAuth::EAP_TTLS)
    result = "EAP_TTLS";
  else if (auth == telux::wlan::SecAuth::EAP_PEAP)
    result = "EAP_PEAP";
  else if (auth == telux::wlan::SecAuth::EAP_FAST)
    result = "EAP_FAST";
  else if (auth == telux::wlan::SecAuth::EAP_PSK)
    result = "EAP_PSK";
  else if (auth == telux::wlan::SecAuth::SAE)
    result = "SAE";
  else
    result = "NONE"; // Default behavior
  LOG(DEBUG, __FUNCTION__, " Converted Telux SecAuth (enum) ",
      static_cast<int>(auth), " to string: ", result);
  return result;
}

// Converts a string representation of a SecAuth to its Protobuf enum value.
wlanStub::SecAuth
WlanServerUtils::getSecAuthFromString(const std::string &str) {
  wlanStub::SecAuth result;
  if (str == "NONE")
    result = wlanStub::NONE;
  else if (str == "PSK")
    result = wlanStub::PSK;
  else if (str == "EAP_SIM")
    result = wlanStub::EAP_SIM;
  else if (str == "EAP_AKA")
    result = wlanStub::EAP_AKA;
  else if (str == "EAP_LEAP")
    result = wlanStub::EAP_LEAP;
  else if (str == "EAP_TLS")
    result = wlanStub::EAP_TLS;
  else if (str == "EAP_TTLS")
    result = wlanStub::EAP_TTLS;
  else if (str == "EAP_PEAP")
    result = wlanStub::EAP_PEAP;
  else if (str == "EAP_FAST")
    result = wlanStub::EAP_FAST;
  else if (str == "EAP_PSK")
    result = wlanStub::EAP_PSK;
  else if (str == "SAE")
    result = wlanStub::SAE;
  else
    result = wlanStub::NONE; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted SecAuth (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Telux SecEncrypt enum to its string representation.
std::string
WlanServerUtils::getSecEncryptString(telux::wlan::SecEncrypt encrypt) {
  std::string result;
  if (encrypt == telux::wlan::SecEncrypt::RC4)
    result = "RC4";
  else if (encrypt == telux::wlan::SecEncrypt::TKIP)
    result = "TKIP";
  else if (encrypt == telux::wlan::SecEncrypt::AES)
    result = "AES";
  else if (encrypt == telux::wlan::SecEncrypt::GCMP)
    result = "GCMP";
  else
    result = "SEC_ENCRYPT_UNKNOWN";
  LOG(DEBUG, __FUNCTION__, " Converted Telux SecEncrypt (enum) ",
      static_cast<int>(encrypt), " to string: ", result);
  return result;
}

// Converts a string representation of a SecEncrypt to its Protobuf enum value.
wlanStub::SecEncrypt
WlanServerUtils::getSecEncryptFromString(const std::string &str) {
  wlanStub::SecEncrypt result;
  if (str == "RC4")
    result = wlanStub::RC4;
  else if (str == "TKIP")
    result = wlanStub::TKIP;
  else if (str == "AES")
    result = wlanStub::AES;
  else if (str == "GCMP")
    result = wlanStub::GCMP;
  else
    result = wlanStub::SEC_ENCRYPT_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted SecEncrypt (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Telux NetAccessType enum to its string representation.
std::string
WlanServerUtils::getNetAccessTypeString(telux::wlan::NetAccessType type) {
  std::string result;
  if (type == telux::wlan::NetAccessType::PRIVATE)
    result = "PRIVATE_ACCESS"; // Corrected string mapping
  else if (type == telux::wlan::NetAccessType::PRIVATE_WITH_GUEST)
    result = "PRIVATE_WITH_GUEST";
  else if (type == telux::wlan::NetAccessType::CHARGEABLE_PUBLIC)
    result = "CHARGEABLE_PUBLIC";
  else if (type == telux::wlan::NetAccessType::FREE_PUBLIC)
    result = "FREE_PUBLIC";
  else if (type == telux::wlan::NetAccessType::PERSONAL_DEVICE)
    result = "PERSONAL_DEVICE";
  else if (type == telux::wlan::NetAccessType::EMERGENCY_SERVICES_ONLY)
    result = "EMERGENCY_SERVICES_ONLY";
  else if (type == telux::wlan::NetAccessType::TEST_OR_EXPERIMENTAL)
    result = "TEST_OR_EXPERIMENTAL";
  else if (type == telux::wlan::NetAccessType::WILDCARD)
    result = "WILDCARD";
  else
    result = "NET_ACCESS_TYPE_UNKNOWN";
  LOG(DEBUG, __FUNCTION__, " Converted Telux NetAccessType (enum) ",
      static_cast<int>(type), " to string: ", result);
  return result;
}

// Converts a string representation of a NetAccessType to its Protobuf enum
// value.
wlanStub::NetAccessType
WlanServerUtils::getNetAccessTypeFromString(const std::string &str) {
  wlanStub::NetAccessType result;
  if (str == "PRIVATE_ACCESS")
    result = wlanStub::PRIVATE_ACCESS;
  else if (str == "PRIVATE_WITH_GUEST")
    result = wlanStub::PRIVATE_WITH_GUEST;
  else if (str == "CHARGEABLE_PUBLIC")
    result = wlanStub::CHARGEABLE_PUBLIC;
  else if (str == "FREE_PUBLIC")
    result = wlanStub::FREE_PUBLIC;
  else if (str == "PERSONAL_DEVICE")
    result = wlanStub::PERSONAL_DEVICE;
  else if (str == "EMERGENCY_SERVICES_ONLY")
    result = wlanStub::EMERGENCY_SERVICES_ONLY;
  else if (str == "TEST_OR_EXPERIMENTAL")
    result = wlanStub::TEST_OR_EXPERIMENTAL;
  else if (str == "WILDCARD")
    result = wlanStub::WILDCARD;
  else
    result = wlanStub::NET_ACCESS_TYPE_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted NetAccessType (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Telux ApInterworking enum to its string representation.
std::string
WlanServerUtils::getApInterworkingString(telux::wlan::ApInterworking type) {
  std::string result;
  if (type == telux::wlan::ApInterworking::INTERNET_ACCESS)
    result = "INTERNET_ACCESS";
  else if (type == telux::wlan::ApInterworking::FULL_ACCESS)
    result = "FULL_ACCESS";
  else
    result = "AP_INTERWORKING_UNKNOWN";
  LOG(DEBUG, __FUNCTION__, " Converted Telux ApInterworking (enum) ",
      static_cast<int>(type), " to string: ", result);
  return result;
}

// Converts a string representation of an ApInterworking to its Protobuf enum
// value.
wlanStub::ApInterworking
WlanServerUtils::getApInterworkingFromString(const std::string &str) {
  wlanStub::ApInterworking result;
  if (str == "INTERNET_ACCESS")
    result = wlanStub::INTERNET_ACCESS;
  else if (str == "FULL_ACCESS")
    result = wlanStub::FULL_ACCESS;
  else
    result = wlanStub::AP_INTERWORKING_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted ApInterworking (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Telux HwDeviceType enum to its string representation.
std::string
WlanServerUtils::getHwDeviceTypeString(telux::wlan::HwDeviceType type) {
  std::string result;
  switch (type) {
  case telux::wlan::HwDeviceType::QCA6574:
    result = "QCA6574";
    break;
  case telux::wlan::HwDeviceType::QCA6696:
    result = "QCA6696";
    break;
  case telux::wlan::HwDeviceType::QCA6595:
    result = "QCA6595";
    break;
  default:
    result = "HW_DEV_TYPE_UNKNOWN";
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux HwDeviceType (enum) ",
      static_cast<int>(type), " to string: ", result);
  return result;
}

// Converts a string representation of an HwDeviceType to its Protobuf enum
// value.
wlanStub::HwDeviceType
WlanServerUtils::getHwDeviceTypeFromString(const std::string &str) {
  wlanStub::HwDeviceType result;
  if (str == "QCA6574")
    result = wlanStub::QCA6574;
  else if (str == "QCA6696")
    result = wlanStub::QCA6696;
  else if (str == "QCA6595")
    result = wlanStub::QCA6595;
  else
    result = wlanStub::HW_DEV_TYPE_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted HwDeviceType (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Telux DevicePerfState enum to its string representation.
std::string
WlanServerUtils::getDevicePerfStateString(telux::wlan::DevicePerfState state) {
  std::string result;
  switch (state) {
  case telux::wlan::DevicePerfState::FULL:
    result = "FULL";
    break;
  case telux::wlan::DevicePerfState::REDUCED:
    result = "REDUCED";
    break;
  case telux::wlan::DevicePerfState::SHUTDOWN:
    result = "SHUTDOWN";
    break;
  default:
    result = "DEV_PERF_STATE_UNKNOWN";
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux DevicePerfState (enum) ",
      static_cast<int>(state), " to string: ", result);
  return result;
}

// Converts a string representation of a DevicePerfState to its Protobuf enum
// value.
wlanStub::DevicePerfState
WlanServerUtils::getDevicePerfStateFromString(const std::string &str) {
  wlanStub::DevicePerfState result;
  if (str == "FULL")
    result = wlanStub::FULL;
  else if (str == "REDUCED")
    result = wlanStub::REDUCED;
  else if (str == "SHUTDOWN")
    result = wlanStub::SHUTDOWN;
  else
    result = wlanStub::DEV_PERF_STATE_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted DevicePerfState (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Protobuf DevicePerfState enum to its Telux DevicePerfState enum
// value.
telux::wlan::DevicePerfState WlanServerUtils::convertDevicePerfStateFromGrpc(
    wlanStub::DevicePerfState state) {
  telux::wlan::DevicePerfState result;
  switch (state) {
  case wlanStub::FULL:
    result = telux::wlan::DevicePerfState::FULL;
    break;
  case wlanStub::REDUCED:
    result = telux::wlan::DevicePerfState::REDUCED;
    break;
  case wlanStub::SHUTDOWN:
    result = telux::wlan::DevicePerfState::SHUTDOWN;
    break;
  default:
    result = telux::wlan::DevicePerfState::UNKNOWN;
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Protobuf DevicePerfState (enum) ", state,
      " to Telux DevicePerfState (enum): ", static_cast<int>(result));
  return result;
}

// Converts JSON data for RegulatoryParams to a Protobuf message.
wlanStub::RegulatoryParams
WlanServerUtils::getRegulatoryParamsFromPtree(const Json::Value &node) {
  wlanStub::RegulatoryParams params;
  params.set_country(node["country"].asString());
  params.set_op_channel(static_cast<float>(node["opChannel"].asDouble()));
  for (const auto &opClass : node["opClass"]) {
    params.add_op_class(static_cast<float>(opClass.asDouble()));
  }
  params.set_tx_power_mw(node["txPowerMw"].asUInt());
  LOG(DEBUG, __FUNCTION__,
      " Converted JSON to RegulatoryParams. Country: ", params.country(),
      ", OpChannel: ", params.op_channel());
  return params;
}

// Converts a Protobuf RegulatoryParams message to JSON data.
Json::Value WlanServerUtils::setRegulatoryParamsToJson(
    const wlanStub::RegulatoryParams &params) {
  Json::Value node;
  node["country"] = params.country();
  node["opChannel"] = params.op_channel();
  Json::Value opClassArray(Json::arrayValue);
  for (int i = 0; i < params.op_class_size(); ++i) {
    opClassArray.append(params.op_class(i));
  }
  node["opClass"] = opClassArray;
  node["txPowerMw"] = params.tx_power_mw();
  LOG(DEBUG, __FUNCTION__,
      " Converted RegulatoryParams to JSON. Country: ", params.country(),
      ", OpChannel: ", params.op_channel());
  return node;
}

// Converts JSON data for ApStatus to a Protobuf message.
wlanStub::ApStatus
WlanServerUtils::getApStatusFromPtree(const Json::Value &node) {
  wlanStub::ApStatus apStatus;
  apStatus.set_id(WlanServerUtils::getTeluxIdFromString(node["id"].asString()));
  apStatus.set_name(node["name"].asString());
  apStatus.set_ipv4_address(node["ipv4Address"].asString());
  apStatus.set_mac_address(node["macAddress"].asString());
  if (node.isMember("network") && node["network"].isArray()) {
    for (const auto &netInfoNode : node["network"]) {
      wlanStub::ApNetInfo grpcApNetInfo;
      if (netInfoNode.isMember("info")) {
        wlanStub::ApInfo grpcApInfo;
        grpcApInfo.set_ap_radio(WlanServerUtils::getBandTypeFromString(
            netInfoNode["info"]["apRadio"].asString()));
        grpcApInfo.set_ap_type(WlanServerUtils::getApTypeFromString(
            netInfoNode["info"]["apType"].asString()));
        *grpcApNetInfo.mutable_info() = grpcApInfo;
      }
      grpcApNetInfo.set_ssid(netInfoNode["ssid"].asString());
      *apStatus.add_network() = grpcApNetInfo;
      LOG(DEBUG, __FUNCTION__,
          " Converted JSON network config for AP ID: ", apStatus.id(),
          ", SSID: ", grpcApNetInfo.ssid());
    }
  }
  LOG(DEBUG, __FUNCTION__, " Converted JSON to ApStatus. ID: ", apStatus.id(),
      ", Name: ", apStatus.name());
  return apStatus;
}

// Converts a Protobuf ApStatus message to JSON data.
Json::Value
WlanServerUtils::setApStatusToJson(const wlanStub::ApStatus &status) {
  Json::Value node;
  node["id"] = WlanServerUtils::getTeluxIdString(
      WlanServerUtils::convertIdFromGrpc(status.id()));
  node["name"] = status.name();
  node["ipv4Address"] = status.ipv4_address();
  node["macAddress"] = status.mac_address();
  Json::Value networkArray(Json::arrayValue);
  for (int i = 0; i < status.network_size(); ++i) {
    const auto &netInfo = status.network(i);
    Json::Value netInfoNode;
    Json::Value infoNode;
    infoNode["apRadio"] = WlanServerUtils::getBandTypeString(
        WlanServerUtils::convertBandTypeFromGrpc(netInfo.info().ap_radio()));
    infoNode["apType"] = WlanServerUtils::getApTypeString(
        static_cast<telux::wlan::ApType>(netInfo.info().ap_type()));
    netInfoNode["info"] = infoNode;
    netInfoNode["ssid"] = netInfo.ssid();
    networkArray.append(netInfoNode);
    LOG(DEBUG, __FUNCTION__,
        " Converted AP network config to JSON for SSID: ", netInfo.ssid());
  }
  node["network"] = networkArray;
  LOG(DEBUG, __FUNCTION__, " Converted ApStatus to JSON. ID: ", status.id(),
      ", Name: ", status.name());
  return node;
}

// Converts JSON data for StaStatus to a Protobuf message.
wlanStub::StaStatus
WlanServerUtils::getStaStatusFromPtree(const Json::Value &node) {
  wlanStub::StaStatus staStatus;
  staStatus.set_id(
      WlanServerUtils::getTeluxIdFromString(node["id"].asString()));
  staStatus.set_name(node["name"].asString());
  staStatus.set_ipv4_address(node["ipv4Address"].asString());
  staStatus.set_ipv6_address(node["ipv6Address"].asString());
  staStatus.set_mac_address(node["macAddress"].asString());
  staStatus.set_status(WlanServerUtils::getStaInterfaceStatusFromString(
      node["status"].asString()));
  staStatus.set_connection_status(
      WlanServerUtils::getStaConnectionStatusFromString(
          node["connectionStatus"].asString()));
  LOG(DEBUG, __FUNCTION__, " Converted JSON to StaStatus. ID: ", staStatus.id(),
      ", Name: ", staStatus.name(), ", Status: ", staStatus.status());
  return staStatus;
}

// Converts a Protobuf StaStatus message to JSON data.
Json::Value
WlanServerUtils::setStaStatusToJson(const wlanStub::StaStatus &status) {
  Json::Value node;
  node["id"] = WlanServerUtils::getTeluxIdString(
      WlanServerUtils::convertIdFromGrpc(status.id()));
  node["name"] = status.name();
  node["ipv4Address"] = status.ipv4_address();
  node["ipv6Address"] = status.ipv6_address();
  node["macAddress"] = status.mac_address();
  node["status"] = WlanServerUtils::getStaInterfaceStatusString(
      WlanServerUtils::convertStaInterfaceStatusFromGrpc(status.status()));
  node["connectionStatus"] = WlanServerUtils::getStaConnectionStatusString(
      WlanServerUtils::convertStaConnectionStatusFromGrpc(
          status.connection_status()));
  LOG(DEBUG, __FUNCTION__, " Converted StaStatus to JSON. ID: ", status.id(),
      ", Name: ", status.name(), ", Status: ", status.status());
  return node;
}

// Converts JSON data for InterfaceStatus to a Protobuf message.
wlanStub::InterfaceStatus
WlanServerUtils::getInterfaceStatusFromPtree(const Json::Value &node) {
  wlanStub::InterfaceStatus ifaceStatus;
  // Cast to wlanStub::HwDeviceType as set_device expects this type
  ifaceStatus.set_device(static_cast<wlanStub::HwDeviceType>(
      WlanServerUtils::getHwDeviceTypeFromString(node["device"].asString())));

  if (node.isMember("apStatus") && node["apStatus"].isArray()) {
    for (const auto &apStatusNode : node["apStatus"]) {
      *ifaceStatus.add_ap_status() =
          WlanServerUtils::getApStatusFromPtree(apStatusNode);
    }
    LOG(DEBUG, __FUNCTION__, " Added ", node["apStatus"].size(),
        " AP statuses from JSON.");
  }
  if (node.isMember("staStatus") && node["staStatus"].isArray()) {
    for (const auto &staStatusNode : node["staStatus"]) {
      *ifaceStatus.add_sta_status() =
          WlanServerUtils::getStaStatusFromPtree(staStatusNode);
    }
    LOG(DEBUG, __FUNCTION__, " Added ", node["staStatus"].size(),
        " STA statuses from JSON.");
  }
  LOG(DEBUG, __FUNCTION__,
      " Converted JSON to InterfaceStatus. Device: ", ifaceStatus.device());
  return ifaceStatus;
}

// Converts a Protobuf InterfaceStatus message to JSON data.
Json::Value WlanServerUtils::setInterfaceStatusToJson(
    const wlanStub::InterfaceStatus &status) {
  Json::Value node;
  // Cast to telux::wlan::HwDeviceType for getHwDeviceTypeString.
  node["device"] = WlanServerUtils::getHwDeviceTypeString(
      static_cast<telux::wlan::HwDeviceType>(status.device()));

  Json::Value apStatusArray(Json::arrayValue);
  for (int i = 0; i < status.ap_status_size(); ++i) {
    apStatusArray.append(
        WlanServerUtils::setApStatusToJson(status.ap_status(i)));
  }
  node["apStatus"] = apStatusArray;
  LOG(DEBUG, __FUNCTION__, " Converted ", status.ap_status_size(),
      " AP statuses to JSON.");

  Json::Value staStatusArray(Json::arrayValue);
  for (int i = 0; i < status.sta_status_size(); ++i) {
    staStatusArray.append(
        WlanServerUtils::setStaStatusToJson(status.sta_status(i)));
  }
  node["staStatus"] = staStatusArray;
  LOG(DEBUG, __FUNCTION__, " Converted ", status.sta_status_size(),
      " STA statuses to JSON.");
  LOG(DEBUG, __FUNCTION__,
      " Converted InterfaceStatus to JSON. Device: ", status.device());
  return node;
}

// Converts a Telux StaIpConfig enum to its string representation.
std::string
WlanServerUtils::getStaIpConfigString(telux::wlan::StaIpConfig config) {
  std::string result;
  switch (config) {
  case telux::wlan::StaIpConfig::DYNAMIC_IP:
    result = "DYNAMIC_IP";
    break;
  case telux::wlan::StaIpConfig::STATIC_IP:
    result = "STATIC_IP";
    break;
  default:
    result = "STA_IP_CONFIG_UNKNOWN";
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux StaIpConfig (enum) ",
      static_cast<int>(config), " to string: ", result);
  return result;
}

// Converts a string representation of a StaIpConfig to its Protobuf enum value.
wlanStub::StaIpConfig
WlanServerUtils::getStaIpConfigFromString(const std::string &str) {
  wlanStub::StaIpConfig result;
  if (str == "DYNAMIC_IP")
    result = wlanStub::DYNAMIC_IP;
  else if (str == "STATIC_IP")
    result = wlanStub::STATIC_IP;
  else
    result = wlanStub::STA_IP_CONFIG_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted StaIpConfig (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Protobuf StaIpConfig enum to its Telux StaIpConfig enum value.
telux::wlan::StaIpConfig
WlanServerUtils::convertStaIpConfigFromGrpc(wlanStub::StaIpConfig config) {
  telux::wlan::StaIpConfig result;
  switch (config) {
  case wlanStub::DYNAMIC_IP:
    result = telux::wlan::StaIpConfig::DYNAMIC_IP;
    break;
  case wlanStub::STATIC_IP:
    result = telux::wlan::StaIpConfig::STATIC_IP;
    break;
  default:
    result = telux::wlan::StaIpConfig::DYNAMIC_IP; // Default Telux enum value
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Protobuf StaIpConfig (enum) ", config,
      " to Telux StaIpConfig (enum): ", static_cast<int>(result));
  return result;
}

// Converts a Telux StaBridgeMode enum to its string representation.
std::string
WlanServerUtils::getStaBridgeModeString(telux::wlan::StaBridgeMode mode) {
  std::string result;
  switch (mode) {
  case telux::wlan::StaBridgeMode::ROUTER:
    result = "ROUTER";
    break;
  case telux::wlan::StaBridgeMode::BRIDGE:
    result = "BRIDGE";
    break;
  default:
    result = "STA_BRIDGE_MODE_UNKNOWN";
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux StaBridgeMode (enum) ",
      static_cast<int>(mode), " to string: ", result);
  return result;
}

// Converts a string representation of a StaBridgeMode to its Protobuf enum
// value.
wlanStub::StaBridgeMode
WlanServerUtils::getStaBridgeModeFromString(const std::string &str) {
  wlanStub::StaBridgeMode result;
  if (str == "ROUTER")
    result = wlanStub::ROUTER;
  else if (str == "BRIDGE")
    result = wlanStub::BRIDGE;
  else
    result = wlanStub::ROUTER; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted StaBridgeMode (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Protobuf StaBridgeMode enum to its Telux StaBridgeMode enum value.
telux::wlan::StaBridgeMode
WlanServerUtils::convertStaBridgeModeFromGrpc(wlanStub::StaBridgeMode mode) {
  telux::wlan::StaBridgeMode result;
  switch (mode) {
  case wlanStub::ROUTER:
    result = telux::wlan::StaBridgeMode::ROUTER;
    break;
  case wlanStub::BRIDGE:
    result = telux::wlan::StaBridgeMode::BRIDGE;
    break;
  default:
    result = telux::wlan::StaBridgeMode::ROUTER; // Default Telux enum value
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Protobuf StaBridgeMode (enum) ", mode,
      " to Telux StaBridgeMode (enum): ", static_cast<int>(result));
  return result;
}

// Converts a Telux StaInterfaceStatus enum to its string representation.
std::string WlanServerUtils::getStaInterfaceStatusString(
    telux::wlan::StaInterfaceStatus status) {
  std::string result;
  switch (status) {
  case telux::wlan::StaInterfaceStatus::CONNECTING:
    result = "CONNECTING";
    break;
  case telux::wlan::StaInterfaceStatus::CONNECTED:
    result = "STA_IF_CONNECTED";
    break;
  case telux::wlan::StaInterfaceStatus::DISCONNECTED:
    result = "STA_IF_DISCONNECTED";
    break;
  case telux::wlan::StaInterfaceStatus::ASSOCIATION_FAILED:
    result = "ASSOCIATION_FAILED";
    break;
  case telux::wlan::StaInterfaceStatus::IP_ASSIGNMENT_FAILED:
    result = "IP_ASSIGNMENT_FAILED";
    break;
  default:
    result = "UNKNOWN";
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux StaInterfaceStatus (enum) ",
      static_cast<int>(status), " to string: ", result);
  return result;
}

// Converts a string representation of a StaInterfaceStatus to its Protobuf enum
// value.
wlanStub::StaInterfaceStatus
WlanServerUtils::getStaInterfaceStatusFromString(const std::string &str) {
  wlanStub::StaInterfaceStatus result;
  if (str == "CONNECTING")
    result = wlanStub::CONNECTING;
  else if (str == "STA_IF_CONNECTED")
    result = wlanStub::STA_IF_CONNECTED;
  else if (str == "STA_IF_DISCONNECTED")
    result = wlanStub::STA_IF_DISCONNECTED;
  else if (str == "ASSOCIATION_FAILED")
    result = wlanStub::ASSOCIATION_FAILED;
  else if (str == "IP_ASSIGNMENT_FAILED")
    result = wlanStub::IP_ASSIGNMENT_FAILED;
  else
    result = wlanStub::STA_IF_STATUS_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted StaInterfaceStatus (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Protobuf StaInterfaceStatus enum to its Telux StaInterfaceStatus
// enum value.
telux::wlan::StaInterfaceStatus
WlanServerUtils::convertStaInterfaceStatusFromGrpc(
    wlanStub::StaInterfaceStatus status) {
  telux::wlan::StaInterfaceStatus result;
  switch (status) {
  case wlanStub::CONNECTING:
    result = telux::wlan::StaInterfaceStatus::CONNECTING;
    break;
  case wlanStub::STA_IF_CONNECTED:
    result = telux::wlan::StaInterfaceStatus::CONNECTED;
    break;
  case wlanStub::STA_IF_DISCONNECTED:
    result = telux::wlan::StaInterfaceStatus::DISCONNECTED;
    break;
  case wlanStub::ASSOCIATION_FAILED:
    result = telux::wlan::StaInterfaceStatus::ASSOCIATION_FAILED;
    break;
  case wlanStub::IP_ASSIGNMENT_FAILED:
    result = telux::wlan::StaInterfaceStatus::IP_ASSIGNMENT_FAILED;
    break;
  default:
    result =
        telux::wlan::StaInterfaceStatus::UNKNOWN; // Default Telux enum value
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Protobuf StaInterfaceStatus (enum) ",
      status,
      " to Telux StaInterfaceStatus (enum): ", static_cast<int>(result));
  return result;
}

// Helper function to get Telux StaInterfaceStatus string from its integer
// string value.
std::string
WlanServerUtils::getStaInterfaceStatusStringFromInt(const std::string &intStr) {
  int statusInt = std::stoi(intStr);
  telux::wlan::StaInterfaceStatus statusEnum;
  switch (statusInt) {
  case 0:
    statusEnum = telux::wlan::StaInterfaceStatus::UNKNOWN;
    break;
  case 1:
    statusEnum = telux::wlan::StaInterfaceStatus::CONNECTING;
    break;
  case 2:
    statusEnum = telux::wlan::StaInterfaceStatus::CONNECTED;
    break;
  case 3:
    statusEnum = telux::wlan::StaInterfaceStatus::DISCONNECTED;
    break;
  case 4:
    statusEnum = telux::wlan::StaInterfaceStatus::ASSOCIATION_FAILED;
    break;
  case 5:
    statusEnum = telux::wlan::StaInterfaceStatus::IP_ASSIGNMENT_FAILED;
    break;
  default:
    statusEnum = telux::wlan::StaInterfaceStatus::UNKNOWN;
    break;
  }
  std::string result = getStaInterfaceStatusString(statusEnum);
  LOG(DEBUG, __FUNCTION__, " Converted StaInterfaceStatus (int string) '",
      intStr, "' to string: ", result);
  return result;
}

// Helper function to get Telux StaConnectionStatus string from its integer
// string value.
std::string WlanServerUtils::getStaConnectionStatusStringFromInt(
    const std::string &intStr) {
  int connStatusInt = std::stoi(intStr);
  telux::wlan::StaConnectionStatus connStatusEnum;
  switch (connStatusInt) {
  case 0:
    connStatusEnum = telux::wlan::StaConnectionStatus::UNKNOWN;
    break;
  case 1:
    connStatusEnum = telux::wlan::StaConnectionStatus::SUCCESS;
    break;
  case 2:
    connStatusEnum = telux::wlan::StaConnectionStatus::INCORRECT_PSK;
    break;
  case 3:
    connStatusEnum = telux::wlan::StaConnectionStatus::AP_NOT_FOUND;
    break;
  default:
    connStatusEnum = telux::wlan::StaConnectionStatus::UNKNOWN;
    break;
  }
  std::string result = getStaConnectionStatusString(connStatusEnum);
  LOG(DEBUG, __FUNCTION__, " Converted StaConnectionStatus (int string) '",
      intStr, "' to string: ", result);
  return result;
}

// Helper function to send an OnStaStatusChanged event.
// Modified to also update the state JSON.
void WlanServerUtils::sendStaStatusChangedEvent(
    telux::wlan::Id staId, telux::wlan::StaInterfaceStatus status,
    telux::wlan::StaConnectionStatus connStatus, const std::string &name,
    const std::string &ipv4, const std::string &mac,
    const std::string &ipv6) { // Added ipv6 parameter
  LOG(DEBUG, "sendStaStatusChangedEvent",
      "Received update for STA ID: ", static_cast<int>(staId),
      ", status: ", getStaInterfaceStatusString(status),
      ", connectionStatus: ", getStaConnectionStatusString(connStatus),
      ", name: ", name, ", IPv4: ", ipv4, ", MAC: ", mac, ", IPv6: ", ipv6);

  Json::Value stateRootObj;
// Ensure the WLAN_STATE_JSON path is correctly defined and accessible.
// Assuming it's defined globally or passed as a constant.
// For this context, I'll use a placeholder.
#define WLAN_STATE_JSON "system-state/wlan/IStaInterfaceManagerState.json"

  telux::common::ErrorCode readError =
      JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);
  if (readError != telux::common::ErrorCode::SUCCESS) {
    LOG(ERROR, "sendStaStatusChangedEvent",
        "Failed to read WLAN state JSON: ", static_cast<int>(readError));
    // Still attempt to send event even if state cannot be persisted for some
    // reason
  }

  Json::Value *staNode = nullptr;
  if (stateRootObj.isMember("IStaInterfaceManager") &&
      stateRootObj["IStaInterfaceManager"].isMember("staStatus") &&
      stateRootObj["IStaInterfaceManager"]["staStatus"].isArray()) {
    staNode = WlanServerUtils::findStaStatusEntry(
        stateRootObj["IStaInterfaceManager"]["staStatus"], staId);
  }

  if (staNode) {
    // Update JSON fields
    (*staNode)["id"] =
        WlanServerUtils::getTeluxIdString(staId); // Ensure ID is consistent
    (*staNode)["name"] = name;
    (*staNode)["ipv4Address"] = ipv4;
    (*staNode)["ipv6Address"] = ipv6;
    (*staNode)["macAddress"] = mac;
    (*staNode)["status"] = WlanServerUtils::getStaInterfaceStatusString(status);
    (*staNode)["connectionStatus"] =
        WlanServerUtils::getStaConnectionStatusString(connStatus);

    telux::common::ErrorCode writeError =
        JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
    if (writeError != telux::common::ErrorCode::SUCCESS) {
      LOG(ERROR, "sendStaStatusChangedEvent",
          "Failed to write WLAN state JSON: ", static_cast<int>(writeError));
    }
  } else {
    LOG(WARNING, "sendStaStatusChangedEvent", "STA ID ",
        static_cast<int>(staId),
        " not found in state JSON. Event will be sent but state not "
        "persisted.");
    // If STA node not found, we can optionally add it or just log a warning.
    // For this context, a warning is sufficient as it implies the STA should
    // already be configured.
  }

  // Prepare and send the gRPC event
  wlanStub::OnStaStatusChanged statusChangedEvent;
  telux::wlan::StaStatus teluxStaStatus;
  teluxStaStatus.id = staId;
  teluxStaStatus.name = name;
  teluxStaStatus.ipv4Address = ipv4;
  teluxStaStatus.ipv6Address = ipv6;
  teluxStaStatus.macAddress = mac;
  teluxStaStatus.status = status;
  teluxStaStatus.connectionStatus = connStatus;
  *statusChangedEvent.add_sta_status() =
      telux::wlan::WlanCommonUtilsStub::convertStaStatusToGrpc(teluxStaStatus);

  ::eventService::EventResponse anyResponse;
  anyResponse.set_filter(STA_FILTER);
  anyResponse.mutable_any()->PackFrom(statusChangedEvent);
  EventService::getInstance().updateEventQueue(anyResponse);
  LOG(DEBUG, "sendStaStatusChangedEvent",
      " Posted OnStaStatusChanged event for STA ID: ", static_cast<int>(staId),
      ", status: ", WlanServerUtils::getStaInterfaceStatusString(status),
      ", connectionStatus: ",
      WlanServerUtils::getStaConnectionStatusString(connStatus));
}

// Converts a Telux StaConnectionStatus enum to its string representation.
std::string WlanServerUtils::getStaConnectionStatusString(
    telux::wlan::StaConnectionStatus status) {
  std::string result;
  switch (status) {
  case telux::wlan::StaConnectionStatus::SUCCESS:
    result = "SUCCESS";
    break;
  case telux::wlan::StaConnectionStatus::INCORRECT_PSK:
    result = "INCORRECT_PSK";
    break;
  case telux::wlan::StaConnectionStatus::AP_NOT_FOUND:
    result = "AP_NOT_FOUND";
    break;
  default:
    result = "STA_CONN_STATUS_UNKNOWN";
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Telux StaConnectionStatus (enum) ",
      static_cast<int>(status), " to string: ", result);
  return result;
}

// Converts a string representation of a StaConnectionStatus to its Protobuf
// enum value.
wlanStub::StaConnectionStatus
WlanServerUtils::getStaConnectionStatusFromString(const std::string &str) {
  wlanStub::StaConnectionStatus result;
  if (str == "SUCCESS")
    result = wlanStub::SUCCESS;
  else if (str == "INCORRECT_PSK")
    result = wlanStub::INCORRECT_PSK;
  else if (str == "AP_NOT_FOUND")
    result = wlanStub::AP_NOT_FOUND;
  else
    result = wlanStub::STA_CONN_STATUS_UNKNOWN; // Default protobuf enum value
  LOG(DEBUG, __FUNCTION__, " Converted StaConnectionStatus (string) '", str,
      "' to Protobuf enum: ", result);
  return result;
}

// Converts a Protobuf StaConnectionStatus enum to its Telux StaConnectionStatus
// enum value.
telux::wlan::StaConnectionStatus
WlanServerUtils::convertStaConnectionStatusFromGrpc(
    wlanStub::StaConnectionStatus status) {
  telux::wlan::StaConnectionStatus result;
  switch (status) {
  case wlanStub::SUCCESS:
    result = telux::wlan::StaConnectionStatus::SUCCESS;
    break;
  case wlanStub::INCORRECT_PSK:
    result = telux::wlan::StaConnectionStatus::INCORRECT_PSK;
    break;
  case wlanStub::AP_NOT_FOUND:
    result = telux::wlan::StaConnectionStatus::AP_NOT_FOUND;
    break;
  default:
    result = telux::wlan::StaConnectionStatus::UNKNOWN; // Default Telux enum
                                                        // value
    break;
  }
  LOG(DEBUG, __FUNCTION__, " Converted Protobuf StaConnectionStatus (enum) ",
      status,
      " to Telux StaConnectionStatus (enum): ", static_cast<int>(result));
  return result;
}

// Converts JSON data for StaStaticIpConfig to a Protobuf message.
wlanStub::StaStaticIpConfig
WlanServerUtils::getStaStaticIpConfigFromPtree(const Json::Value &node) {
  wlanStub::StaStaticIpConfig config;
  config.set_ip_addr(node["ipAddr"].asString());
  config.set_gw_ip_addr(node["gwIpAddr"].asString());
  config.set_net_mask(node["netMask"].asString());
  config.set_dns_addr(node["dnsAddr"].asString());
  LOG(DEBUG, __FUNCTION__,
      " Converted JSON to StaStaticIpConfig. IP: ", config.ip_addr(),
      ", GW: ", config.gw_ip_addr());
  return config;
}

// Converts a Protobuf StaStaticIpConfig message to JSON data.
Json::Value WlanServerUtils::setStaStaticIpConfigToJson(
    const wlanStub::StaStaticIpConfig &config) {
  Json::Value node;
  node["ipAddr"] = config.ip_addr();
  node["gwIpAddr"] = config.gw_ip_addr();
  node["netMask"] = config.net_mask();
  node["dnsAddr"] = config.dns_addr();
  LOG(DEBUG, __FUNCTION__,
      " Converted StaStaticIpConfig to JSON. IP: ", config.ip_addr(),
      ", GW: ", config.gw_ip_addr());
  return node;
}

// Converts JSON data for StaNetworkConfig to a Protobuf message.
wlanStub::StaNetworkConfig
WlanServerUtils::getStaNetworkConfigFromPtree(const Json::Value &node) {
  wlanStub::StaNetworkConfig config;
  config.set_ssid(node["ssid"].asString());
  config.set_priority(node["priority"].asUInt());
  config.set_band(
      WlanServerUtils::getBandTypeFromString(node["band"].asString()));
  config.set_bssid(node["bssid"].asString());
  LOG(DEBUG, __FUNCTION__,
      " Converted JSON to StaNetworkConfig. SSID: ", config.ssid(),
      ", Band: ", config.band());
  return config;
}

// Converts a Protobuf StaNetworkConfig message to JSON data.
Json::Value WlanServerUtils::setStaNetworkConfigToJson(
    const wlanStub::StaNetworkConfig &config) {
  Json::Value node;
  node["ssid"] = config.ssid();
  node["priority"] = config.priority();
  node["band"] = WlanServerUtils::getBandTypeString(
      WlanServerUtils::convertBandTypeFromGrpc(config.band()));
  node["bssid"] = config.bssid();
  LOG(DEBUG, __FUNCTION__,
      " Converted StaNetworkConfig to JSON. SSID: ", config.ssid(),
      ", Band: ", config.band());
  return node;
}

// Converts JSON data for StaScanResult to a Protobuf message.
wlanStub::StaScanResult
WlanServerUtils::getStaScanResultFromPtree(const Json::Value &node) {
  wlanStub::StaScanResult result;
  result.set_sta_id(
      WlanServerUtils::getTeluxIdFromString(node["staId"].asString()));
  result.set_batch_index(node["batchIndex"].asUInt());
  result.set_is_scan_complete(node["isScanComplete"].asBool());

  if (node.isMember("externalApList") && node["externalApList"].isArray()) {
    for (const auto &apNode : node["externalApList"]) {
      wlanStub::ExternalApInfo apInfo;
      apInfo.set_ssid(apNode["ssid"].asString());
      apInfo.set_bssid(apNode["bssid"].asString());
      apInfo.set_band(
          WlanServerUtils::getBandTypeFromString(apNode["band"].asString()));
      apInfo.set_security_flags(apNode["security_flags"].asString());
      apInfo.set_signal_strength(apNode["signal_strength"].asInt());
      *result.add_external_ap_list() = apInfo;
      LOG(DEBUG, __FUNCTION__,
          " Added AP to scan result. SSID: ", apInfo.ssid(),
          ", BSSID: ", apInfo.bssid());
    }
  }
  LOG(DEBUG, __FUNCTION__,
      " Converted JSON to StaScanResult for STA ID: ", result.sta_id(),
      ", Scan Complete: ", result.is_scan_complete());
  return result;
}

// Converts a Protobuf StaScanResult message to JSON data.
Json::Value
WlanServerUtils::setStaScanResultToJson(const wlanStub::StaScanResult &result) {
  Json::Value node;
  node["staId"] = WlanServerUtils::getTeluxIdString(
      WlanServerUtils::convertIdFromGrpc(result.sta_id()));
  node["batchIndex"] = result.batch_index();
  node["isScanComplete"] = result.is_scan_complete();

  Json::Value externalApListArray(Json::arrayValue);
  for (int i = 0; i < result.external_ap_list_size(); ++i) {
    Json::Value apNode;
    apNode["ssid"] = result.external_ap_list(i).ssid();
    apNode["bssid"] = result.external_ap_list(i).bssid();
    apNode["band"] = WlanServerUtils::getBandTypeString(
        WlanServerUtils::convertBandTypeFromGrpc(
            result.external_ap_list(i).band()));
    apNode["security_flags"] = result.external_ap_list(i).security_flags();
    apNode["signal_strength"] = result.external_ap_list(i).signal_strength();
    externalApListArray.append(apNode);
    LOG(DEBUG, __FUNCTION__, " Converted AP to JSON for scan result. SSID: ",
        apNode["ssid"].asString(), ", BSSID: ", apNode["bssid"].asString());
  }
  node["externalApList"] = externalApListArray;
  LOG(DEBUG, __FUNCTION__,
      " Converted StaScanResult to JSON for STA ID: ", result.sta_id(),
      ", Scan Complete: ", result.is_scan_complete());
  return node;
}

} // namespace server
} // namespace wlan
} // namespace telux