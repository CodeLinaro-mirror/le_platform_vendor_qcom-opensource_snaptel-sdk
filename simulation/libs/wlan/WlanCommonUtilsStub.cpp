/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanCommonUtilsStub.hpp"
#include "common/Logger.hpp"
#include <common/AsyncTaskQueue.hpp>
#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace wlan {

// --- telux::wlan::BandType conversions ---
telux::wlan::BandType
WlanCommonUtilsStub::convertBandTypeFromGrpc(::wlanStub::BandType type) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc BandType: ", static_cast<int>(type));
  switch (type) {
  case ::wlanStub::BAND_5GHZ:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::BandType::BAND_5GHZ");
    return telux::wlan::BandType::BAND_5GHZ;
  case ::wlanStub::BAND_2GHZ:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::BandType::BAND_2GHZ");
    return telux::wlan::BandType::BAND_2GHZ;
  case ::wlanStub::BAND_6GHZ:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::BandType::BAND_6GHZ");
    return telux::wlan::BandType::BAND_6GHZ;
  default:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::BandType::BAND_5GHZ (Default)");
    return telux::wlan::BandType::BAND_5GHZ; // Default to UNKNOWN
  }
}

::wlanStub::BandType
WlanCommonUtilsStub::convertBandTypeToGrpc(telux::wlan::BandType type) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc BandType from: ", static_cast<int>(type));
  switch (type) {
  case telux::wlan::BandType::BAND_5GHZ:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::BAND_5GHZ");
    return ::wlanStub::BAND_5GHZ;
  case telux::wlan::BandType::BAND_2GHZ:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::BAND_2GHZ");
    return ::wlanStub::BAND_2GHZ;
  case telux::wlan::BandType::BAND_6GHZ:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::BAND_6GHZ");
    return ::wlanStub::BAND_6GHZ;
  default:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::BAND_TYPE_UNKNOWN (Default)");
    return ::wlanStub::BAND_TYPE_UNKNOWN;
  }
}

// --- telux::wlan::Id conversions ---
telux::wlan::Id WlanCommonUtilsStub::convertIdFromGrpc(::wlanStub::Id id) {
  LOG(DEBUG, __FUNCTION__, " Converting from Grpc Id: ", static_cast<int>(id));
  switch (id) {
  case ::wlanStub::PRIMARY:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::Id::PRIMARY");
    return telux::wlan::Id::PRIMARY;
  case ::wlanStub::SECONDARY:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::Id::SECONDARY");
    return telux::wlan::Id::SECONDARY;
  case ::wlanStub::TERTIARY:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::Id::TERTIARY");
    return telux::wlan::Id::TERTIARY;
  case ::wlanStub::QUATERNARY:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::Id::QUATERNARY");
    return telux::wlan::Id::QUATERNARY;
  default:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::Id::PRIMARY (Default)");
    return telux::wlan::Id::PRIMARY; // Default
  }
}

::wlanStub::Id WlanCommonUtilsStub::convertIdToGrpc(telux::wlan::Id id) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc Id from: ", static_cast<int>(id));
  switch (id) {
  case telux::wlan::Id::PRIMARY:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::PRIMARY");
    return ::wlanStub::PRIMARY;
  case telux::wlan::Id::SECONDARY:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::SECONDARY");
    return ::wlanStub::SECONDARY;
  case telux::wlan::Id::TERTIARY:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::TERTIARY");
    return ::wlanStub::TERTIARY;
  case telux::wlan::Id::QUATERNARY:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::QUATERNARY");
    return ::wlanStub::QUATERNARY;
  default:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::ID_UNKNOWN (Default)");
    return ::wlanStub::ID_UNKNOWN;
  }
}

// --- telux::wlan::ServiceOperation conversions ---
telux::wlan::ServiceOperation
WlanCommonUtilsStub::convertServiceOperationFromGrpc(
    ::wlanStub::ServiceOperation op) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc ServiceOperation: ", static_cast<int>(op));
  switch (op) {
  case ::wlanStub::STOP:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::ServiceOperation::STOP");
    return telux::wlan::ServiceOperation::STOP;
  case ::wlanStub::START:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::ServiceOperation::START");
    return telux::wlan::ServiceOperation::START;
  case ::wlanStub::RESTART:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::ServiceOperation::RESTART");
    return telux::wlan::ServiceOperation::RESTART;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::ServiceOperation::STOP (Default)");
    return telux::wlan::ServiceOperation::STOP; // Default
  }
}

::wlanStub::ServiceOperation WlanCommonUtilsStub::convertServiceOperationToGrpc(
    telux::wlan::ServiceOperation op) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc ServiceOperation from: ", static_cast<int>(op));
  switch (op) {
  case telux::wlan::ServiceOperation::STOP:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::STOP");
    return ::wlanStub::STOP;
  case telux::wlan::ServiceOperation::START:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::START");
    return ::wlanStub::START;
  case telux::wlan::ServiceOperation::RESTART:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::RESTART");
    return ::wlanStub::RESTART;
  default:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::SERV_OP_UNKNOWN (Default)");
    return ::wlanStub::SERV_OP_UNKNOWN;
  }
}

// --- telux::wlan::ApInfo conversions ---
telux::wlan::ApInfo
WlanCommonUtilsStub::convertApInfoFromGrpc(const ::wlanStub::ApInfo &apInfo) {
  LOG(DEBUG, __FUNCTION__, " Converting from Grpc ApInfo");
  telux::wlan::ApInfo apInfoObj;
  apInfoObj.apRadio = convertBandTypeFromGrpc(apInfo.ap_radio());
  if (apInfo.ap_type() == ::wlanStub::PRIVATE) {
    apInfoObj.apType = telux::wlan::ApType::PRIVATE;
    LOG(DEBUG, __FUNCTION__, " ApType: PRIVATE");
  } else if (apInfo.ap_type() == ::wlanStub::GUEST) {
    apInfoObj.apType = telux::wlan::ApType::GUEST;
    LOG(DEBUG, __FUNCTION__, " ApType: GUEST");
  } else {
    apInfoObj.apType = telux::wlan::ApType::UNKNOWN;
    LOG(DEBUG, __FUNCTION__, " ApType: UNKNOWN (Default)");
  }
  return apInfoObj;
}

