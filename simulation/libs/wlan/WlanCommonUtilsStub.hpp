/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TELUX_WLAN_SIM_WLANCOMMONUTILSSTUB_HPP
#define TELUX_WLAN_SIM_WLANCOMMONUTILSSTUB_HPP

#include <string>
#include <vector>

#include <telux/wlan/ApInterfaceManager.hpp>
#include <telux/wlan/StaInterfaceManager.hpp>
#include <telux/wlan/WlanDefines.hpp>
#include <telux/wlan/WlanDeviceManager.hpp>

// Updated include for generated protobuf headers to reflect the new package
#include "protos/proto-src/common_simulation.pb.h" // Assuming this still uses commonStub or is not affected
#include "protos/proto-src/wlan_simulation.pb.h"

namespace telux {
namespace wlan {

/**
 * @brief Utility functions to convert between C++ SDK types and Protobuf types
 * for WLAN. This includes common types shared across WlanDevice, AP, and STA
 * managers.
 */
class WlanCommonUtilsStub {
public:
  // telux::wlan::BandType conversions
  static telux::wlan::BandType
  convertBandTypeFromGrpc(::wlanStub::BandType type);
  static ::wlanStub::BandType convertBandTypeToGrpc(telux::wlan::BandType type);

  // telux::wlan::Id conversions
  static telux::wlan::Id convertIdFromGrpc(::wlanStub::Id id);
  static ::wlanStub::Id convertIdToGrpc(telux::wlan::Id id);

  // telux::wlan::ServiceOperation conversions
  static telux::wlan::ServiceOperation
  convertServiceOperationFromGrpc(::wlanStub::ServiceOperation op);
  static ::wlanStub::ServiceOperation
  convertServiceOperationToGrpc(telux::wlan::ServiceOperation op);

  // telux::wlan::ApInfo conversions
  static telux::wlan::ApInfo
  convertApInfoFromGrpc(const ::wlanStub::ApInfo &apInfo);
  static ::wlanStub::ApInfo
  convertApInfoToGrpc(const telux::wlan::ApInfo &apInfo);

  // telux::wlan::ApNetInfo conversions
  static telux::wlan::ApNetInfo
  convertApNetInfoFromGrpc(const ::wlanStub::ApNetInfo &apNetInfo);
  static ::wlanStub::ApNetInfo
  convertApNetInfoToGrpc(const telux::wlan::ApNetInfo &apNetInfo);

  // telux::wlan::ApStatus conversions
  static telux::wlan::ApStatus
  convertApStatusFromGrpc(const ::wlanStub::ApStatus &apStatus);
  static ::wlanStub::ApStatus
  convertApStatusToGrpc(const telux::wlan::ApStatus &apStatus);

  // telux::wlan::StaStatus conversions
  static telux::wlan::StaStatus
  convertStaStatusFromGrpc(const ::wlanStub::StaStatus &staStatus);
  static ::wlanStub::StaStatus
  convertStaStatusToGrpc(const telux::wlan::StaStatus &staStatus);

  // telux::wlan::DevicePerfState conversions
  static telux::wlan::DevicePerfState
  convertDevicePerfStateFromGrpc(::wlanStub::DevicePerfState state);
  static ::wlanStub::DevicePerfState
  convertDevicePerfStateToGrpc(telux::wlan::DevicePerfState state);

  // telux::wlan::ApInterworking conversions
  static telux::wlan::ApInterworking
  convertApInterworkingFromGrpc(::wlanStub::ApInterworking interworking);
  static ::wlanStub::ApInterworking
  convertApInterworkingToGrpc(telux::wlan::ApInterworking interworking);

  // telux::wlan::SecMode conversions
  static telux::wlan::SecMode convertSecModeFromGrpc(::wlanStub::SecMode mode);
  static ::wlanStub::SecMode convertSecModeToGrpc(telux::wlan::SecMode mode);

  // telux::wlan::SecAuth conversions
  static telux::wlan::SecAuth convertSecAuthFromGrpc(::wlanStub::SecAuth auth);
  static ::wlanStub::SecAuth convertSecAuthToGrpc(telux::wlan::SecAuth auth);

  // telux::wlan::SecEncrypt conversions
  static telux::wlan::SecEncrypt
  convertSecEncryptFromGrpc(::wlanStub::SecEncrypt encrypt);
  static ::wlanStub::SecEncrypt
  convertSecEncryptToGrpc(telux::wlan::SecEncrypt encrypt);

  // telux::wlan::NetAccessType conversions
  static telux::wlan::NetAccessType
  convertNetAccessTypeFromGrpc(::wlanStub::NetAccessType type);
  static ::wlanStub::NetAccessType
  convertNetAccessTypeToGrpc(telux::wlan::NetAccessType type);

  // telux::wlan::ApSecurity conversions
  static telux::wlan::ApSecurity
  convertApSecurityFromGrpc(const ::wlanStub::ApSecurity &apSecurity);
  static ::wlanStub::ApSecurity
  convertApSecurityToGrpc(const telux::wlan::ApSecurity &apSecurity);

