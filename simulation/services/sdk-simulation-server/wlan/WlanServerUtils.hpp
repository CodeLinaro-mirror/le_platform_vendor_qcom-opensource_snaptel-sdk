/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef WLAN_SERVER_UTILS_HPP
#define WLAN_SERVER_UTILS_HPP

#include <string>
#include <telux/wlan/ApInterfaceManager.hpp>
#include <telux/wlan/StaInterfaceManager.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>
#include <vector>

#include "protos/proto-src/wlan_simulation.pb.h"
#include <jsoncpp/json/json.h>

namespace telux {
namespace wlan {
namespace server {

class WlanServerUtils {
public:
  // --- Telux ID Conversions ---
  static std::string getTeluxIdString(telux::wlan::Id id);
  static wlanStub::Id convertIdToGrpc(telux::wlan::Id id);
  static telux::wlan::Id convertIdFromGrpc(wlanStub::Id id);
  static wlanStub::Id getTeluxIdFromString(const std::string &idStr);

  // --- BandType Conversions ---
  static std::string getBandTypeString(telux::wlan::BandType type);
  static wlanStub::BandType getBandTypeFromString(const std::string &str);
  static telux::wlan::BandType convertBandTypeFromGrpc(wlanStub::BandType type);
  static wlanStub::BandType convertBandTypeToGrpc(telux::wlan::BandType type);

  // --- ApType Conversions ---
  static std::string getApTypeString(telux::wlan::ApType type);
  static wlanStub::ApType getApTypeFromString(const std::string &str);

  // --- SecMode Conversions ---
  static std::string getSecModeString(telux::wlan::SecMode mode);
  static wlanStub::SecMode getSecModeFromString(const std::string &str);

  // --- SecAuth Conversions ---
  static std::string getSecAuthString(telux::wlan::SecAuth auth);
  static wlanStub::SecAuth getSecAuthFromString(const std::string &str);

  // --- SecEncrypt Conversions ---
  static std::string getSecEncryptString(telux::wlan::SecEncrypt encrypt);
  static wlanStub::SecEncrypt getSecEncryptFromString(const std::string &str);

  // --- NetAccessType Conversions ---
  static std::string getNetAccessTypeString(telux::wlan::NetAccessType type);
  static wlanStub::NetAccessType
  getNetAccessTypeFromString(const std::string &str);

  // --- ApInterworking Conversions ---
  static std::string getApInterworkingString(telux::wlan::ApInterworking type);
  static wlanStub::ApInterworking
  getApInterworkingFromString(const std::string &str);

  // --- HwDeviceType Conversions ---
  static std::string getHwDeviceTypeString(telux::wlan::HwDeviceType type);
  static wlanStub::HwDeviceType
  getHwDeviceTypeFromString(const std::string &str);

  // --- DevicePerfState Conversions ---
  static std::string
  getDevicePerfStateString(telux::wlan::DevicePerfState state);
  static wlanStub::DevicePerfState
  getDevicePerfStateFromString(const std::string &str);
  static telux::wlan::DevicePerfState
  convertDevicePerfStateFromGrpc(wlanStub::DevicePerfState state);

  // --- RegulatoryParams Conversions (JSON <-> Protobuf) ---
  static wlanStub::RegulatoryParams
  getRegulatoryParamsFromPtree(const Json::Value &node);
  static Json::Value
  setRegulatoryParamsToJson(const wlanStub::RegulatoryParams &params);

  // --- ApStatus Conversions (JSON <-> Protobuf) ---
  static wlanStub::ApStatus getApStatusFromPtree(const Json::Value &node);
  static Json::Value setApStatusToJson(const wlanStub::ApStatus &status);

  // --- StaStatus Conversions (JSON <-> Protobuf) ---
  static wlanStub::StaStatus getStaStatusFromPtree(const Json::Value &node);
  static Json::Value setStaStatusToJson(const wlanStub::StaStatus &status);

  // --- InterfaceStatus Conversions (JSON <-> Protobuf) ---
  static wlanStub::InterfaceStatus
  getInterfaceStatusFromPtree(const Json::Value &node);
  static Json::Value
  setInterfaceStatusToJson(const wlanStub::InterfaceStatus &status);