::wlanStub::ApInfo
WlanCommonUtilsStub::convertApInfoToGrpc(const telux::wlan::ApInfo &apInfo) {
  LOG(DEBUG, __FUNCTION__, " Converting to Grpc ApInfo");
  ::wlanStub::ApInfo grpcApInfo;
  grpcApInfo.set_ap_radio(convertBandTypeToGrpc(apInfo.apRadio));
  if (apInfo.apType == telux::wlan::ApType::PRIVATE) {
    grpcApInfo.set_ap_type(::wlanStub::PRIVATE);
    LOG(DEBUG, __FUNCTION__, " ApType: PRIVATE");
  } else if (apInfo.apType == telux::wlan::ApType::GUEST) {
    grpcApInfo.set_ap_type(::wlanStub::GUEST);
    LOG(DEBUG, __FUNCTION__, " ApType: GUEST");
  } else {
    grpcApInfo.set_ap_type(::wlanStub::AP_TYPE_UNKNOWN);
    LOG(DEBUG, __FUNCTION__, " ApType: AP_TYPE_UNKNOWN (Default)");
  }
  return grpcApInfo;
}

// --- telux::wlan::ApNetInfo conversions ---
telux::wlan::ApNetInfo WlanCommonUtilsStub::convertApNetInfoFromGrpc(
    const ::wlanStub::ApNetInfo &apNetInfo) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc ApNetInfo, SSID: ", apNetInfo.ssid());
  telux::wlan::ApNetInfo apNetInfoObj;
  apNetInfoObj.info = convertApInfoFromGrpc(apNetInfo.info());
  apNetInfoObj.ssid = apNetInfo.ssid();
  return apNetInfoObj;
}

::wlanStub::ApNetInfo WlanCommonUtilsStub::convertApNetInfoToGrpc(
    const telux::wlan::ApNetInfo &apNetInfo) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc ApNetInfo, SSID: ", apNetInfo.ssid);
  ::wlanStub::ApNetInfo grpcApNetInfo;
  *grpcApNetInfo.mutable_info() = convertApInfoToGrpc(apNetInfo.info);
  grpcApNetInfo.set_ssid(apNetInfo.ssid);
  return grpcApNetInfo;
}

// --- telux::wlan::ApStatus conversions ---
telux::wlan::ApStatus WlanCommonUtilsStub::convertApStatusFromGrpc(
    const ::wlanStub::ApStatus &apStatus) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc ApStatus, ID: ", static_cast<int>(apStatus.id()),
      ", Name: ", apStatus.name());
  telux::wlan::ApStatus apStatusObj;
  apStatusObj.id = convertIdFromGrpc(apStatus.id());
  apStatusObj.name = apStatus.name();
  apStatusObj.ipv4Address = apStatus.ipv4_address();
  apStatusObj.macAddress = apStatus.mac_address();
  for (const auto &netInfo : apStatus.network()) {
    apStatusObj.network.push_back(convertApNetInfoFromGrpc(netInfo));
  }
  return apStatusObj;
}

::wlanStub::ApStatus WlanCommonUtilsStub::convertApStatusToGrpc(
    const telux::wlan::ApStatus &apStatus) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc ApStatus, ID: ", static_cast<int>(apStatus.id),
      ", Name: ", apStatus.name);
  ::wlanStub::ApStatus grpcApStatus;
  grpcApStatus.set_id(convertIdToGrpc(apStatus.id));
  grpcApStatus.set_name(apStatus.name);
  grpcApStatus.set_ipv4_address(apStatus.ipv4Address);
  grpcApStatus.set_mac_address(apStatus.macAddress);
  for (const auto &netInfo : apStatus.network) {
    *grpcApStatus.add_network() = convertApNetInfoToGrpc(netInfo);
  }
  return grpcApStatus;
}

// --- telux::wlan::StaStatus conversions ---
telux::wlan::StaStatus WlanCommonUtilsStub::convertStaStatusFromGrpc(
    const ::wlanStub::StaStatus &staStatus) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc StaStatus, ID: ", static_cast<int>(staStatus.id()),
      ", Name: ", staStatus.name());
  telux::wlan::StaStatus staStatusObj;
  staStatusObj.id = convertIdFromGrpc(staStatus.id());
  staStatusObj.name = staStatus.name();
  staStatusObj.ipv4Address = staStatus.ipv4_address();
  staStatusObj.ipv6Address = staStatus.ipv6_address();
  staStatusObj.macAddress = staStatus.mac_address();
  staStatusObj.status =
      static_cast<telux::wlan::StaInterfaceStatus>(staStatus.status());
  staStatusObj.connectionStatus = static_cast<telux::wlan::StaConnectionStatus>(
      staStatus.connection_status());
  LOG(DEBUG, __FUNCTION__, " Status: ", static_cast<int>(staStatusObj.status),
      ", ConnectionStatus: ", static_cast<int>(staStatusObj.connectionStatus));
  return staStatusObj;
}

::wlanStub::StaStatus WlanCommonUtilsStub::convertStaStatusToGrpc(
    const telux::wlan::StaStatus &staStatus) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc StaStatus, ID: ", static_cast<int>(staStatus.id),
      ", Name: ", staStatus.name);
  ::wlanStub::StaStatus grpcStaStatus;
  grpcStaStatus.set_id(convertIdToGrpc(staStatus.id));
  grpcStaStatus.set_name(staStatus.name);
  grpcStaStatus.set_ipv4_address(staStatus.ipv4Address);
  grpcStaStatus.set_ipv6_address(staStatus.ipv6Address);
  grpcStaStatus.set_mac_address(staStatus.macAddress);
  grpcStaStatus.set_status(
      static_cast<::wlanStub::StaInterfaceStatus>(staStatus.status));
  grpcStaStatus.set_connection_status(
      static_cast<::wlanStub::StaConnectionStatus>(staStatus.connectionStatus));
  LOG(DEBUG, __FUNCTION__, " Status: ", static_cast<int>(staStatus.status),
      ", ConnectionStatus: ", static_cast<int>(staStatus.connectionStatus));
  return grpcStaStatus;
}

