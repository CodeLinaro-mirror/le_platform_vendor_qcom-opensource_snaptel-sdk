/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "StaInterfaceManagerServerImpl.hpp"
#include "WlanServerUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/wlan/WlanCommonUtilsStub.hpp"

#define WLAN_API_LOCAL_JSON "api/wlan/IStaInterfaceManager.json"
#define WLAN_STATE_JSON "system-state/wlan/IStaInterfaceManagerState.json"
#define WLAN_DEVICE_MANAGER_STATE_JSON                                         \
  "system-state/wlan/IWlanDeviceManagerState.json"
#define DEFAULT_DELIMITER " "
#define STA_FILTER "wlan_sta"

// Helper function to get configured numSta (sequential count: 1) from
// IWlanDeviceManagerState.json
static int getConfiguredNumStaFromDeviceManagerState() {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error = JsonParser::readFromJsonFile(
      deviceStateRootObj, WLAN_DEVICE_MANAGER_STATE_JSON);
  if (error == telux::common::ErrorCode::SUCCESS &&
      deviceStateRootObj.isMember("IWlanDeviceManager") &&
      deviceStateRootObj["IWlanDeviceManager"].isMember("configuredNumSta")) {
    return deviceStateRootObj["IWlanDeviceManager"]["configuredNumSta"].asInt();
  }
  LOG(WARNING, "getConfiguredNumStaFromDeviceManagerState",
      "Could not read configuredNumSta from ", WLAN_DEVICE_MANAGER_STATE_JSON,
      ". Defaulting to 0.");
  return 0; // Default to 0 if file not found or configuredNumSta not specified
}

// Helper function to get running numSta (bitmask: 1) from
// IWlanDeviceManagerState.json
static int getRunningNumStaBitmaskFromDeviceManagerState() {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error = JsonParser::readFromJsonFile(
      deviceStateRootObj, WLAN_DEVICE_MANAGER_STATE_JSON);
  if (error == telux::common::ErrorCode::SUCCESS &&
      deviceStateRootObj.isMember("IWlanDeviceManager") &&
      deviceStateRootObj["IWlanDeviceManager"].isMember("numSta")) {
    return deviceStateRootObj["IWlanDeviceManager"]["numSta"].asInt();
  }
  LOG(WARNING, "getRunningNumStaBitmaskFromDeviceManagerState",
      "Could not read numSta from ", WLAN_DEVICE_MANAGER_STATE_JSON,
      ". Defaulting to 0.");
  return 0; // Default to 0 if file not found or numSta not specified
}

// Helper function to check if WLAN is enabled
static bool isWlanEnabled() {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error = JsonParser::readFromJsonFile(
      deviceStateRootObj, WLAN_DEVICE_MANAGER_STATE_JSON);
  if (error == telux::common::ErrorCode::SUCCESS &&
      deviceStateRootObj.isMember("IWlanDeviceManager") &&
      deviceStateRootObj["IWlanDeviceManager"].isMember("isEnabled")) {
    return deviceStateRootObj["IWlanDeviceManager"]["isEnabled"].asBool();
  }
  LOG(WARNING, "isWlanEnabled", "Could not read isEnabled from ",
      WLAN_DEVICE_MANAGER_STATE_JSON, ". Defaulting to false.");
  return false; // Default to false if file not found or isEnabled not specified
}

// Helper function to update the 'numSta' bitmask in
// IWlanDeviceManagerState.json
static void updateRunningNumStaBitmaskInDeviceManagerState(
    telux::wlan::Id staId, telux::wlan::ServiceOperation operation) {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error = JsonParser::readFromJsonFile(
      deviceStateRootObj, WLAN_DEVICE_MANAGER_STATE_JSON);
  if (error != telux::common::ErrorCode::SUCCESS ||
      !deviceStateRootObj.isMember("IWlanDeviceManager")) {
    LOG(ERROR, "updateRunningNumStaBitmaskInDeviceManagerState",
        "Could not read or find IWlanDeviceManager from ",
        WLAN_DEVICE_MANAGER_STATE_JSON);
    return;
  }

  int currentNumStaBitmask =
      deviceStateRootObj["IWlanDeviceManager"]["numSta"].asInt();
  // Convert 1-indexed ID to 0-indexed bit position
  int staBit = (1 << (static_cast<int>(staId) - 1));

  if (operation == telux::wlan::ServiceOperation::START) {
    currentNumStaBitmask |= staBit; // Set the bit for this STA
  } else if (operation == telux::wlan::ServiceOperation::STOP) {
    currentNumStaBitmask &= ~staBit; // Clear the bit for this STA
  }

  deviceStateRootObj["IWlanDeviceManager"]["numSta"] = currentNumStaBitmask;
  JsonParser::writeToJsonFile(deviceStateRootObj,
                              WLAN_DEVICE_MANAGER_STATE_JSON);
  LOG(INFO, "updateRunningNumStaBitmaskInDeviceManagerState",
      "Updated IWlanDeviceManager::numSta to: ", currentNumStaBitmask);
}

StaInterfaceManagerServerImpl::StaInterfaceManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
  taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

StaInterfaceManagerServerImpl::~StaInterfaceManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
}