  // telux::wlan::ApElementInfoConfig conversions
  static telux::wlan::ApElementInfoConfig convertApElementInfoConfigFromGrpc(
      const ::wlanStub::ApElementInfoConfig &config);
  static ::wlanStub::ApElementInfoConfig convertApElementInfoConfigToGrpc(
      const telux::wlan::ApElementInfoConfig &config);

  // telux::wlan::ApNetConfig conversions
  static telux::wlan::ApNetConfig
  convertApNetConfigFromGrpc(const ::wlanStub::ApNetConfig &config);
  static ::wlanStub::ApNetConfig
  convertApNetConfigToGrpc(const telux::wlan::ApNetConfig &config);

  // telux::wlan::ApConfig conversions
  static telux::wlan::ApConfig
  convertApConfigFromGrpc(const ::wlanStub::ApConfig &config);
  static ::wlanStub::ApConfig
  convertApConfigToGrpc(const telux::wlan::ApConfig &config);

  // telux::wlan::DeviceInfo conversions
  static telux::wlan::DeviceInfo
  convertDeviceInfoFromGrpc(const ::wlanStub::DeviceInfo &info);
  static ::wlanStub::DeviceInfo
  convertDeviceInfoToGrpc(const telux::wlan::DeviceInfo &info);

  // telux::wlan::DeviceIndInfo conversions
  static telux::wlan::DeviceIndInfo
  convertDeviceIndInfoFromGrpc(const ::wlanStub::DeviceIndInfo &info);
  static ::wlanStub::DeviceIndInfo
  convertDeviceIndInfoToGrpc(const telux::wlan::DeviceIndInfo &info);

  // telux::wlan::StaIpConfig conversions
  static telux::wlan::StaIpConfig
  convertStaIpConfigFromGrpc(::wlanStub::StaIpConfig config);
  static ::wlanStub::StaIpConfig
  convertStaIpConfigToGrpc(telux::wlan::StaIpConfig config);

  // telux::wlan::StaBridgeMode conversions
  static telux::wlan::StaBridgeMode
  convertStaBridgeModeFromGrpc(::wlanStub::StaBridgeMode mode);
  static ::wlanStub::StaBridgeMode
  convertStaBridgeModeToGrpc(telux::wlan::StaBridgeMode mode);

  // telux::wlan::StaStaticIpConfig conversions
  static telux::wlan::StaStaticIpConfig
  convertStaStaticIpConfigFromGrpc(const ::wlanStub::StaStaticIpConfig &config);
  static ::wlanStub::StaStaticIpConfig
  convertStaStaticIpConfigToGrpc(const telux::wlan::StaStaticIpConfig &config);

  // telux::wlan::StaConfig conversions
  static telux::wlan::StaConfig
  convertStaConfigFromGrpc(const ::wlanStub::StaConfig &config);
  static ::wlanStub::StaConfig
  convertStaConfigToGrpc(const telux::wlan::StaConfig &config);

  // telux::wlan::StaNetworkConfig conversions
  static telux::wlan::StaNetworkConfig
  convertStaNetworkConfigFromGrpc(const ::wlanStub::StaNetworkConfig &config);
  static ::wlanStub::StaNetworkConfig
  convertStaNetworkConfigToGrpc(const telux::wlan::StaNetworkConfig &config);

  // telux::wlan::StaNetworkConfigEntry conversions
  static telux::wlan::StaNetworkConfigEntry
  convertStaNetworkConfigEntryFromGrpc(
      const ::wlanStub::StaNetworkConfigEntry &entry);
  static ::wlanStub::StaNetworkConfigEntry convertStaNetworkConfigEntryToGrpc(
      const telux::wlan::StaNetworkConfigEntry &entry);

  // telux::wlan::StaNetworkConfigInfo conversions
  static telux::wlan::StaNetworkConfigInfo convertStaNetworkConfigInfoFromGrpc(
      const ::wlanStub::StaNetworkConfigInfo &info);
  static ::wlanStub::StaNetworkConfigInfo convertStaNetworkConfigInfoToGrpc(
      const telux::wlan::StaNetworkConfigInfo &info);

  // telux::wlan::ExternalApInfo conversions
  static telux::wlan::ExternalApInfo
  convertExternalApInfoFromGrpc(const ::wlanStub::ExternalApInfo &info);
  static ::wlanStub::ExternalApInfo
  convertExternalApInfoToGrpc(const telux::wlan::ExternalApInfo &info);

  // telux::wlan::StaScanResult conversions
  static telux::wlan::StaScanResult
  convertStaScanResultFromGrpc(const ::wlanStub::StaScanResult &result);
  static ::wlanStub::StaScanResult
  convertStaScanResultToGrpc(const telux::wlan::StaScanResult &result);

}; // class WlanCommonUtilsStub

} // namespace wlan
} // namespace telux

#endif // TELUX_WLAN_SIM_WLANCOMMONUTILSSTUB_HPP