// --- telux::wlan::DevicePerfState conversions ---
telux::wlan::DevicePerfState
WlanCommonUtilsStub::convertDevicePerfStateFromGrpc(
    ::wlanStub::DevicePerfState state) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc DevicePerfState: ", static_cast<int>(state));
  switch (state) {
  case ::wlanStub::FULL:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::DevicePerfState::FULL");
    return telux::wlan::DevicePerfState::FULL;
  case ::wlanStub::REDUCED:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::DevicePerfState::REDUCED");
    return telux::wlan::DevicePerfState::REDUCED;
  case ::wlanStub::SHUTDOWN:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::DevicePerfState::SHUTDOWN");
    return telux::wlan::DevicePerfState::SHUTDOWN;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::DevicePerfState::UNKNOWN (Default)");
    return telux::wlan::DevicePerfState::UNKNOWN;
  }
}

::wlanStub::DevicePerfState WlanCommonUtilsStub::convertDevicePerfStateToGrpc(
    telux::wlan::DevicePerfState state) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc DevicePerfState from: ", static_cast<int>(state));
  switch (state) {
  case telux::wlan::DevicePerfState::FULL:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::FULL");
    return ::wlanStub::FULL;
  case telux::wlan::DevicePerfState::REDUCED:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::REDUCED");
    return ::wlanStub::REDUCED;
  case telux::wlan::DevicePerfState::SHUTDOWN:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::SHUTDOWN");
    return ::wlanStub::SHUTDOWN;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> ::wlanStub::DEV_PERF_STATE_UNKNOWN (Default)");
    return ::wlanStub::DEV_PERF_STATE_UNKNOWN;
  }
}

// --- telux::wlan::ApInterworking conversions ---
telux::wlan::ApInterworking WlanCommonUtilsStub::convertApInterworkingFromGrpc(
    ::wlanStub::ApInterworking interworking) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc ApInterworking: ", static_cast<int>(interworking));
  switch (interworking) {
  case ::wlanStub::INTERNET_ACCESS:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::ApInterworking::INTERNET_ACCESS");
    return telux::wlan::ApInterworking::INTERNET_ACCESS;
  case ::wlanStub::FULL_ACCESS:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::ApInterworking::FULL_ACCESS");
    return telux::wlan::ApInterworking::FULL_ACCESS;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::ApInterworking::INTERNET_ACCESS (Default)");
    return telux::wlan::ApInterworking::INTERNET_ACCESS; // Default
  }
}

::wlanStub::ApInterworking WlanCommonUtilsStub::convertApInterworkingToGrpc(
    telux::wlan::ApInterworking interworking) {
  LOG(DEBUG, __FUNCTION__, " Converting to Grpc ApInterworking from: ",
      static_cast<int>(interworking));
  switch (interworking) {
  case telux::wlan::ApInterworking::INTERNET_ACCESS:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::INTERNET_ACCESS");
    return ::wlanStub::INTERNET_ACCESS;
  case telux::wlan::ApInterworking::FULL_ACCESS:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::FULL_ACCESS");
    return ::wlanStub::FULL_ACCESS;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> ::wlanStub::AP_INTERWORKING_UNKNOWN (Default)");
    return ::wlanStub::AP_INTERWORKING_UNKNOWN;
  }
}

// --- telux::wlan::SecMode conversions ---
telux::wlan::SecMode
WlanCommonUtilsStub::convertSecModeFromGrpc(::wlanStub::SecMode mode) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc SecMode: ", static_cast<int>(mode));
  switch (mode) {
  case ::wlanStub::OPEN:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecMode::OPEN");
    return telux::wlan::SecMode::OPEN;
  case ::wlanStub::WEP:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecMode::WEP");
    return telux::wlan::SecMode::WEP;
  case ::wlanStub::WPA:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecMode::WPA");
    return telux::wlan::SecMode::WPA;
  case ::wlanStub::WPA2:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecMode::WPA2");
    return telux::wlan::SecMode::WPA2;
  case ::wlanStub::WPA3:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecMode::WPA3");
    return telux::wlan::SecMode::WPA3;
  default:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecMode::OPEN (Default)");
    return telux::wlan::SecMode::OPEN; // Default
  }
}

::wlanStub::SecMode
WlanCommonUtilsStub::convertSecModeToGrpc(telux::wlan::SecMode mode) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc SecMode from: ", static_cast<int>(mode));
  switch (mode) {
  case telux::wlan::SecMode::OPEN:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::OPEN");
    return ::wlanStub::OPEN;
  case telux::wlan::SecMode::WEP:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::WEP");
    return ::wlanStub::WEP;
  case telux::wlan::SecMode::WPA:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::WPA");
    return ::wlanStub::WPA;
  case telux::wlan::SecMode::WPA2:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::WPA2");
    return ::wlanStub::WPA2;
  case telux::wlan::SecMode::WPA3:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::WPA3");
    return ::wlanStub::WPA3;
  default:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::SEC_MODE_UNKNOWN (Default)");
    return ::wlanStub::SEC_MODE_UNKNOWN;
  }
}