grpc::Status StaInterfaceManagerServerImpl::InitService(
    ServerContext *context, const ::google::protobuf::Empty *request,
    wlanStub::GetServiceStatusReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value rootObj;
  std::string filePath = WLAN_API_LOCAL_JSON;
  telux::common::ErrorCode error =
      JsonParser::readFromJsonFile(rootObj, filePath);
  if (error != telux::common::ErrorCode::SUCCESS) {
    LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ");
    return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
  }

  int cbDelay =
      rootObj["IStaInterfaceManager"]["IsSubsystemReadyDelay"].asInt();
  std::string cbStatus =
      rootObj["IStaInterfaceManager"]["IsSubsystemReady"].asString();
  telux::common::ServiceStatus status =
      telux::common::CommonUtils::mapServiceStatus(cbStatus);
  LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

  response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
  response->set_delay(cbDelay);

  if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    std::vector<std::string> filters = {STA_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(shared_from_this(), filters);
  }

  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::SetIpConfig(
    ServerContext *context, const wlanStub::StaSetIpConfigRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager",
      "setIpConfig", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Get the configured sequential count of STAs from device manager state.
    int configuredStaCount = getConfiguredNumStaFromDeviceManagerState();
    if (static_cast<int>(staId) > configuredStaCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId), " is not configured by configuredNumSta (",
          configuredStaCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    telux::wlan::StaIpConfig ipConfig =
        telux::wlan::WlanCommonUtilsStub::convertStaIpConfigFromGrpc(
            request->ip_config());

    // Find and update the specific STA config by ID, or add if not found.
    Json::Value &configuredStaConfigArray =
        stateRootObj["IStaInterfaceManager"]["configuredStaConfig"];
    bool updated = false;
    for (Json::ArrayIndex i = 0; i < configuredStaConfigArray.size(); ++i) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredStaConfigArray[i]["staId"].asString())) == staId) {
        configuredStaConfigArray[i]["ipConfig"] =
            telux::wlan::server::WlanServerUtils::getStaIpConfigString(
                ipConfig);
        configuredStaConfigArray[i]["staticIpConfig"] =
            telux::wlan::server::WlanServerUtils::setStaStaticIpConfigToJson(
                request->static_ip_config());
        updated = true;
        break;
      }
    }
    if (!updated) {
      LOG(WARNING, __FUNCTION__, "STA ID ", static_cast<int>(staId),
          " not found in configuredStaConfig for IP config update.");
    }
    JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::SetBridgeMode(
    ServerContext *context, const wlanStub::StaSetBridgeModeRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager",
      "setBridgeMode", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Get the configured sequential count of STAs from device manager state.
    int configuredStaCount = getConfiguredNumStaFromDeviceManagerState();
    if (static_cast<int>(staId) > configuredStaCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId), " is not configured by configuredNumSta (",
          configuredStaCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    telux::wlan::StaBridgeMode bridgeMode =
        telux::wlan::WlanCommonUtilsStub::convertStaBridgeModeFromGrpc(
            request->bridge_mode());

    // Find and update the specific STA config by ID, or add if not found.
    Json::Value &configuredStaConfigArray =
        stateRootObj["IStaInterfaceManager"]["configuredStaConfig"];
    bool updated = false;
    for (Json::ArrayIndex i = 0; i < configuredStaConfigArray.size(); ++i) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredStaConfigArray[i]["staId"].asString())) == staId) {
        configuredStaConfigArray[i]["bridgeMode"] =
            telux::wlan::server::WlanServerUtils::getStaBridgeModeString(
                bridgeMode);
        updated = true;
        break;
      }
    }
    if (!updated) {
      LOG(WARNING, __FUNCTION__, "STA ID ", static_cast<int>(staId),
          " not found in configuredStaConfig for bridge mode update.");
    }
    JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::EnableHotspot2(
    ServerContext *context, const wlanStub::StaEnableHotspot2Request *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager",
      "enableHotspot2", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Get the configured sequential count of STAs from device manager state.
    int configuredStaCount = getConfiguredNumStaFromDeviceManagerState();
    if (static_cast<int>(staId) > configuredStaCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId), " is not configured by configuredNumSta (",
          configuredStaCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    // No direct state for Hotspot2 enable in IStaInterfaceManagerState.json
    // If needed, a new field would be added, e.g., "isHotspot2Enabled":
    // true/false
    LOG(INFO, __FUNCTION__,
        " EnableHotspot2 for STA ID: ", static_cast<int>(request->sta_id()),
        ", Enable: ", request->enable());

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::GetConfig(
    ServerContext *context, const ::google::protobuf::Empty *request,
    wlanStub::StaGetConfigResponse *response) { // Request type corrected

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager", "getConfig",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) { // Check if WLAN is enabled
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    // Get the configured sequential count of STAs from device manager state.
    int configuredStaCount = getConfiguredNumStaFromDeviceManagerState();
    if (configuredStaCount == 0) { // If no STA is configured at all
      LOG(WARNING, __FUNCTION__,
          "No STA interfaces configured (configuredNumSta is 0).");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    const Json::Value &configuredStaConfigArray =
        stateRootObj["IStaInterfaceManager"]["configuredStaConfig"];
    if (configuredStaConfigArray.isArray()) {
      for (const auto &staConfigNode : configuredStaConfigArray) {
        telux::wlan::Id teluxStaId =
            telux::wlan::server::WlanServerUtils::convertIdFromGrpc(
                telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                    staConfigNode["staId"].asString()));

        // Filter based on configuredStaCount: Only return configured STAs up to
        // the allowed sequential count.
        if (static_cast<int>(teluxStaId) > configuredStaCount) {
          LOG(INFO, __FUNCTION__, "Skipping configured STA ID ",
              static_cast<int>(teluxStaId), " as it exceeds configuredNumSta (",
              configuredStaCount, ")");
          continue; // Skip this one, it's configured but beyond the limit
        }

        telux::wlan::StaConfig teluxStaConfig;
        teluxStaConfig.staId = teluxStaId;
        teluxStaConfig.ipConfig =
            telux::wlan::server::WlanServerUtils::convertStaIpConfigFromGrpc(
                telux::wlan::server::WlanServerUtils::getStaIpConfigFromString(
                    staConfigNode["ipConfig"].asString()));
        teluxStaConfig.staticIpConfig.ipAddr =
            telux::wlan::server::WlanServerUtils::getStaStaticIpConfigFromPtree(
                staConfigNode["staticIpConfig"])
                .ip_addr();
        teluxStaConfig.staticIpConfig.gwIpAddr =
            telux::wlan::server::WlanServerUtils::getStaStaticIpConfigFromPtree(
                staConfigNode["staticIpConfig"])
                .gw_ip_addr();
        teluxStaConfig.staticIpConfig.netMask =
            telux::wlan::server::WlanServerUtils::getStaStaticIpConfigFromPtree(
                staConfigNode["staticIpConfig"])
                .net_mask();
        teluxStaConfig.staticIpConfig.dnsAddr =
            telux::wlan::server::WlanServerUtils::getStaStaticIpConfigFromPtree(
                staConfigNode["staticIpConfig"])
                .dns_addr();
        teluxStaConfig.bridgeMode =
            telux::wlan::server::WlanServerUtils::convertStaBridgeModeFromGrpc(
                telux::wlan::server::WlanServerUtils::
                    getStaBridgeModeFromString(
                        staConfigNode["bridgeMode"].asString()));

        *response->add_config() =
            telux::wlan::WlanCommonUtilsStub::convertStaConfigToGrpc(
                teluxStaConfig);
      }
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'configuredStaConfig' not found or not an array in state JSON.");
    }

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::GetStatus(
    ServerContext *context, const ::google::protobuf::Empty *request,
    wlanStub::StaGetStatusResponse *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager", "getStatus",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    // Get the active allowed number of STAs (bitmask) from the Device Manager
    // state.
    int activeNumStaBitmask = getRunningNumStaBitmaskFromDeviceManagerState();
    if (activeNumStaBitmask == 0) {
      LOG(WARNING, __FUNCTION__,
          "No STA interfaces are active (numSta bitmask is 0).");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    const Json::Value &staStatusArray =
        stateRootObj["IStaInterfaceManager"]["staStatus"];
    if (staStatusArray.isArray()) {
      for (const auto &staStatusNode : staStatusArray) {
        telux::wlan::Id teluxStaId =
            telux::wlan::server::WlanServerUtils::convertIdFromGrpc(
                telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                    staStatusNode["id"].asString()));

        // Filter based on activeNumStaBitmask.
        if ((activeNumStaBitmask & (1 << (static_cast<int>(teluxStaId) - 1))) ==
            0) {
          LOG(INFO, __FUNCTION__, "Skipping STA ID ",
              static_cast<int>(teluxStaId),
              " in status as it's not enabled by active numSta bitmask (",
              activeNumStaBitmask, ")");
          continue;
        }

        telux::wlan::StaStatus teluxStaStatus;
        teluxStaStatus.id = teluxStaId;
        teluxStaStatus.name = staStatusNode["name"].asString();
        teluxStaStatus.ipv4Address = staStatusNode["ipv4Address"].asString();
        teluxStaStatus.ipv6Address = staStatusNode["ipv6Address"].asString();
        teluxStaStatus.macAddress = staStatusNode["macAddress"].asString();
        teluxStaStatus.status = telux::wlan::server::WlanServerUtils::
            convertStaInterfaceStatusFromGrpc(
                telux::wlan::server::WlanServerUtils::
                    getStaInterfaceStatusFromString(
                        staStatusNode["status"].asString()));
        teluxStaStatus.connectionStatus = telux::wlan::server::WlanServerUtils::
            convertStaConnectionStatusFromGrpc(
                telux::wlan::server::WlanServerUtils::
                    getStaConnectionStatusFromString(
                        staStatusNode["connectionStatus"].asString()));
        *response->add_status() =
            telux::wlan::WlanCommonUtilsStub::convertStaStatusToGrpc(
                teluxStaStatus);
      }
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'staStatus' not found or not an array in state JSON.");
    }

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::StartScan(
    ServerContext *context, const wlanStub::StaStartScanRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager", "startScan",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Get the active allowed number of STAs (bitmask) from the Device Manager
    // state.
    int activeNumStaBitmask = getRunningNumStaBitmaskFromDeviceManagerState();
    if ((activeNumStaBitmask & (1 << (static_cast<int>(staId) - 1))) == 0) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId),
          " is not enabled by current active numSta bitmask (",
          activeNumStaBitmask, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    // Trigger OnStaScanResultUpdated event.
    // Using the predefined staScanResult from the state JSON.
    if (stateRootObj["IStaInterfaceManager"].isMember("staScanResult")) {
      wlanStub::OnStaScanResultUpdated scanResultEvent;
      // Assuming staScanResult in JSON is global or the first in array, or
      // needs filtering by staId
      *scanResultEvent.mutable_sta_scan_result() =
          telux::wlan::server::WlanServerUtils::getStaScanResultFromPtree(
              stateRootObj["IStaInterfaceManager"]["staScanResult"]);

      ::eventService::EventResponse anyResponse;
      anyResponse.set_filter(STA_FILTER);
      anyResponse.mutable_any()->PackFrom(scanResultEvent);
      EventService::getInstance().updateEventQueue(anyResponse);
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'staScanResult' not found in state JSON for scan simulation.");
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::AddNetworkConfig(
    ServerContext *context, const wlanStub::StaAddNetworkConfigRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager",
      "addNetworkConfig", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Get the active allowed number of STAs (bitmask) from the Device Manager
    // state.
    int activeNumStaBitmask = getRunningNumStaBitmaskFromDeviceManagerState();
    if ((activeNumStaBitmask & (1 << (static_cast<int>(staId) - 1))) == 0) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId),
          " is not enabled by current active numSta bitmask (",
          activeNumStaBitmask, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    Json::Value &networkConfigsArray =
        stateRootObj["IStaInterfaceManager"]["networkConfigs"];

    // Create a new network config entry JSON object
    Json::Value newNetConfig;
    newNetConfig["networkId"] =
        networkConfigsArray.size() + 1; // Simple incrementing ID
    newNetConfig["isCurrent"] = false;
    newNetConfig["sta_network_config"] =
        telux::wlan::server::WlanServerUtils::setStaNetworkConfigToJson(
            request->network().sta_network_config());

    networkConfigsArray.append(newNetConfig);
    JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::RemoveNetworkConfig(
    ServerContext *context,
    const wlanStub::StaRemoveNetworkConfigRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager",
      "removeNetworkConfig", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Get the active allowed number of STAs (bitmask) from the Device Manager
    // state.
    int activeNumStaBitmask = getRunningNumStaBitmaskFromDeviceManagerState();
    if ((activeNumStaBitmask & (1 << (static_cast<int>(staId) - 1))) == 0) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId),
          " is not enabled by current active numSta bitmask (",
          activeNumStaBitmask, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    Json::Value &networkConfigsArray =
        stateRootObj["IStaInterfaceManager"]["networkConfigs"];
    Json::Value newNetworkConfigsArray(Json::arrayValue);
    for (const auto &netConfigNode : networkConfigsArray) {
      if (netConfigNode["networkId"].asUInt() != request->network_id()) {
        newNetworkConfigsArray.append(netConfigNode);
      }
    }
    networkConfigsArray = newNetworkConfigsArray;
    JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::GetNetworkConfigs(
    ServerContext *context,
    const wlanStub::StaGetNetworkConfigsRequest *request,
    wlanStub::StaGetNetworkConfigsResponse
        *response) { // Request type corrected

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager",
      "getNetworkConfigs", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Get the active allowed number of STAs (bitmask) from the Device Manager
    // state.
    int activeNumStaBitmask = getRunningNumStaBitmaskFromDeviceManagerState();
    if ((activeNumStaBitmask & (1 << (static_cast<int>(staId) - 1))) == 0) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId),
          " is not enabled by current active numSta bitmask (",
          activeNumStaBitmask, ").");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    const Json::Value &networkConfigsArray =
        stateRootObj["IStaInterfaceManager"]["networkConfigs"];
    if (networkConfigsArray.isArray()) {
      for (const auto &netConfigNode : networkConfigsArray) {
        wlanStub::StaNetworkConfigInfo networkInfo;
        networkInfo.set_network_id(netConfigNode["networkId"].asUInt());
        networkInfo.set_is_current(netConfigNode["isCurrent"].asBool());
        *networkInfo.mutable_sta_network_config() =
            telux::wlan::server::WlanServerUtils::getStaNetworkConfigFromPtree(
                netConfigNode["sta_network_config"]);
        *response->add_network() = networkInfo;
      }
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'networkConfigs' not found or not an array in state JSON.");
    }

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::Connect(
    ServerContext *context, const wlanStub::StaConnectRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager", "connect",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Check if the STA ID is configured in the device manager.
    int configuredStaCount = getConfiguredNumStaFromDeviceManagerState();
    if (static_cast<int>(staId) > configuredStaCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId), " is not configured by configuredNumSta (",
          configuredStaCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    // Simulate a connection by updating StaStatus
    Json::Value &staStatusArray =
        stateRootObj["IStaInterfaceManager"]["staStatus"];
    Json::Value *staNode =
        telux::wlan::server::WlanServerUtils::findStaStatusEntry(staStatusArray,
                                                                 staId);
    bool statusUpdated = false;

    if (staNode) {
      // Only proceed if it's not already connected
      if ((*staNode)["status"].asString() ==
          telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
              telux::wlan::StaInterfaceStatus::CONNECTED)) {
        LOG(INFO, __FUNCTION__, "STA ID ", static_cast<int>(staId),
            " is already connected. Returning NO_EFFECT.");
        response->set_error(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::NO_EFFECT));
        return grpc::Status::OK;
      }

      // Simulate CONNECTING state
      (*staNode)["status"] =
          telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
              telux::wlan::StaInterfaceStatus::CONNECTING);
      (*staNode)["connectionStatus"] =
          telux::wlan::server::WlanServerUtils::getStaConnectionStatusString(
              telux::wlan::StaConnectionStatus::UNKNOWN);
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

      // Get actual interface name for event
      std::vector<std::string> staInterfaceNames =
          telux::wlan::server::WlanServerUtils::getInterfaceNames(
              "sim.wlan.sta_interfaces");
      std::string ifaceName;
      if (static_cast<size_t>(static_cast<int>(staId) - 1) <
          staInterfaceNames.size()) {
        ifaceName = staInterfaceNames[static_cast<int>(staId) - 1];
        (*staNode)["name"] = ifaceName;
        telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
            staId, telux::wlan::StaInterfaceStatus::CONNECTING,
            telux::wlan::StaConnectionStatus::UNKNOWN, ifaceName,
            "", // Name from configured interface
            "");
      } else {
        ifaceName =
            "wlan_sta" + std::to_string(static_cast<int>(staId)); // Fallback
        (*staNode)["name"] = ifaceName;
        telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
            staId, telux::wlan::StaInterfaceStatus::CONNECTING,
            telux::wlan::StaConnectionStatus::UNKNOWN, ifaceName,
            "", // Fallback name
            "");
      }

      std::this_thread::sleep_for(
          std::chrono::milliseconds(500)); // Simulate delay

      // Simulate CONNECTED state
      if (static_cast<size_t>(static_cast<int>(staId) - 1) <
          staInterfaceNames.size()) {
        (*staNode)["ipv4Address"] =
            telux::wlan::server::WlanServerUtils::getIpv4Address(ifaceName);
        (*staNode)["macAddress"] =
            telux::wlan::server::WlanServerUtils::getMacAddress(ifaceName);
        (*staNode)["ipv6Address"] =
            telux::wlan::server::WlanServerUtils::getIpv6Address(ifaceName);
      } else {
        // Fallback to generated IP/MAC if interface name was fallback
        (*staNode)["ipv4Address"] =
            "192.168.1." +
            std::to_string(static_cast<int>(staId)); // Placeholder IP
        (*staNode)["macAddress"] =
            "00:AA:BB:CC:DD:" +
            ((static_cast<int>(staId) < 10)
                 ? ("0" + std::to_string(static_cast<int>(staId)))
                 : std::to_string(static_cast<int>(staId))); // Placeholder MAC
        (*staNode)["ipv6Address"] = ""; // No dynamic IPv6 for fallback
      }

      (*staNode)["status"] =
          telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
              telux::wlan::StaInterfaceStatus::CONNECTED);
      (*staNode)["connectionStatus"] =
          telux::wlan::server::WlanServerUtils::getStaConnectionStatusString(
              telux::wlan::StaConnectionStatus::SUCCESS);
      statusUpdated = true;
    }

    if (statusUpdated) {
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
          staId, telux::wlan::StaInterfaceStatus::CONNECTED,
          telux::wlan::StaConnectionStatus::SUCCESS,
          (*staNode)["name"].asString(), (*staNode)["ipv4Address"].asString(),
          (*staNode)["macAddress"].asString());
    } else {
      LOG(WARNING, __FUNCTION__, "STA ID ", static_cast<int>(staId),
          " not found in staStatus for Connect.");
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::Disconnect(
    ServerContext *context, const wlanStub::StaDisconnectRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager",
      "disconnect", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Check if the STA ID is configured in the device manager.
    int configuredStaCount = getConfiguredNumStaFromDeviceManagerState();
    if (static_cast<int>(staId) > configuredStaCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId), " is not configured by configuredNumSta (",
          configuredStaCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    // Simulate a disconnection by updating StaStatus
    Json::Value &staStatusArray =
        stateRootObj["IStaInterfaceManager"]["staStatus"];
    Json::Value *staNode =
        telux::wlan::server::WlanServerUtils::findStaStatusEntry(staStatusArray,
                                                                 staId);
    bool statusUpdated = false;

    if (staNode) {
      if ((*staNode)["status"].asString() ==
          telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
              telux::wlan::StaInterfaceStatus::DISCONNECTED)) {
        LOG(INFO, __FUNCTION__, "STA ID ", static_cast<int>(staId),
            " is already disconnected. Returning NO_EFFECT.");
        response->set_error(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::NO_EFFECT));
        return grpc::Status::OK;
      }
      (*staNode)["status"] =
          telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
              telux::wlan::StaInterfaceStatus::DISCONNECTED);
      (*staNode)["connectionStatus"] =
          telux::wlan::server::WlanServerUtils::getStaConnectionStatusString(
              telux::wlan::StaConnectionStatus::SUCCESS); // Still success but
                                                          // disconnected
      (*staNode)["name"] = "";
      (*staNode)["ipv4Address"] = "";
      (*staNode)["macAddress"] = "";
      statusUpdated = true;
    }

    if (statusUpdated) {
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
          staId, telux::wlan::StaInterfaceStatus::DISCONNECTED,
          telux::wlan::StaConnectionStatus::SUCCESS, "", "",
          ""); // Empty for disconnected
    } else {
      LOG(WARNING, __FUNCTION__, "STA ID ", static_cast<int>(staId),
          " not found in staStatus for Disconnect.");
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status StaInterfaceManagerServerImpl::ManageStaService(
    ServerContext *context, const wlanStub::StaManageStaServiceRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IStaInterfaceManager",
      "manageStaService", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__, "Operation denied. WLAN is not enabled.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    telux::wlan::Id staId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->sta_id());
    // Get the configured sequential count of STAs from device manager state.
    int configuredStaCount = getConfiguredNumStaFromDeviceManagerState();
    if (static_cast<int>(staId) > configuredStaCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. STA ID ",
          static_cast<int>(staId), " is not configured by configuredNumSta (",
          configuredStaCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    telux::wlan::ServiceOperation opr =
        telux::wlan::WlanCommonUtilsStub::convertServiceOperationFromGrpc(
            request->opr());
    LOG(INFO, __FUNCTION__,
        " ManageStaService for STA ID: ", static_cast<int>(staId),
        ", Operation: ", static_cast<int>(opr));

    Json::Value &staStatusArray =
        stateRootObj["IStaInterfaceManager"]["staStatus"];
    Json::Value *staNode =
        telux::wlan::server::WlanServerUtils::findStaStatusEntry(staStatusArray,
                                                                 staId);

    if (!staNode) {
      LOG(WARNING, __FUNCTION__, "STA ID ", static_cast<int>(staId),
          " not found in staStatus JSON. Cannot manage service.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    std::string currentStatusStr = (*staNode)["status"].asString();
    telux::wlan::StaInterfaceStatus currentStatus =
        telux::wlan::server::WlanServerUtils::convertStaInterfaceStatusFromGrpc(
            telux::wlan::server::WlanServerUtils::
                getStaInterfaceStatusFromString(currentStatusStr));

    // Determine ifaceName for event
    std::vector<std::string> staInterfaceNames =
        telux::wlan::server::WlanServerUtils::getInterfaceNames(
            "sim.wlan.sta_interfaces");
    std::string ifaceName;
    bool isFallbackIface = false;
    if (static_cast<size_t>(static_cast<int>(staId) - 1) <
        staInterfaceNames.size()) {
      ifaceName = staInterfaceNames[static_cast<int>(staId) - 1];
    } else {
      ifaceName =
          "wlan_sta" + std::to_string(static_cast<int>(staId)); // Fallback
      isFallbackIface = true;
    }

    if (opr == telux::wlan::ServiceOperation::START ||
        opr == telux::wlan::ServiceOperation::RESTART) {
      if (currentStatus == telux::wlan::StaInterfaceStatus::CONNECTED &&
          opr == telux::wlan::ServiceOperation::START) {
        LOG(INFO, __FUNCTION__, "STA ID ", static_cast<int>(staId),
            " is already connected. Returning NO_EFFECT for START operation.");
        response->set_error(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::NO_EFFECT));
        return grpc::Status::OK;
      }

      // If RESTART or if START and not currently DISCONNECTED, simulate a
      // disconnect first.
      if (opr == telux::wlan::ServiceOperation::RESTART &&
          currentStatus != telux::wlan::StaInterfaceStatus::DISCONNECTED) {
        LOG(INFO, __FUNCTION__, "STA ID ", static_cast<int>(staId),
            " is currently ", currentStatusStr,
            ". Simulating DISCONNECT before RESTART.");
        // Update internal state to DISCONNECTED
        (*staNode)["status"] =
            telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
                telux::wlan::StaInterfaceStatus::DISCONNECTED);
        (*staNode)["connectionStatus"] =
            telux::wlan::server::WlanServerUtils::getStaConnectionStatusString(
                telux::wlan::StaConnectionStatus::SUCCESS);
        (*staNode)["name"] = "";
        (*staNode)["ipv4Address"] = "";
        (*staNode)["macAddress"] = "";
        (*staNode)["ipv6Address"] = "";
        JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
        telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
            staId, telux::wlan::StaInterfaceStatus::DISCONNECTED,
            telux::wlan::StaConnectionStatus::SUCCESS, "", "",
            ""); // Event for disconnected
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
      } else if (opr == telux::wlan::ServiceOperation::START &&
                 currentStatus !=
                     telux::wlan::StaInterfaceStatus::DISCONNECTED) {
        // If START is requested and STA is not DISCONNECTED, it should be
        // CONNECTED and handled by the NO_EFFECT above. If it's CONNECTING, it
        // should wait for CONNECTED. For simulation, assume we just become
        // CONNECTED.
        LOG(INFO, __FUNCTION__, "STA ID ", static_cast<int>(staId),
            " is in state ", currentStatusStr,
            " for START operation. Attempting to force CONNECTED.");
      }

      // Simulate START sequence
      LOG(INFO, __FUNCTION__, "STA ID ", static_cast<int>(staId),
          " simulating START sequence for operation:",
          (opr == telux::wlan::ServiceOperation::START ? "START" : "RESTART"));

      // 1. CONNECTING state
      (*staNode)["status"] =
          telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
              telux::wlan::StaInterfaceStatus::CONNECTING);
      (*staNode)["connectionStatus"] =
          telux::wlan::server::WlanServerUtils::getStaConnectionStatusString(
              telux::wlan::StaConnectionStatus::UNKNOWN);
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
          staId, telux::wlan::StaInterfaceStatus::CONNECTING,
          telux::wlan::StaConnectionStatus::UNKNOWN, ifaceName, "",
          ""); // Name from newStaStatus["name"], IP, MAC not yet known in
               // CONNECTING
      std::this_thread::sleep_for(
          std::chrono::milliseconds(500)); // Simulate connection time

      // 2. CONNECTED state
      (*staNode)["name"] = ifaceName;
      if (!isFallbackIface) {
        (*staNode)["ipv4Address"] =
            telux::wlan::server::WlanServerUtils::getIpv4Address(ifaceName);
        (*staNode)["macAddress"] =
            telux::wlan::server::WlanServerUtils::getMacAddress(ifaceName);
        (*staNode)["ipv6Address"] =
            telux::wlan::server::WlanServerUtils::getIpv6Address(ifaceName);
      } else {
        // Fallback to generated IP/MAC if interface name was fallback
        (*staNode)["ipv4Address"] =
            "192.168.1." +
            std::to_string(static_cast<int>(staId)); // Placeholder IP
        (*staNode)["macAddress"] =
            "00:AA:BB:CC:DD:" +
            ((static_cast<int>(staId) < 10)
                 ? ("0" + std::to_string(static_cast<int>(staId)))
                 : std::to_string(static_cast<int>(staId))); // Placeholder MAC
        (*staNode)["ipv6Address"] = ""; // No dynamic IPv6 for fallback
      }

      (*staNode)["status"] =
          telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
              telux::wlan::StaInterfaceStatus::CONNECTED);
      (*staNode)["connectionStatus"] =
          telux::wlan::server::WlanServerUtils::getStaConnectionStatusString(
              telux::wlan::StaConnectionStatus::SUCCESS);

      updateRunningNumStaBitmaskInDeviceManagerState(
          staId, telux::wlan::ServiceOperation::START);
      // After START/RESTART sequence, send the final CONNECTED event
      telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
          staId, telux::wlan::StaInterfaceStatus::CONNECTED,
          telux::wlan::StaConnectionStatus::SUCCESS,
          (*staNode)["name"].asString(), (*staNode)["ipv4Address"].asString(),
          (*staNode)["macAddress"].asString());
      JsonParser::writeToJsonFile(stateRootObj,
                                  WLAN_STATE_JSON); // Write final state
    } else if (opr == telux::wlan::ServiceOperation::STOP) {
      LOG(INFO, __FUNCTION__, "STA ID ", static_cast<int>(staId),
          " simulated STOP.");
      if (currentStatus == telux::wlan::StaInterfaceStatus::DISCONNECTED) {
        LOG(INFO, __FUNCTION__, "STA ID ", static_cast<int>(staId),
            " is already stopped. Returning NO_EFFECT.");
        response->set_error(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::NO_EFFECT));
        return grpc::Status::OK;
      }
      (*staNode)["name"] = "";
      (*staNode)["ipv4Address"] = "";
      (*staNode)["macAddress"] = "";
      (*staNode)["ipv6Address"] = "";
      (*staNode)["status"] =
          telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
              telux::wlan::StaInterfaceStatus::DISCONNECTED);
      (*staNode)["connectionStatus"] =
          telux::wlan::server::WlanServerUtils::getStaConnectionStatusString(
              telux::wlan::StaConnectionStatus::UNKNOWN);

      updateRunningNumStaBitmaskInDeviceManagerState(
          staId, telux::wlan::ServiceOperation::STOP);
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
          staId, telux::wlan::StaInterfaceStatus::DISCONNECTED,
          telux::wlan::StaConnectionStatus::UNKNOWN, "", "",
          ""); // Empty for disconnected
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