  // --- StaIpConfig Conversions ---
  static std::string getStaIpConfigString(telux::wlan::StaIpConfig config);
  static wlanStub::StaIpConfig getStaIpConfigFromString(const std::string &str);
  static telux::wlan::StaIpConfig
  convertStaIpConfigFromGrpc(wlanStub::StaIpConfig config);

  // --- StaBridgeMode Conversions ---
  static std::string getStaBridgeModeString(telux::wlan::StaBridgeMode mode);
  static wlanStub::StaBridgeMode
  getStaBridgeModeFromString(const std::string &str);
  static telux::wlan::StaBridgeMode
  convertStaBridgeModeFromGrpc(wlanStub::StaBridgeMode mode);

  // --- StaInterfaceStatus Conversions ---
  static std::string
  getStaInterfaceStatusString(telux::wlan::StaInterfaceStatus status);
  static wlanStub::StaInterfaceStatus
  getStaInterfaceStatusFromString(const std::string &str);
  static telux::wlan::StaInterfaceStatus
  convertStaInterfaceStatusFromGrpc(wlanStub::StaInterfaceStatus status);

  // --- StaConnectionStatus Conversions ---
  static std::string
  getStaConnectionStatusString(telux::wlan::StaConnectionStatus status);
  static wlanStub::StaConnectionStatus
  getStaConnectionStatusFromString(const std::string &str);
  static telux::wlan::StaConnectionStatus
  convertStaConnectionStatusFromGrpc(wlanStub::StaConnectionStatus status);

  // --- StaStaticIpConfig Conversions (JSON <-> Protobuf) ---
  static wlanStub::StaStaticIpConfig
  getStaStaticIpConfigFromPtree(const Json::Value &node);
  static Json::Value
  setStaStaticIpConfigToJson(const wlanStub::StaStaticIpConfig &config);

  // --- StaNetworkConfig Conversions (JSON <-> Protobuf) ---
  static wlanStub::StaNetworkConfig
  getStaNetworkConfigFromPtree(const Json::Value &node);
  static Json::Value
  setStaNetworkConfigToJson(const wlanStub::StaNetworkConfig &config);

  // --- StaScanResult Conversions (JSON <-> Protobuf) ---
  static wlanStub::StaScanResult
  getStaScanResultFromPtree(const Json::Value &node);
  static Json::Value
  setStaScanResultToJson(const wlanStub::StaScanResult &result);

  // --- Utility Functions ---
  // Finds a STA status entry in a JSON array by its Telux ID.
  static Json::Value *findStaStatusEntry(Json::Value &staStatusArray,
                                         telux::wlan::Id staId);
  // Sends an unsolicited OnStaStatusChanged event.
  static void sendStaStatusChangedEvent(
      telux::wlan::Id staId, telux::wlan::StaInterfaceStatus status,
      telux::wlan::StaConnectionStatus connStatus, const std::string &name,
      const std::string &ipv4, const std::string &mac);

  // Utility Functions for Network Interface Details
  static std::string getIpv4Address(const std::string &ifaceName);
  static std::string getIpv6Address(const std::string &ifaceName);
  static std::string getMacAddress(const std::string &ifaceName);
  static std::vector<std::string>
  getInterfaceNames(const std::string &configKey);

  // Helper function to get Telux StaInterfaceStatus string from its integer
  // string value.
  static std::string
  getStaInterfaceStatusStringFromInt(const std::string &intStr);

  // Helper function to get Telux StaConnectionStatus string from its integer
  // string value.
  static std::string
  getStaConnectionStatusStringFromInt(const std::string &intStr);

  // Helper function to send an OnStaStatusChanged event.
  static void sendStaStatusChangedEvent(
      telux::wlan::Id staId, telux::wlan::StaInterfaceStatus status,
      telux::wlan::StaConnectionStatus connStatus, const std::string &name,
      const std::string &ipv4, const std::string &mac, const std::string &ipv6);
};

} // namespace server
} // namespace wlan
} // namespace telux

#endif // WLAN_SERVER_UTILS_HPP