// --- telux::wlan::SecAuth conversions ---
telux::wlan::SecAuth
WlanCommonUtilsStub::convertSecAuthFromGrpc(::wlanStub::SecAuth auth) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc SecAuth: ", static_cast<int>(auth));
  switch (auth) {
  case ::wlanStub::NONE:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::NONE");
    return telux::wlan::SecAuth::NONE;
  case ::wlanStub::PSK:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::PSK");
    return telux::wlan::SecAuth::PSK;
  case ::wlanStub::EAP_SIM:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::EAP_SIM");
    return telux::wlan::SecAuth::EAP_SIM;
  case ::wlanStub::EAP_AKA:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::EAP_AKA");
    return telux::wlan::SecAuth::EAP_AKA;
  case ::wlanStub::EAP_LEAP:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::EAP_LEAP");
    return telux::wlan::SecAuth::EAP_LEAP;
  case ::wlanStub::EAP_TLS:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::EAP_TLS");
    return telux::wlan::SecAuth::EAP_TLS;
  case ::wlanStub::EAP_TTLS:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::EAP_TTLS");
    return telux::wlan::SecAuth::EAP_TTLS;
  case ::wlanStub::EAP_PEAP:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::EAP_PEAP");
    return telux::wlan::SecAuth::EAP_PEAP;
  case ::wlanStub::EAP_FAST:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::EAP_FAST");
    return telux::wlan::SecAuth::EAP_FAST;
  case ::wlanStub::EAP_PSK:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::EAP_PSK");
    return telux::wlan::SecAuth::EAP_PSK;
  case ::wlanStub::SAE:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::SAE");
    return telux::wlan::SecAuth::SAE;
  default:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecAuth::NONE (Default)");
    return telux::wlan::SecAuth::NONE; // Default
  }
}

::wlanStub::SecAuth
WlanCommonUtilsStub::convertSecAuthToGrpc(telux::wlan::SecAuth auth) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc SecAuth from: ", static_cast<int>(auth));
  switch (auth) {
  case telux::wlan::SecAuth::NONE:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::NONE");
    return ::wlanStub::NONE;
  case telux::wlan::SecAuth::PSK:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::PSK");
    return ::wlanStub::PSK;
  case telux::wlan::SecAuth::EAP_SIM:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EAP_SIM");
    return ::wlanStub::EAP_SIM;
  case telux::wlan::SecAuth::EAP_AKA:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EAP_AKA");
    return ::wlanStub::EAP_AKA;
  case telux::wlan::SecAuth::EAP_LEAP:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EAP_LEAP");
    return ::wlanStub::EAP_LEAP;
  case telux::wlan::SecAuth::EAP_TLS:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EAP_TLS");
    return ::wlanStub::EAP_TLS;
  case telux::wlan::SecAuth::EAP_TTLS:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EAP_TTLS");
    return ::wlanStub::EAP_TTLS;
  case telux::wlan::SecAuth::EAP_PEAP:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EAP_PEAP");
    return ::wlanStub::EAP_PEAP;
  case telux::wlan::SecAuth::EAP_FAST:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EAP_FAST");
    return ::wlanStub::EAP_FAST;
  case telux::wlan::SecAuth::EAP_PSK:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EAP_PSK");
    return ::wlanStub::EAP_PSK;
  case telux::wlan::SecAuth::SAE:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::SAE");
    return ::wlanStub::SAE;
  default:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::NONE (Default)");
    return ::wlanStub::NONE;
  }
}

// --- telux::wlan::SecEncrypt conversions ---
telux::wlan::SecEncrypt
WlanCommonUtilsStub::convertSecEncryptFromGrpc(::wlanStub::SecEncrypt encrypt) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc SecEncrypt: ", static_cast<int>(encrypt));
  switch (encrypt) {
  case ::wlanStub::RC4:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecEncrypt::RC4");
    return telux::wlan::SecEncrypt::RC4;
  case ::wlanStub::TKIP:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecEncrypt::TKIP");
    return telux::wlan::SecEncrypt::TKIP;
  case ::wlanStub::AES:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecEncrypt::AES");
    return telux::wlan::SecEncrypt::AES;
  case ::wlanStub::GCMP:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecEncrypt::GCMP");
    return telux::wlan::SecEncrypt::GCMP;
  default:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::SecEncrypt::RC4 (Default)");
    return telux::wlan::SecEncrypt::RC4; // Default
  }
}

::wlanStub::SecEncrypt
WlanCommonUtilsStub::convertSecEncryptToGrpc(telux::wlan::SecEncrypt encrypt) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc SecEncrypt from: ", static_cast<int>(encrypt));
  switch (encrypt) {
  case telux::wlan::SecEncrypt::RC4:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::RC4");
    return ::wlanStub::RC4;
  case telux::wlan::SecEncrypt::TKIP:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::TKIP");
    return ::wlanStub::TKIP;
  case telux::wlan::SecEncrypt::AES:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::AES");
    return ::wlanStub::AES;
  case telux::wlan::SecEncrypt::GCMP:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::GCMP");
    return ::wlanStub::GCMP;
  default:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::SEC_ENCRYPT_UNKNOWN (Default)");
    return ::wlanStub::SEC_ENCRYPT_UNKNOWN;
  }
}

// --- telux::wlan::NetAccessType conversions ---
telux::wlan::NetAccessType WlanCommonUtilsStub::convertNetAccessTypeFromGrpc(
    ::wlanStub::NetAccessType type) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc NetAccessType: ", static_cast<int>(type));
  switch (type) {
  case ::wlanStub::PRIVATE_ACCESS:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::NetAccessType::PRIVATE");
    return telux::wlan::NetAccessType::PRIVATE;
  case ::wlanStub::PRIVATE_WITH_GUEST:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::NetAccessType::PRIVATE_WITH_GUEST");
    return telux::wlan::NetAccessType::PRIVATE_WITH_GUEST;
  case ::wlanStub::CHARGEABLE_PUBLIC:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::NetAccessType::CHARGEABLE_PUBLIC");
    return telux::wlan::NetAccessType::CHARGEABLE_PUBLIC;
  case ::wlanStub::FREE_PUBLIC:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::NetAccessType::FREE_PUBLIC");
    return telux::wlan::NetAccessType::FREE_PUBLIC;
  case ::wlanStub::PERSONAL_DEVICE:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::NetAccessType::PERSONAL_DEVICE");
    return telux::wlan::NetAccessType::PERSONAL_DEVICE;
  case ::wlanStub::EMERGENCY_SERVICES_ONLY:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::NetAccessType::EMERGENCY_SERVICES_ONLY");
    return telux::wlan::NetAccessType::EMERGENCY_SERVICES_ONLY;
  case ::wlanStub::TEST_OR_EXPERIMENTAL:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::NetAccessType::TEST_OR_EXPERIMENTAL");
    return telux::wlan::NetAccessType::TEST_OR_EXPERIMENTAL;
  case ::wlanStub::WILDCARD:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::NetAccessType::WILDCARD");
    return telux::wlan::NetAccessType::WILDCARD;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::NetAccessType::PRIVATE (Default)");
    return telux::wlan::NetAccessType::PRIVATE; // Default
  }
}