void StaInterfaceManagerServerImpl::onEventUpdate(
    ::eventService::UnsolicitedEvent message) {
  if (message.filter() == STA_FILTER) {
    onEventUpdate(message.event());
  }
}

void StaInterfaceManagerServerImpl::onEventUpdate(std::string event) {
  std::string token =
      telux::common::EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
  LOG(DEBUG, __FUNCTION__, "Received event token: ", token);
  if (token == "onStaStatusChanged") {
    handleStaStatusChangedEvent(event);
  } else if (token == "onStaScanResultUpdated") {
    handleStaScanResultUpdatedEvent(event);
  } else if (token == "onStaBandChanged") {
    handleStaBandChangedEvent(event);
  } else {
    LOG(ERROR, __FUNCTION__, "Unhandled event flag: ", token);
  }
}

void StaInterfaceManagerServerImpl::handleStaStatusChangedEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);
  std::string staIdIntStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  std::string ifaceStatusIntStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  std::string connStatusIntStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);

  try {
    // Convert STA ID string (e.g., "1") to Telux ID enum (e.g.,
    // telux::wlan::Id::PRIMARY)
    telux::wlan::Id staId =
        telux::wlan::server::WlanServerUtils::convertIdFromGrpc(
            telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                staIdIntStr));

    // Before posting event, check if this STA ID is currently active (part of
    // the bitmask)
    int activeNumStaBitmask = getRunningNumStaBitmaskFromDeviceManagerState();
    if ((activeNumStaBitmask & (1 << (static_cast<int>(staId) - 1))) == 0) {
      LOG(INFO, __FUNCTION__, "Ignoring onStaStatusChanged event for STA ID ",
          static_cast<int>(staId),
          " as it's not currently active by numSta bitmask.");
      return; // Don't post event for inactive STAs
    }

    // Convert interface status integer string (e.g., "2") to Telux enum (e.g.,
    // telux::wlan::StaInterfaceStatus::CONNECTED)
    telux::wlan::StaInterfaceStatus newIfaceStatus =
        telux::wlan::server::WlanServerUtils::convertStaInterfaceStatusFromGrpc(
            telux::wlan::server::WlanServerUtils::
                getStaInterfaceStatusFromString(
                    telux::wlan::server::WlanServerUtils::
                        getStaInterfaceStatusStringFromInt(ifaceStatusIntStr)));

    // Convert connection status integer string (e.g., "1") to Telux enum (e.g.,
    // telux::wlan::StaConnectionStatus::SUCCESS)
    telux::wlan::StaConnectionStatus newConnStatus = telux::wlan::server::
        WlanServerUtils::convertStaConnectionStatusFromGrpc(
            telux::wlan::server::WlanServerUtils::
                getStaConnectionStatusFromString(
                    telux::wlan::server::WlanServerUtils::
                        getStaConnectionStatusStringFromInt(connStatusIntStr)));

    std::string ifaceName = "";
    std::string ipv4 = "";
    std::string ipv6 = "";
    std::string mac = "";

    // Dynamically get interface name, IP, and MAC if connected
    if (newIfaceStatus == telux::wlan::StaInterfaceStatus::CONNECTED &&
        newConnStatus == telux::wlan::StaConnectionStatus::SUCCESS) {
      std::vector<std::string> staInterfaceNames =
          telux::wlan::server::WlanServerUtils::getInterfaceNames(
              "sim.wlan.sta_interfaces");
      if (static_cast<size_t>(static_cast<int>(staId) - 1) <
          staInterfaceNames.size()) {
        ifaceName = staInterfaceNames[static_cast<int>(staId) - 1];
        ipv4 = telux::wlan::server::WlanServerUtils::getIpv4Address(ifaceName);
        ipv6 = telux::wlan::server::WlanServerUtils::getIpv6Address(ifaceName);
        mac = telux::wlan::server::WlanServerUtils::getMacAddress(ifaceName);
      } else {
        // Fallback for simulation if interface not found
        ifaceName = "wlan_sta" + std::to_string(static_cast<int>(staId));
        ipv4 = "192.168.1." +
               std::to_string(static_cast<int>(staId)); // Placeholder IP
        mac =
            "00:AA:BB:CC:DD:" +
            ((static_cast<int>(staId) < 10)
                 ? ("0" + std::to_string(static_cast<int>(staId)))
                 : std::to_string(static_cast<int>(staId))); // Placeholder MAC
        ipv6 = ""; // No dynamic IPv6 for fallback
      }
    } else {
      // For disconnected or failed states, clear interface-specific info
      // These will be passed as empty to sendStaStatusChangedEvent and
      // reflected in JSON.
      ifaceName = "";
      ipv4 = "";
      ipv6 = "";
      mac = "";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    // Call the optimized helper function. This function now handles:
    // 1. Updating the staStatus entry in WLAN_STATE_JSON
    // 2. Writing the updated state back to the JSON file
    // 3. Constructing and sending the gRPC OnStaStatusChanged event
    telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
        staId, newIfaceStatus, newConnStatus, ifaceName, ipv4, mac, ipv6);

    LOG(DEBUG, __FUNCTION__, " Posted OnStaStatusChanged event for STA ID: ",
        static_cast<int>(staId), ", interface status: ",
        telux::wlan::server::WlanServerUtils::getStaInterfaceStatusString(
            newIfaceStatus),
        ", connection status: ",
        telux::wlan::server::WlanServerUtils::getStaConnectionStatusString(
            newConnStatus));

  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing onStaStatusChanged event: ",
        ex.what());
  }
}
void StaInterfaceManagerServerImpl::handleStaScanResultUpdatedEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);
  // The event string might contain parameters for the scan result, e.g.,
  // "PRIMARY BATCH 0 COMPLETE" For this simulation, we'll just re-read the
  // static scan result from state JSON.
  std::string staIdStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  try {
    telux::wlan::Id staId =
        telux::wlan::server::WlanServerUtils::convertIdFromGrpc(
            telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                staIdStr));

    // Before posting event, check if this STA ID is currently active (part of
    // the bitmask)
    int activeNumStaBitmask = getRunningNumStaBitmaskFromDeviceManagerState();
    if ((activeNumStaBitmask & (1 << (static_cast<int>(staId) - 1))) == 0) {
      LOG(INFO, __FUNCTION__,
          "Ignoring onStaScanResultUpdated event for STA ID ",
          static_cast<int>(staId),
          " as it's not currently active by numSta bitmask.");
      return; // Don't post event for inactive STAs
    }

    Json::Value stateRootObj_sta_scan;
    JsonParser::readFromJsonFile(stateRootObj_sta_scan, WLAN_STATE_JSON);

    if (stateRootObj_sta_scan["IStaInterfaceManager"].isMember(
            "staScanResult")) {
      std::this_thread::sleep_for(std::chrono::milliseconds(200));

      ::eventService::EventResponse anyResponse;
      wlanStub::OnStaScanResultUpdated grpcEvent;
      *grpcEvent.mutable_sta_scan_result() =
          telux::wlan::server::WlanServerUtils::getStaScanResultFromPtree(
              stateRootObj_sta_scan["IStaInterfaceManager"]["staScanResult"]);
      anyResponse.set_filter(STA_FILTER);
      anyResponse.mutable_any()->PackFrom(grpcEvent);
      EventService::getInstance().updateEventQueue(anyResponse);
      LOG(DEBUG, __FUNCTION__,
          " Posted OnStaScanResultUpdated event for STA ID: ", staIdStr);
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'staScanResult' not found in state JSON for event simulation.");
    }
  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing onStaScanResultUpdated event: ",
        ex.what());
  }
}

void StaInterfaceManagerServerImpl::handleStaBandChangedEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);
  std::string bandTypeStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  try {
    // The getBandTypeFromString now returns wlanStub::BandType, convert it to
    // telux::wlan::BandType
    telux::wlan::BandType band =
        telux::wlan::server::WlanServerUtils::convertBandTypeFromGrpc(
            telux::wlan::server::WlanServerUtils::getBandTypeFromString(
                bandTypeStr));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ::eventService::EventResponse anyResponse;
    wlanStub::OnStaBandChanged grpcEvent;
    // The grpcEvent.set_band() expects wlanStub::BandType, so use
    // convertBandTypeToGrpc with the Telux enum.
    grpcEvent.set_band(
        telux::wlan::server::WlanServerUtils::convertBandTypeToGrpc(band));
    anyResponse.set_filter(STA_FILTER);
    anyResponse.mutable_any()->PackFrom(grpcEvent);
    EventService::getInstance().updateEventQueue(anyResponse);
    LOG(DEBUG, __FUNCTION__,
        " Posted OnStaBandChanged event for Band: ", bandTypeStr);

  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing onStaBandChanged event: ", ex.what());
  }
}