::wlanStub::NetAccessType WlanCommonUtilsStub::convertNetAccessTypeToGrpc(
    telux::wlan::NetAccessType type) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc NetAccessType from: ", static_cast<int>(type));
  switch (type) {
  case telux::wlan::NetAccessType::PRIVATE:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::PRIVATE_ACCESS");
    return ::wlanStub::PRIVATE_ACCESS;
  case telux::wlan::NetAccessType::PRIVATE_WITH_GUEST:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::PRIVATE_WITH_GUEST");
    return ::wlanStub::PRIVATE_WITH_GUEST;
  case telux::wlan::NetAccessType::CHARGEABLE_PUBLIC:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::CHARGEABLE_PUBLIC");
    return ::wlanStub::CHARGEABLE_PUBLIC;
  case telux::wlan::NetAccessType::FREE_PUBLIC:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::FREE_PUBLIC");
    return ::wlanStub::FREE_PUBLIC;
  case telux::wlan::NetAccessType::PERSONAL_DEVICE:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::PERSONAL_DEVICE");
    return ::wlanStub::PERSONAL_DEVICE;
  case telux::wlan::NetAccessType::EMERGENCY_SERVICES_ONLY:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::EMERGENCY_SERVICES_ONLY");
    return ::wlanStub::EMERGENCY_SERVICES_ONLY;
  case telux::wlan::NetAccessType::TEST_OR_EXPERIMENTAL:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::TEST_OR_EXPERIMENTAL");
    return ::wlanStub::TEST_OR_EXPERIMENTAL;
  case telux::wlan::NetAccessType::WILDCARD:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::WILDCARD");
    return ::wlanStub::WILDCARD;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> ::wlanStub::NET_ACCESS_TYPE_UNKNOWN (Default)");
    return ::wlanStub::NET_ACCESS_TYPE_UNKNOWN;
  }
}

// --- telux::wlan::ApSecurity conversions ---
telux::wlan::ApSecurity WlanCommonUtilsStub::convertApSecurityFromGrpc(
    const ::wlanStub::ApSecurity &apSecurity) {
  LOG(DEBUG, __FUNCTION__, " Converting from Grpc ApSecurity");
  telux::wlan::ApSecurity apSecurityObj;
  apSecurityObj.mode = convertSecModeFromGrpc(apSecurity.mode());
  apSecurityObj.auth = convertSecAuthFromGrpc(apSecurity.auth());
  apSecurityObj.encrypt = convertSecEncryptFromGrpc(apSecurity.encrypt());
  return apSecurityObj;
}

::wlanStub::ApSecurity WlanCommonUtilsStub::convertApSecurityToGrpc(
    const telux::wlan::ApSecurity &apSecurity) {
  LOG(DEBUG, __FUNCTION__, " Converting to Grpc ApSecurity");
  ::wlanStub::ApSecurity grpcApSecurity;
  grpcApSecurity.set_mode(convertSecModeToGrpc(apSecurity.mode));
  grpcApSecurity.set_auth(convertSecAuthToGrpc(apSecurity.auth));
  grpcApSecurity.set_encrypt(convertSecEncryptToGrpc(apSecurity.encrypt));
  return grpcApSecurity;
}

// --- telux::wlan::ApElementInfoConfig conversions ---
telux::wlan::ApElementInfoConfig
WlanCommonUtilsStub::convertApElementInfoConfigFromGrpc(
    const ::wlanStub::ApElementInfoConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc ApElementInfoConfig, IsEnabled: ",
      config.is_enabled(),
      ", IsInterworkingEnabled: ", config.is_interworking_enabled());
  telux::wlan::ApElementInfoConfig configObj;
  configObj.isEnabled = config.is_enabled();
  configObj.isInterworkingEnabled = config.is_interworking_enabled();
  configObj.netAccessType =
      convertNetAccessTypeFromGrpc(config.net_access_type());
  configObj.internet = config.internet();
  configObj.asra = config.asra();
  configObj.esr = config.esr();
  configObj.uesa = config.uesa();
  configObj.venueGroup = static_cast<uint8_t>(config.venue_group());
  configObj.venueType = static_cast<uint8_t>(config.venue_type());
  configObj.hessid = config.hessid();
  configObj.vendorElements = config.vendor_elements();
  configObj.assocRespElements = config.assoc_resp_elements();
  return configObj;
}

::wlanStub::ApElementInfoConfig
WlanCommonUtilsStub::convertApElementInfoConfigToGrpc(
    const telux::wlan::ApElementInfoConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc ApElementInfoConfig, IsEnabled: ", config.isEnabled,
      ", IsInterworkingEnabled: ", config.isInterworkingEnabled);
  ::wlanStub::ApElementInfoConfig grpcConfig;
  grpcConfig.set_is_enabled(config.isEnabled);
  grpcConfig.set_is_interworking_enabled(config.isInterworkingEnabled);
  grpcConfig.set_net_access_type(
      convertNetAccessTypeToGrpc(config.netAccessType));
  grpcConfig.set_internet(config.internet);
  grpcConfig.set_asra(config.asra);
  grpcConfig.set_esr(config.esr);
  grpcConfig.set_uesa(config.uesa);
  grpcConfig.set_venue_group(config.venueGroup);
  grpcConfig.set_venue_type(config.venueType);
  grpcConfig.set_hessid(config.hessid);
  grpcConfig.set_vendor_elements(config.vendorElements);
  grpcConfig.set_assoc_resp_elements(config.assocRespElements);
  return grpcConfig;
}

// --- telux::wlan::ApNetConfig conversions ---
telux::wlan::ApNetConfig WlanCommonUtilsStub::convertApNetConfigFromGrpc(
    const ::wlanStub::ApNetConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc ApNetConfig, SSID: ", config.ssid());
  telux::wlan::ApNetConfig configObj;
  configObj.info = convertApInfoFromGrpc(config.info());
  configObj.ssid = config.ssid();
  configObj.isVisible = config.is_visible();
  configObj.elementInfoConfig =
      convertApElementInfoConfigFromGrpc(config.element_info_config());
  configObj.interworking = convertApInterworkingFromGrpc(config.interworking());
  configObj.apSecurity = convertApSecurityFromGrpc(config.ap_security());
  configObj.passPhrase = config.pass_phrase();
  return configObj;
}

::wlanStub::ApNetConfig WlanCommonUtilsStub::convertApNetConfigToGrpc(
    const telux::wlan::ApNetConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc ApNetConfig, SSID: ", config.ssid);
  ::wlanStub::ApNetConfig grpcConfig;
  *grpcConfig.mutable_info() = convertApInfoToGrpc(config.info);
  grpcConfig.set_ssid(config.ssid);
  grpcConfig.set_is_visible(config.isVisible);
  *grpcConfig.mutable_element_info_config() =
      convertApElementInfoConfigToGrpc(config.elementInfoConfig);
  grpcConfig.set_interworking(convertApInterworkingToGrpc(config.interworking));
  *grpcConfig.mutable_ap_security() =
      convertApSecurityToGrpc(config.apSecurity);
  grpcConfig.set_pass_phrase(config.passPhrase);
  return grpcConfig;
}

// --- telux::wlan::ApConfig conversions ---
telux::wlan::ApConfig WlanCommonUtilsStub::convertApConfigFromGrpc(
    const ::wlanStub::ApConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc ApConfig, ID: ", static_cast<int>(config.id()));
  telux::wlan::ApConfig configObj;
  configObj.id = convertIdFromGrpc(config.id());
  configObj.venue.type = config.venue().type();
  configObj.venue.group = config.venue().group();
  for (const auto &netConfig : config.network()) {
    configObj.network.push_back(convertApNetConfigFromGrpc(netConfig));
  }
  return configObj;
}

::wlanStub::ApConfig WlanCommonUtilsStub::convertApConfigToGrpc(
    const telux::wlan::ApConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc ApConfig, ID: ", static_cast<int>(config.id));
  ::wlanStub::ApConfig grpcConfig;
  grpcConfig.set_id(convertIdToGrpc(config.id));
  grpcConfig.mutable_venue()->set_type(config.venue.type);
  grpcConfig.mutable_venue()->set_group(config.venue.group);
  for (const auto &netConfig : config.network) {
    *grpcConfig.add_network() = convertApNetConfigToGrpc(netConfig);
  }
  return grpcConfig;
}

// --- telux::wlan::DeviceInfo conversions ---
telux::wlan::DeviceInfo WlanCommonUtilsStub::convertDeviceInfoFromGrpc(
    const ::wlanStub::DeviceInfo &info) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc DeviceInfo, ID: ", static_cast<int>(info.id()),
      ", Name: ", info.name(), ", IPv4: ", info.ipv4_address());
  telux::wlan::DeviceInfo infoObj;
  infoObj.id = convertIdFromGrpc(info.id());
  infoObj.name = info.name();
  infoObj.ipv4Address = info.ipv4_address();
  for (const auto &ipv6 : info.ipv6_address()) {
    infoObj.ipv6Address.push_back(ipv6);
  }
  infoObj.macAddress = info.mac_address();
  return infoObj;
}

::wlanStub::DeviceInfo WlanCommonUtilsStub::convertDeviceInfoToGrpc(
    const telux::wlan::DeviceInfo &info) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc DeviceInfo, ID: ", static_cast<int>(info.id),
      ", Name: ", info.name, ", IPv4: ", info.ipv4Address);
  ::wlanStub::DeviceInfo grpcInfo;
  grpcInfo.set_id(convertIdToGrpc(info.id));
  grpcInfo.set_name(info.name);
  grpcInfo.set_ipv4_address(info.ipv4Address);
  for (const auto &ipv6 : info.ipv6Address) {
    grpcInfo.add_ipv6_address(ipv6);
  }
  grpcInfo.set_mac_address(info.macAddress);
  return grpcInfo;
}

// --- telux::wlan::DeviceIndInfo conversions ---
telux::wlan::DeviceIndInfo WlanCommonUtilsStub::convertDeviceIndInfoFromGrpc(
    const ::wlanStub::DeviceIndInfo &info) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc DeviceIndInfo, ID: ", static_cast<int>(info.id()),
      ", MAC: ", info.mac_address());
  telux::wlan::DeviceIndInfo infoObj;
  infoObj.id = convertIdFromGrpc(info.id());
  infoObj.macAddress = info.mac_address();
  return infoObj;
}

::wlanStub::DeviceIndInfo WlanCommonUtilsStub::convertDeviceIndInfoToGrpc(
    const telux::wlan::DeviceIndInfo &info) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc DeviceIndInfo, ID: ", static_cast<int>(info.id),
      ", MAC: ", info.macAddress);
  ::wlanStub::DeviceIndInfo grpcInfo;
  grpcInfo.set_id(convertIdToGrpc(info.id));
  grpcInfo.set_mac_address(info.macAddress);
  return grpcInfo;
}

// --- telux::wlan::StaIpConfig conversions ---
telux::wlan::StaIpConfig WlanCommonUtilsStub::convertStaIpConfigFromGrpc(
    ::wlanStub::StaIpConfig config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc StaIpConfig: ", static_cast<int>(config));
  switch (config) {
  case ::wlanStub::DYNAMIC_IP:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::StaIpConfig::DYNAMIC_IP");
    return telux::wlan::StaIpConfig::DYNAMIC_IP;
  case ::wlanStub::STATIC_IP:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::StaIpConfig::STATIC_IP");
    return telux::wlan::StaIpConfig::STATIC_IP;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::StaIpConfig::DYNAMIC_IP (Default)");
    return telux::wlan::StaIpConfig::DYNAMIC_IP; // Default
  }
}

::wlanStub::StaIpConfig
WlanCommonUtilsStub::convertStaIpConfigToGrpc(telux::wlan::StaIpConfig config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc StaIpConfig from: ", static_cast<int>(config));
  switch (config) {
  case telux::wlan::StaIpConfig::DYNAMIC_IP:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::DYNAMIC_IP");
    return ::wlanStub::DYNAMIC_IP;
  case telux::wlan::StaIpConfig::STATIC_IP:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::STATIC_IP");
    return ::wlanStub::STATIC_IP;
  default:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::STA_IP_CONFIG_UNKNOWN (Default)");
    return ::wlanStub::STA_IP_CONFIG_UNKNOWN;
  }
}

// --- telux::wlan::StaBridgeMode conversions ---
telux::wlan::StaBridgeMode WlanCommonUtilsStub::convertStaBridgeModeFromGrpc(
    ::wlanStub::StaBridgeMode mode) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc StaBridgeMode: ", static_cast<int>(mode));
  switch (mode) {
  case ::wlanStub::ROUTER:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::StaBridgeMode::ROUTER");
    return telux::wlan::StaBridgeMode::ROUTER;
  case ::wlanStub::BRIDGE:
    LOG(DEBUG, __FUNCTION__, " -> telux::wlan::StaBridgeMode::BRIDGE");
    return telux::wlan::StaBridgeMode::BRIDGE;
  default:
    LOG(DEBUG, __FUNCTION__,
        " -> telux::wlan::StaBridgeMode::ROUTER (Default)");
    return telux::wlan::StaBridgeMode::ROUTER; // Default
  }
}

::wlanStub::StaBridgeMode WlanCommonUtilsStub::convertStaBridgeModeToGrpc(
    telux::wlan::StaBridgeMode mode) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc StaBridgeMode from: ", static_cast<int>(mode));
  switch (mode) {
  case telux::wlan::StaBridgeMode::ROUTER:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::ROUTER");
    return ::wlanStub::ROUTER;
  case telux::wlan::StaBridgeMode::BRIDGE:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::BRIDGE");
    return ::wlanStub::BRIDGE;
  default:
    LOG(DEBUG, __FUNCTION__, " -> ::wlanStub::ROUTER (Default)");
    return ::wlanStub::ROUTER;
  }
}

// --- telux::wlan::StaStaticIpConfig conversions ---
telux::wlan::StaStaticIpConfig
WlanCommonUtilsStub::convertStaStaticIpConfigFromGrpc(
    const ::wlanStub::StaStaticIpConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc StaStaticIpConfig, IP: ", config.ip_addr());
  telux::wlan::StaStaticIpConfig configObj;
  configObj.ipAddr = config.ip_addr();
  configObj.gwIpAddr = config.gw_ip_addr();
  configObj.netMask = config.net_mask();
  configObj.dnsAddr = config.dns_addr();
  return configObj;
}

::wlanStub::StaStaticIpConfig
WlanCommonUtilsStub::convertStaStaticIpConfigToGrpc(
    const telux::wlan::StaStaticIpConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc StaStaticIpConfig, IP: ", config.ipAddr);
  ::wlanStub::StaStaticIpConfig grpcConfig;
  grpcConfig.set_ip_addr(config.ipAddr);
  grpcConfig.set_gw_ip_addr(config.gwIpAddr);
  grpcConfig.set_net_mask(config.netMask);
  grpcConfig.set_dns_addr(config.dnsAddr);
  return grpcConfig;
}

// --- telux::wlan::StaConfig conversions ---
telux::wlan::StaConfig WlanCommonUtilsStub::convertStaConfigFromGrpc(
    const ::wlanStub::StaConfig &config) {
  LOG(DEBUG, __FUNCTION__, " Converting from Grpc StaConfig, STA ID: ",
      static_cast<int>(config.sta_id()));
  telux::wlan::StaConfig configObj;
  configObj.staId = convertIdFromGrpc(config.sta_id());
  configObj.ipConfig = convertStaIpConfigFromGrpc(config.ip_config());
  configObj.staticIpConfig =
      convertStaStaticIpConfigFromGrpc(config.static_ip_config());
  configObj.bridgeMode = convertStaBridgeModeFromGrpc(config.bridge_mode());
  return configObj;
}

::wlanStub::StaConfig WlanCommonUtilsStub::convertStaConfigToGrpc(
    const telux::wlan::StaConfig &config) {
  LOG(DEBUG, __FUNCTION__, " Converting to Grpc StaConfig, STA ID: ",
      static_cast<int>(config.staId));
  ::wlanStub::StaConfig grpcConfig;
  grpcConfig.set_sta_id(convertIdToGrpc(config.staId));
  grpcConfig.set_ip_config(convertStaIpConfigToGrpc(config.ipConfig));
  *grpcConfig.mutable_static_ip_config() =
      convertStaStaticIpConfigToGrpc(config.staticIpConfig);
  grpcConfig.set_bridge_mode(convertStaBridgeModeToGrpc(config.bridgeMode));
  return grpcConfig;
}

// --- telux::wlan::StaNetworkConfig conversions ---
telux::wlan::StaNetworkConfig
WlanCommonUtilsStub::convertStaNetworkConfigFromGrpc(
    const ::wlanStub::StaNetworkConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc StaNetworkConfig, SSID: ", config.ssid());
  telux::wlan::StaNetworkConfig configObj;
  configObj.ssid = config.ssid();
  configObj.priority = static_cast<telux::wlan::Priority>(config.priority());
  configObj.band = convertBandTypeFromGrpc(config.band());
  configObj.bssid = config.bssid();
  return configObj;
}

::wlanStub::StaNetworkConfig WlanCommonUtilsStub::convertStaNetworkConfigToGrpc(
    const telux::wlan::StaNetworkConfig &config) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc StaNetworkConfig, SSID: ", config.ssid);
  ::wlanStub::StaNetworkConfig grpcConfig;
  grpcConfig.set_ssid(config.ssid);
  grpcConfig.set_priority(config.priority);
  grpcConfig.set_band(convertBandTypeToGrpc(config.band));
  grpcConfig.set_bssid(config.bssid);
  return grpcConfig;
}

// --- telux::wlan::StaNetworkConfigEntry conversions ---
telux::wlan::StaNetworkConfigEntry
WlanCommonUtilsStub::convertStaNetworkConfigEntryFromGrpc(
    const ::wlanStub::StaNetworkConfigEntry &entry) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc StaNetworkConfigEntry, SSID: ",
      entry.sta_network_config().ssid());
  telux::wlan::StaNetworkConfigEntry entryObj;
  entryObj.ssid = entry.sta_network_config().ssid();
  entryObj.priority =
      static_cast<telux::wlan::Priority>(entry.sta_network_config().priority());
  entryObj.band = convertBandTypeFromGrpc(entry.sta_network_config().band());
  entryObj.bssid = entry.sta_network_config().bssid();
  entryObj.passPhrase = entry.pass_phrase();
  entryObj.enable = entry.enable();
  return entryObj;
}

::wlanStub::StaNetworkConfigEntry
WlanCommonUtilsStub::convertStaNetworkConfigEntryToGrpc(
    const telux::wlan::StaNetworkConfigEntry &entry) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc StaNetworkConfigEntry, SSID: ", entry.ssid);
  ::wlanStub::StaNetworkConfigEntry grpcEntry;
  *grpcEntry.mutable_sta_network_config() =
      convertStaNetworkConfigToGrpc(entry);
  grpcEntry.set_pass_phrase(entry.passPhrase);
  grpcEntry.set_enable(entry.enable);
  return grpcEntry;
}

// --- telux::wlan::StaNetworkConfigInfo conversions ---
telux::wlan::StaNetworkConfigInfo
WlanCommonUtilsStub::convertStaNetworkConfigInfoFromGrpc(
    const ::wlanStub::StaNetworkConfigInfo &info) {
  LOG(DEBUG, __FUNCTION__, " Converting from Grpc StaNetworkConfigInfo, SSID: ",
      info.sta_network_config().ssid());
  telux::wlan::StaNetworkConfigInfo infoObj;
  infoObj.ssid = info.sta_network_config().ssid();
  infoObj.priority =
      static_cast<telux::wlan::Priority>(info.sta_network_config().priority());
  infoObj.band = convertBandTypeFromGrpc(info.sta_network_config().band());
  infoObj.bssid = info.sta_network_config().bssid();
  infoObj.networkId = static_cast<telux::wlan::NetworkId>(info.network_id());
  infoObj.isCurrent = info.is_current();
  return infoObj;
}

::wlanStub::StaNetworkConfigInfo
WlanCommonUtilsStub::convertStaNetworkConfigInfoToGrpc(
    const telux::wlan::StaNetworkConfigInfo &info) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc StaNetworkConfigInfo, SSID: ", info.ssid);
  ::wlanStub::StaNetworkConfigInfo grpcInfo;
  *grpcInfo.mutable_sta_network_config() = convertStaNetworkConfigToGrpc(info);
  grpcInfo.set_network_id(info.networkId);
  grpcInfo.set_is_current(info.isCurrent);
  return grpcInfo;
}

// --- telux::wlan::ExternalApInfo conversions ---
telux::wlan::ExternalApInfo WlanCommonUtilsStub::convertExternalApInfoFromGrpc(
    const ::wlanStub::ExternalApInfo &info) {
  LOG(DEBUG, __FUNCTION__,
      " Converting from Grpc ExternalApInfo, SSID: ", info.ssid());
  telux::wlan::ExternalApInfo infoObj;
  infoObj.ssid = info.ssid();
  infoObj.bssid = info.bssid();
  infoObj.band = convertBandTypeFromGrpc(info.band());
  infoObj.securityFlags = info.security_flags();
  infoObj.signalStrength = static_cast<int16_t>(info.signal_strength());
  return infoObj;
}

::wlanStub::ExternalApInfo WlanCommonUtilsStub::convertExternalApInfoToGrpc(
    const telux::wlan::ExternalApInfo &info) {
  LOG(DEBUG, __FUNCTION__,
      " Converting to Grpc ExternalApInfo, SSID: ", info.ssid);
  ::wlanStub::ExternalApInfo grpcInfo;
  grpcInfo.set_ssid(info.ssid);
  grpcInfo.set_bssid(info.bssid);
  grpcInfo.set_band(convertBandTypeToGrpc(info.band));
  grpcInfo.set_security_flags(info.securityFlags);
  grpcInfo.set_signal_strength(info.signalStrength);
  return grpcInfo;
}

// --- telux::wlan::StaScanResult conversions ---
telux::wlan::StaScanResult WlanCommonUtilsStub::convertStaScanResultFromGrpc(
    const ::wlanStub::StaScanResult &result) {
  LOG(DEBUG, __FUNCTION__, " Converting from Grpc StaScanResult, STA ID: ",
      static_cast<int>(result.sta_id()),
      ", ScanComplete: ", result.is_scan_complete());
  telux::wlan::StaScanResult resultObj;
  resultObj.staId = convertIdFromGrpc(result.sta_id());
  for (const auto &ap : result.external_ap_list()) {
    resultObj.externalApList.push_back(convertExternalApInfoFromGrpc(ap));
  }
  resultObj.batchIndex = static_cast<uint8_t>(result.batch_index());
  resultObj.isScanComplete = result.is_scan_complete();
  return resultObj;
}

::wlanStub::StaScanResult WlanCommonUtilsStub::convertStaScanResultToGrpc(
    const telux::wlan::StaScanResult &result) {
  LOG(DEBUG, __FUNCTION__, " Converting to Grpc StaScanResult, STA ID: ",
      static_cast<int>(result.staId),
      ", ScanComplete: ", result.isScanComplete);
  ::wlanStub::StaScanResult grpcResult;
  grpcResult.set_sta_id(convertIdToGrpc(result.staId));
  for (const auto &ap : result.externalApList) {
    *grpcResult.add_external_ap_list() = convertExternalApInfoToGrpc(ap);
  }
  grpcResult.set_batch_index(result.batchIndex);
  grpcResult.set_is_scan_complete(result.isScanComplete);
  return grpcResult;
}

} // namespace wlan
} // namespace telux