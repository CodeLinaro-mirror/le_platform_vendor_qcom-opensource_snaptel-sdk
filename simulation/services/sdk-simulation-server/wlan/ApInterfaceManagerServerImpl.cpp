/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>
#include <vector>

#include "ApInterfaceManagerServerImpl.hpp"
#include "WlanServerUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/wlan/WlanCommonUtilsStub.hpp"

// Define API and State JSON file paths
#define WLAN_API_LOCAL_JSON "api/wlan/IApInterfaceManager.json"
#define WLAN_STATE_JSON "system-state/wlan/IApInterfaceManagerState.json"
#define WLAN_DEVICE_MANAGER_STATE_JSON                                         \
  "system-state/wlan/IWlanDeviceManagerState.json"
#define DEFAULT_DELIMITER " "
#define AP_FILTER "wlan_ap"

// Helper function to get configured numAp (sequential count: 1, 2, or 3) from
// IWlanDeviceManagerState.json
static int getConfiguredNumApFromDeviceManagerState() {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error = JsonParser::readFromJsonFile(
      deviceStateRootObj, WLAN_DEVICE_MANAGER_STATE_JSON);
  if (error == telux::common::ErrorCode::SUCCESS &&
      deviceStateRootObj.isMember("IWlanDeviceManager") &&
      deviceStateRootObj["IWlanDeviceManager"].isMember("configuredNumAp")) {
    return deviceStateRootObj["IWlanDeviceManager"]["configuredNumAp"].asInt();
  }
  LOG(WARNING, "getConfiguredNumApFromDeviceManagerState",
      "Could not read configuredNumAp from ", WLAN_DEVICE_MANAGER_STATE_JSON,
      ". Defaulting to 0.");
  return 0; // Default to 0 if file not found or configuredNumAp not specified
}

// Helper function to get running numAp (bitmask: 1, 3, or 7) from
// IWlanDeviceManagerState.json
static int getRunningNumApBitmaskFromDeviceManagerState() {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error = JsonParser::readFromJsonFile(
      deviceStateRootObj, WLAN_DEVICE_MANAGER_STATE_JSON);
  if (error == telux::common::ErrorCode::SUCCESS &&
      deviceStateRootObj.isMember("IWlanDeviceManager") &&
      deviceStateRootObj["IWlanDeviceManager"].isMember("numAp")) {
    return deviceStateRootObj["IWlanDeviceManager"]["numAp"].asInt();
  }
  LOG(WARNING, "getRunningNumApBitmaskFromDeviceManagerState",
      "Could not read numAp from ", WLAN_DEVICE_MANAGER_STATE_JSON,
      ". Defaulting to 0.");
  return 0; // Default to 0 if file not found or numAp not specified
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

// Helper function to update the 'numAp' bitmask in IWlanDeviceManagerState.json
// This helper is crucial for `ManageApService` to reflect the running state
// change
static void updateRunningNumApBitmaskInDeviceManagerState(
    telux::wlan::Id apId, telux::wlan::ServiceOperation operation) {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error = JsonParser::readFromJsonFile(
      deviceStateRootObj, WLAN_DEVICE_MANAGER_STATE_JSON);
  if (error != telux::common::ErrorCode::SUCCESS ||
      !deviceStateRootObj.isMember("IWlanDeviceManager")) {
    LOG(ERROR, "updateRunningNumApBitmaskInDeviceManagerState",
        "Could not read or find IWlanDeviceManager from ",
        WLAN_DEVICE_MANAGER_STATE_JSON);
    return;
  }

  int currentNumApBitmask =
      deviceStateRootObj["IWlanDeviceManager"]["numAp"].asInt();
  int apBit = (1 << (static_cast<int>(apId) -
                     1)); // Convert 1-indexed ID to 0-indexed bit

  if (operation == telux::wlan::ServiceOperation::START) {
    currentNumApBitmask |= apBit; // Set the bit for this AP
  } else if (operation == telux::wlan::ServiceOperation::STOP) {
    currentNumApBitmask &= ~apBit; // Clear the bit for this AP
  }

  deviceStateRootObj["IWlanDeviceManager"]["numAp"] = currentNumApBitmask;
  JsonParser::writeToJsonFile(deviceStateRootObj,
                              WLAN_DEVICE_MANAGER_STATE_JSON);
  LOG(INFO, "updateRunningNumApBitmaskInDeviceManagerState",
      "Updated IWlanDeviceManager::numAp to: ", currentNumApBitmask);
}

ApInterfaceManagerServerImpl::ApInterfaceManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
  taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();

  // Initialize/validate IApInterfaceManagerState.json upon construction
  Json::Value stateRootObj;
  telux::common::ErrorCode error =
      JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  // If file read failed or "IApInterfaceManager" is missing, create default
  // structure
  if (error != telux::common::ErrorCode::SUCCESS ||
      !stateRootObj.isMember("IApInterfaceManager")) {
    LOG(INFO, __FUNCTION__,
        " 'IApInterfaceManager' object not found or file read failed in ",
        WLAN_STATE_JSON, ". Initializing with default structure.");
    stateRootObj["IApInterfaceManager"] = Json::Value(Json::objectValue);
  }

  // Initialize configuredApConfig if not present or empty
  if (!stateRootObj["IApInterfaceManager"].isMember("configuredApConfig") ||
      stateRootObj["IApInterfaceManager"]["configuredApConfig"].empty()) {
    LOG(INFO, __FUNCTION__, " 'configuredApConfig' not found or empty in ",
        WLAN_STATE_JSON, ". Initializing with default config for PRIMARY.");

    Json::Value defaultApConfig;
    defaultApConfig["id"] = "PRIMARY";
    defaultApConfig["venue"]["type"] = 1;
    defaultApConfig["venue"]["group"] = 1;

    Json::Value defaultNetworkConfig;
    defaultNetworkConfig["info"]["apRadio"] = "BAND_5GHZ";
    defaultNetworkConfig["info"]["apType"] = "PRIVATE";
    defaultNetworkConfig["ssid"] = "MyTeluxAP_Default";
    defaultNetworkConfig["isVisible"] = true;
    defaultNetworkConfig["elementInfoConfig"]["isEnabled"] = false;
    defaultNetworkConfig["elementInfoConfig"]["isInterworkingEnabled"] = false;
    defaultNetworkConfig["elementInfoConfig"]["netAccessType"] =
        "PRIVATE_ACCESS";
    defaultNetworkConfig["elementInfoConfig"]["internet"] = true;
    defaultNetworkConfig["elementInfoConfig"]["asra"] = false;
    defaultNetworkConfig["elementInfoConfig"]["esr"] = false;
    defaultNetworkConfig["elementInfoConfig"]["uesa"] = false;
    defaultNetworkConfig["elementInfoConfig"]["venueGroup"] = 0;
    defaultNetworkConfig["elementInfoConfig"]["venueType"] = 0;
    defaultNetworkConfig["elementInfoConfig"]["hessid"] = "";
    defaultNetworkConfig["elementInfoConfig"]["vendorElements"] = "";
    defaultNetworkConfig["elementInfoConfig"]["assocRespElements"] = "";
    defaultNetworkConfig["interworking"] = "FULL_ACCESS";
    defaultNetworkConfig["apSecurity"]["mode"] = "WPA2";
    defaultNetworkConfig["apSecurity"]["auth"] = "PSK";
    defaultNetworkConfig["apSecurity"]["encrypt"] = "AES";
    defaultNetworkConfig["passPhrase"] = "DefaultPassword123";

    defaultApConfig["network"].append(defaultNetworkConfig);
    stateRootObj["IApInterfaceManager"]["configuredApConfig"].append(
        defaultApConfig);
  }

  // Initialize apStatus as an empty array (it will be populated dynamically
  // when APs are started)
  if (!stateRootObj["IApInterfaceManager"].isMember("apStatus") ||
      !stateRootObj["IApInterfaceManager"]["apStatus"].isArray()) {
    LOG(INFO, __FUNCTION__, " 'apStatus' not found or not an array in ",
        WLAN_STATE_JSON, ". Initializing as empty array.");
    stateRootObj["IApInterfaceManager"]["apStatus"] =
        Json::Value(Json::arrayValue);
  } else if (!stateRootObj["IApInterfaceManager"]["apStatus"].empty()) {
    LOG(INFO, __FUNCTION__, " 'apStatus' found with existing entries in ",
        WLAN_STATE_JSON, ". Clearing for dynamic population.");
    stateRootObj["IApInterfaceManager"]["apStatus"]
        .clear(); // Ensure it starts empty for dynamic filling
  }

  // Initialize connectedDevices as an empty array if not present or empty
  if (!stateRootObj["IApInterfaceManager"].isMember("connectedDevices") ||
      !stateRootObj["IApInterfaceManager"]["connectedDevices"].isArray() ||
      stateRootObj["IApInterfaceManager"]["connectedDevices"].empty()) {
    LOG(INFO, __FUNCTION__,
        " 'connectedDevices' not found, not an array, or empty in ",
        WLAN_STATE_JSON, ". Initializing as empty array.");
    stateRootObj["IApInterfaceManager"]["connectedDevices"] =
        Json::Value(Json::arrayValue); // Explicitly set to empty array
  }

  // Write back the updated/initialized JSON data
  JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
}

ApInterfaceManagerServerImpl::~ApInterfaceManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
}

grpc::Status ApInterfaceManagerServerImpl::InitService(
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

  int cbDelay = rootObj["IApInterfaceManager"]["IsSubsystemReadyDelay"].asInt();
  std::string cbStatus =
      rootObj["IApInterfaceManager"]["IsSubsystemReady"].asString();
  telux::common::ServiceStatus status =
      telux::common::CommonUtils::mapServiceStatus(cbStatus);
  LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

  response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
  response->set_delay(cbDelay);

  if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    std::vector<std::string> filters = {AP_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(shared_from_this(), filters);
  }

  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::SetConfig(
    ServerContext *context, const wlanStub::SetConfigRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  // Read API data for status and error codes
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager", "setConfig",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id teluxApId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(
            request->config().id());
    int configuredApCount =
        getConfiguredNumApFromDeviceManagerState(); // Get the configured
                                                    // sequential count

    // Check if the requested AP ID is configured
    if (static_cast<int>(teluxApId) > configuredApCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. AP ID ",
          static_cast<int>(teluxApId),
          " is not configured by configuredNumAp (", configuredApCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    // Convert gRPC ApConfig to Telux ApConfig
    telux::wlan::ApConfig teluxApConfig =
        telux::wlan::WlanCommonUtilsStub::convertApConfigFromGrpc(
            request->config());
    LOG(DEBUG, __FUNCTION__,
        " SetConfig for AP ID: ", static_cast<int>(teluxApId));

    if (teluxApConfig.network.empty()) {
      LOG(ERROR, __FUNCTION__,
          " Operation denied. Network configuration list is empty for AP ID ",
          static_cast<int>(teluxApId));
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }
    if (teluxApConfig.network.size() > 1) {
      LOG(WARNING, __FUNCTION__,
          " Multiple network configurations provided for AP ID ",
          static_cast<int>(teluxApId), ". Only the first one will be used.");
    }
    const telux::wlan::ApNetConfig &processedNetConfig =
        teluxApConfig.network[0]; // Always use the first network config

    // If this is 1st AP (PRIMARY) it should be private AP and has full access,
    // else return not allowed
    if ((teluxApConfig.id == telux::wlan::Id::PRIMARY) &&
        ((processedNetConfig.info.apType != telux::wlan::ApType::PRIVATE) ||
         (processedNetConfig.interworking !=
          telux::wlan::ApInterworking::FULL_ACCESS))) {
      LOG(ERROR, __FUNCTION__,
          "Operation denied: Primary AP must be PRIVATE type and have "
          "FULL_ACCESS interworking. AP ID: ",
          static_cast<int>(teluxApId));
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::OPERATION_NOT_ALLOWED));
      return grpc::Status::OK;
    }

    // if this not 1st AP and is set to private, return not allowed
    if ((teluxApConfig.id != telux::wlan::Id::PRIMARY) &&
        (processedNetConfig.info.apType == telux::wlan::ApType::PRIVATE)) {
      LOG(ERROR, __FUNCTION__,
          "Operation denied: Non-primary APs cannot be set to PRIVATE type. AP "
          "ID: ",
          static_cast<int>(teluxApId));
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::OPERATION_NOT_ALLOWED));
      return grpc::Status::OK;
    }

    Json::Value &configuredApConfig =
        stateRootObj["IApInterfaceManager"]["configuredApConfig"];
    bool updated = false;
    for (Json::ArrayIndex i = 0; i < configuredApConfig.size(); ++i) {
      // Find the AP config entry for the given ID
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredApConfig[i]["id"].asString())) == teluxApId) {
        // Update the existing AP config (venue fields)
        configuredApConfig[i]["venue"]["type"] = teluxApConfig.venue.type;
        configuredApConfig[i]["venue"]["group"] = teluxApConfig.venue.group;

        // Clear all existing network configs and replace with only the
        // processed (first) one
        configuredApConfig[i]["network"].clear();
        Json::Value netConfigNode;
        netConfigNode["info"]["apRadio"] =
            telux::wlan::server::WlanServerUtils::getBandTypeString(
                processedNetConfig.info.apRadio);
        netConfigNode["info"]["apType"] =
            telux::wlan::server::WlanServerUtils::getApTypeString(
                processedNetConfig.info.apType);
        netConfigNode["ssid"] = processedNetConfig.ssid;
        netConfigNode["isVisible"] = processedNetConfig.isVisible;
        netConfigNode["elementInfoConfig"]["isEnabled"] =
            processedNetConfig.elementInfoConfig.isEnabled;
        netConfigNode["elementInfoConfig"]["isInterworkingEnabled"] =
            processedNetConfig.elementInfoConfig.isInterworkingEnabled;
        netConfigNode["elementInfoConfig"]["netAccessType"] =
            telux::wlan::server::WlanServerUtils::getNetAccessTypeString(
                processedNetConfig.elementInfoConfig.netAccessType);
        netConfigNode["elementInfoConfig"]["internet"] =
            processedNetConfig.elementInfoConfig.internet;
        netConfigNode["elementInfoConfig"]["asra"] =
            processedNetConfig.elementInfoConfig.asra;
        netConfigNode["elementInfoConfig"]["esr"] =
            processedNetConfig.elementInfoConfig.esr;
        netConfigNode["elementInfoConfig"]["uesa"] =
            processedNetConfig.elementInfoConfig.uesa;
        netConfigNode["elementInfoConfig"]["venueGroup"] =
            processedNetConfig.elementInfoConfig.venueGroup;
        netConfigNode["elementInfoConfig"]["venueType"] =
            processedNetConfig.elementInfoConfig.venueType;
        netConfigNode["elementInfoConfig"]["hessid"] =
            processedNetConfig.elementInfoConfig.hessid;
        netConfigNode["elementInfoConfig"]["vendorElements"] =
            processedNetConfig.elementInfoConfig.vendorElements;
        netConfigNode["elementInfoConfig"]["assocRespElements"] =
            processedNetConfig.elementInfoConfig.assocRespElements;
        netConfigNode["interworking"] =
            telux::wlan::server::WlanServerUtils::getApInterworkingString(
                processedNetConfig.interworking);
        netConfigNode["apSecurity"]["mode"] =
            telux::wlan::server::WlanServerUtils::getSecModeString(
                processedNetConfig.apSecurity.mode);
        netConfigNode["apSecurity"]["auth"] =
            telux::wlan::server::WlanServerUtils::getSecAuthString(
                processedNetConfig.apSecurity.auth);
        netConfigNode["apSecurity"]["encrypt"] =
            telux::wlan::server::WlanServerUtils::getSecEncryptString(
                processedNetConfig.apSecurity.encrypt);
        netConfigNode["passPhrase"] = processedNetConfig.passPhrase;
        configuredApConfig[i]["network"].append(
            netConfigNode); // Append the single network config

        updated = true;
        break;
      }
    }

    if (!updated) {
      // If AP with matching ID not found, create a new config entry
      // This case should ideally not happen if configuredApCount validation
      // passes, unless configuredApConfig array somehow doesn't match
      // configuredNumAp. For now, allow it to create if validation passes.
      Json::Value newApConfig;
      newApConfig["id"] =
          telux::wlan::server::WlanServerUtils::getTeluxIdString(
              teluxApConfig.id);
      newApConfig["venue"]["type"] = teluxApConfig.venue.type;
      newApConfig["venue"]["group"] = teluxApConfig.venue.group;

      // Add only the processed (first) network config to the new AP entry
      Json::Value netConfigNode;
      netConfigNode["info"]["apRadio"] =
          telux::wlan::server::WlanServerUtils::getBandTypeString(
              processedNetConfig.info.apRadio);
      netConfigNode["info"]["apType"] =
          telux::wlan::server::WlanServerUtils::getApTypeString(
              processedNetConfig.info.apType);
      netConfigNode["ssid"] = processedNetConfig.ssid;
      netConfigNode["isVisible"] = processedNetConfig.isVisible;
      netConfigNode["elementInfoConfig"]["isEnabled"] =
          processedNetConfig.elementInfoConfig.isEnabled;
      netConfigNode["elementInfoConfig"]["isInterworkingEnabled"] =
          processedNetConfig.elementInfoConfig.isInterworkingEnabled;
      netConfigNode["elementInfoConfig"]["netAccessType"] =
          telux::wlan::server::WlanServerUtils::getNetAccessTypeString(
              processedNetConfig.elementInfoConfig.netAccessType);
      netConfigNode["elementInfoConfig"]["internet"] =
          processedNetConfig.elementInfoConfig.internet;
      netConfigNode["elementInfoConfig"]["asra"] =
          processedNetConfig.elementInfoConfig.asra;
      netConfigNode["elementInfoConfig"]["esr"] =
          processedNetConfig.elementInfoConfig.esr;
      netConfigNode["elementInfoConfig"]["uesa"] =
          processedNetConfig.elementInfoConfig.uesa;
      netConfigNode["elementInfoConfig"]["venueGroup"] =
          processedNetConfig.elementInfoConfig.venueGroup;
      netConfigNode["elementInfoConfig"]["venueType"] =
          processedNetConfig.elementInfoConfig.venueType;
      netConfigNode["elementInfoConfig"]["hessid"] =
          processedNetConfig.elementInfoConfig.hessid;
      netConfigNode["elementInfoConfig"]["vendorElements"] =
          processedNetConfig.elementInfoConfig.vendorElements;
      netConfigNode["elementInfoConfig"]["assocRespElements"] =
          processedNetConfig.elementInfoConfig.assocRespElements;
      netConfigNode["interworking"] =
          telux::wlan::server::WlanServerUtils::getApInterworkingString(
              processedNetConfig.interworking);
      netConfigNode["apSecurity"]["mode"] =
          telux::wlan::server::WlanServerUtils::getSecModeString(
              processedNetConfig.apSecurity.mode);
      netConfigNode["apSecurity"]["auth"] =
          telux::wlan::server::WlanServerUtils::getSecAuthString(
              processedNetConfig.apSecurity.auth);
      netConfigNode["apSecurity"]["encrypt"] =
          telux::wlan::server::WlanServerUtils::getSecEncryptString(
              processedNetConfig.apSecurity.encrypt);
      netConfigNode["passPhrase"] = processedNetConfig.passPhrase;
      newApConfig["network"].append(netConfigNode);

      configuredApConfig.append(newApConfig);
    }

    JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

    // Simulate publishing OnApConfigChanged event
    wlanStub::OnApConfigChanged configChangedEvent;
    configChangedEvent.set_ap_id(request->config().id());
    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter(AP_FILTER);
    anyResponse.mutable_any()->PackFrom(configChangedEvent);
    EventService::getInstance().updateEventQueue(anyResponse);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::SetSecurityConfig(
    ServerContext *context, const wlanStub::SetSecurityConfigRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager",
      "setSecurityConfig", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id teluxApId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->ap_id());
    int configuredApCount =
        getConfiguredNumApFromDeviceManagerState(); // Get the configured
                                                    // sequential count
    if (static_cast<int>(teluxApId) > configuredApCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. AP ID ",
          static_cast<int>(teluxApId),
          " is not configured by configuredNumAp (", configuredApCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    telux::wlan::ApSecurity teluxApSecurity =
        telux::wlan::WlanCommonUtilsStub::convertApSecurityFromGrpc(
            request->ap_security());
    LOG(DEBUG, __FUNCTION__,
        " SetSecurityConfig for AP ID: ", static_cast<int>(teluxApId));

    Json::Value &configuredApConfig =
        stateRootObj["IApInterfaceManager"]["configuredApConfig"];
    bool updated = false;
    for (Json::ArrayIndex i = 0; i < configuredApConfig.size(); ++i) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredApConfig[i]["id"].asString())) == teluxApId) {
        if (configuredApConfig[i].isMember("network") &&
            configuredApConfig[i]["network"].isArray() &&
            !configuredApConfig[i]["network"].empty()) {
          // Update only the first network entry's security for this AP
          Json::Value &firstNetwork = configuredApConfig[i]["network"][0];
          firstNetwork["apSecurity"]["mode"] =
              telux::wlan::server::WlanServerUtils::getSecModeString(
                  teluxApSecurity.mode);
          firstNetwork["apSecurity"]["auth"] =
              telux::wlan::server::WlanServerUtils::getSecAuthString(
                  teluxApSecurity.auth);
          firstNetwork["apSecurity"]["encrypt"] =
              telux::wlan::server::WlanServerUtils::getSecEncryptString(
                  teluxApSecurity.encrypt);
          updated = true;
        }
        break;
      }
    }
    if (updated) {
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      // Trigger OnApConfigChanged event
      wlanStub::OnApConfigChanged configChangedEvent;
      configChangedEvent.set_ap_id(
          request->ap_id()); // Use the AP ID from the request
      ::eventService::EventResponse anyResponse;
      anyResponse.set_filter(AP_FILTER);
      anyResponse.mutable_any()->PackFrom(configChangedEvent);
      EventService::getInstance().updateEventQueue(anyResponse);
    } else {
      LOG(WARNING, __FUNCTION__, " AP ID ", static_cast<int>(teluxApId),
          " or its network config not found for security update.");
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status
ApInterfaceManagerServerImpl::SetSsid(ServerContext *context,
                                      const wlanStub::SetSsidRequest *request,
                                      wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager", "setSsid",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id teluxApId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->ap_id());
    int configuredApCount =
        getConfiguredNumApFromDeviceManagerState(); // Get the configured
                                                    // sequential count
    if (static_cast<int>(teluxApId) > configuredApCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. AP ID ",
          static_cast<int>(teluxApId),
          " is not configured by configuredNumAp (", configuredApCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    LOG(DEBUG, __FUNCTION__,
        " SetSsid for AP ID: ", static_cast<int>(teluxApId),
        ", SSID: ", request->ssid());

    Json::Value &configuredApConfig =
        stateRootObj["IApInterfaceManager"]["configuredApConfig"];
    bool updated = false;
    for (Json::ArrayIndex i = 0; i < configuredApConfig.size(); ++i) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredApConfig[i]["id"].asString())) == teluxApId) {
        if (configuredApConfig[i].isMember("network") &&
            configuredApConfig[i]["network"].isArray() &&
            !configuredApConfig[i]["network"].empty()) {
          Json::Value &firstNetwork = configuredApConfig[i]["network"][0];
          firstNetwork["ssid"] = request->ssid();
          updated = true;
        }
        break;
      }
    }
    if (updated) {
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      // Trigger OnApConfigChanged event
      wlanStub::OnApConfigChanged configChangedEvent;
      configChangedEvent.set_ap_id(
          request->ap_id()); // Use the AP ID from the request
      ::eventService::EventResponse anyResponse;
      anyResponse.set_filter(AP_FILTER);
      anyResponse.mutable_any()->PackFrom(configChangedEvent);
      EventService::getInstance().updateEventQueue(anyResponse);
    } else {
      LOG(WARNING, __FUNCTION__, " AP ID ", static_cast<int>(teluxApId),
          " or its network config not found for SSID update.");
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
    response->set_status(
        static_cast<commonStub::Status>(telux::common::Status::FAILED));
  }
  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::SetVisibility(
    ServerContext *context, const wlanStub::SetVisibilityRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager",
      "setVisibility", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id teluxApId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->ap_id());
    int configuredApCount =
        getConfiguredNumApFromDeviceManagerState(); // Get the configured
                                                    // sequential count
    if (static_cast<int>(teluxApId) > configuredApCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. AP ID ",
          static_cast<int>(teluxApId),
          " is not configured by configuredNumAp (", configuredApCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    LOG(DEBUG, __FUNCTION__,
        " SetVisibility for AP ID: ", static_cast<int>(teluxApId),
        ", IsVisible: ", request->is_visible());

    Json::Value &configuredApConfig =
        stateRootObj["IApInterfaceManager"]["configuredApConfig"];
    bool updated = false;
    for (Json::ArrayIndex i = 0; i < configuredApConfig.size(); ++i) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredApConfig[i]["id"].asString())) == teluxApId) {
        if (configuredApConfig[i].isMember("network") &&
            configuredApConfig[i]["network"].isArray() &&
            !configuredApConfig[i]["network"].empty()) {
          Json::Value &firstNetwork = configuredApConfig[i]["network"][0];
          firstNetwork["isVisible"] = request->is_visible();
          updated = true;
        }
        break;
      }
    }
    if (updated) {
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      // Trigger OnApConfigChanged event
      wlanStub::OnApConfigChanged configChangedEvent;
      configChangedEvent.set_ap_id(
          request->ap_id()); // Use the AP ID from the request
      ::eventService::EventResponse anyResponse;
      anyResponse.set_filter(AP_FILTER);
      anyResponse.mutable_any()->PackFrom(configChangedEvent);
      EventService::getInstance().updateEventQueue(anyResponse);
    } else {
      LOG(WARNING, __FUNCTION__, " AP ID ", static_cast<int>(teluxApId),
          " or its network config not found for visibility update.");
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::SetElementInfoConfig(
    ServerContext *context,
    const wlanStub::SetElementInfoConfigRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager",
      "setElementInfoConfig", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id teluxApId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->ap_id());
    int configuredApCount =
        getConfiguredNumApFromDeviceManagerState(); // Get the configured
                                                    // sequential count
    if (static_cast<int>(teluxApId) > configuredApCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. AP ID ",
          static_cast<int>(teluxApId),
          " is not configured by configuredNumAp (", configuredApCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    telux::wlan::ApElementInfoConfig teluxConfig =
        telux::wlan::WlanCommonUtilsStub::convertApElementInfoConfigFromGrpc(
            request->config());
    LOG(DEBUG, __FUNCTION__,
        " SetElementInfoConfig for AP ID: ", static_cast<int>(teluxApId));

    Json::Value &configuredApConfig =
        stateRootObj["IApInterfaceManager"]["configuredApConfig"];
    bool updated = false;
    for (Json::ArrayIndex i = 0; i < configuredApConfig.size(); ++i) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredApConfig[i]["id"].asString())) == teluxApId) {
        if (configuredApConfig[i].isMember("network") &&
            configuredApConfig[i]["network"].isArray() &&
            !configuredApConfig[i]["network"].empty()) {
          Json::Value &firstNetwork = configuredApConfig[i]["network"][0];
          Json::Value &elementInfoNode = firstNetwork["elementInfoConfig"];
          elementInfoNode["isEnabled"] = teluxConfig.isEnabled;
          elementInfoNode["isInterworkingEnabled"] =
              teluxConfig.isInterworkingEnabled;
          elementInfoNode["netAccessType"] =
              telux::wlan::server::WlanServerUtils::getNetAccessTypeString(
                  teluxConfig.netAccessType);
          elementInfoNode["internet"] = teluxConfig.internet;
          elementInfoNode["asra"] = teluxConfig.asra;
          elementInfoNode["esr"] = teluxConfig.esr;
          elementInfoNode["uesa"] = teluxConfig.uesa;
          elementInfoNode["venueGroup"] = teluxConfig.venueGroup;
          elementInfoNode["venueType"] = teluxConfig.venueType;
          elementInfoNode["hessid"] = teluxConfig.hessid;
          elementInfoNode["vendorElements"] = teluxConfig.vendorElements;
          elementInfoNode["assocRespElements"] = teluxConfig.assocRespElements;
          updated = true;
        }
        break;
      }
    }
    if (updated) {
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      // Trigger OnApConfigChanged event
      wlanStub::OnApConfigChanged configChangedEvent;
      configChangedEvent.set_ap_id(
          request->ap_id()); // Use the AP ID from the request
      ::eventService::EventResponse anyResponse;
      anyResponse.set_filter(AP_FILTER);
      anyResponse.mutable_any()->PackFrom(configChangedEvent);
      EventService::getInstance().updateEventQueue(anyResponse);
    } else {
      LOG(WARNING, __FUNCTION__, " AP ID ", static_cast<int>(teluxApId),
          " or its network config not found for element info update.");
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::SetPassPhrase(
    ServerContext *context, const wlanStub::SetPassPhraseRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager",
      "setPassPhrase", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    telux::wlan::Id teluxApId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->ap_id());
    int configuredApCount =
        getConfiguredNumApFromDeviceManagerState(); // Get the configured
                                                    // sequential count
    if (static_cast<int>(teluxApId) > configuredApCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. AP ID ",
          static_cast<int>(teluxApId),
          " is not configured by configuredNumAp (", configuredApCount, ").");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    LOG(DEBUG, __FUNCTION__,
        " SetPassPhrase for AP ID: ", static_cast<int>(teluxApId));

    Json::Value &configuredApConfig =
        stateRootObj["IApInterfaceManager"]["configuredApConfig"];
    bool updated = false;
    for (Json::ArrayIndex i = 0; i < configuredApConfig.size(); ++i) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredApConfig[i]["id"].asString())) == teluxApId) {
        if (configuredApConfig[i].isMember("network") &&
            configuredApConfig[i]["network"].isArray() &&
            !configuredApConfig[i]["network"].empty()) {
          Json::Value &firstNetwork = configuredApConfig[i]["network"][0];
          firstNetwork["passPhrase"] = request->pass_phrase();
          updated = true;
        }
        break;
      }
    }
    if (updated) {
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
      // Trigger OnApConfigChanged event
      wlanStub::OnApConfigChanged configChangedEvent;
      configChangedEvent.set_ap_id(
          request->ap_id()); // Use the AP ID from the request
      ::eventService::EventResponse anyResponse;
      anyResponse.set_filter(AP_FILTER);
      anyResponse.mutable_any()->PackFrom(configChangedEvent);
      EventService::getInstance().updateEventQueue(anyResponse);
    } else {
      LOG(WARNING, __FUNCTION__, " AP ID ", static_cast<int>(teluxApId),
          " or its network config not found for pass phrase update.");
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::GetConfig(
    ServerContext *context, const ::google::protobuf::Empty *request,
    wlanStub::ApGetConfigResponse *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager", "getConfig",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    // Read 'configuredApConfig' from IApInterfaceManagerState.json
    const Json::Value &configuredApConfigArray =
        stateRootObj["IApInterfaceManager"]["configuredApConfig"];
    if (configuredApConfigArray.isArray()) {
      int configuredApCount =
          getConfiguredNumApFromDeviceManagerState(); // Get the configured
                                                      // sequential count
      for (const auto &apConfigNode : configuredApConfigArray) {
        telux::wlan::Id teluxApId = static_cast<telux::wlan::Id>(
            telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                apConfigNode["id"].asString()));

        // Filter based on configuredApCount: Only return configured APs up to
        // the allowed sequential count
        if (static_cast<int>(teluxApId) > configuredApCount) {
          LOG(INFO, __FUNCTION__, "Skipping configured AP ID ",
              static_cast<int>(teluxApId),
              " as it's not enabled by configuredNumAp (", configuredApCount,
              ")");
          continue; // Skip this AP if it's configured but beyond the limit
        }

        telux::wlan::ApConfig teluxApConfig;
        teluxApConfig.id = teluxApId;
        teluxApConfig.venue.type = apConfigNode["venue"]["type"].asInt();
        teluxApConfig.venue.group = apConfigNode["venue"]["group"].asInt();

        if (apConfigNode.isMember("network") &&
            apConfigNode["network"].isArray()) {
          for (const auto &netConfigNode : apConfigNode["network"]) {
            telux::wlan::ApNetConfig netConfig;
            netConfig.info.apRadio = static_cast<telux::wlan::BandType>(
                telux::wlan::server::WlanServerUtils::getBandTypeFromString(
                    netConfigNode["info"]["apRadio"].asString()));
            netConfig.info.apType = static_cast<telux::wlan::ApType>(
                telux::wlan::server::WlanServerUtils::getApTypeFromString(
                    netConfigNode["info"]["apType"].asString()));
            netConfig.ssid = netConfigNode["ssid"].asString();
            netConfig.isVisible = netConfigNode["isVisible"].asBool();
            netConfig.elementInfoConfig.isEnabled =
                netConfigNode["elementInfoConfig"]["isEnabled"].asBool();
            netConfig.elementInfoConfig.isInterworkingEnabled =
                netConfigNode["elementInfoConfig"]["isInterworkingEnabled"]
                    .asBool();
            netConfig.elementInfoConfig.netAccessType =
                static_cast<telux::wlan::NetAccessType>(
                    telux::wlan::server::WlanServerUtils::
                        getNetAccessTypeFromString(
                            netConfigNode["elementInfoConfig"]["netAccessType"]
                                .asString()));
            netConfig.elementInfoConfig.internet =
                netConfigNode["elementInfoConfig"]["internet"].asBool();
            netConfig.elementInfoConfig.asra =
                netConfigNode["elementInfoConfig"]["asra"].asBool();
            netConfig.elementInfoConfig.esr =
                netConfigNode["elementInfoConfig"]["esr"].asBool();
            netConfig.elementInfoConfig.uesa =
                netConfigNode["elementInfoConfig"]["uesa"].asBool();
            netConfig.elementInfoConfig.venueGroup = static_cast<uint8_t>(
                netConfigNode["elementInfoConfig"]["venueGroup"].asUInt());
            netConfig.elementInfoConfig.venueType = static_cast<uint8_t>(
                netConfigNode["elementInfoConfig"]["venueType"].asUInt());
            netConfig.elementInfoConfig.hessid =
                netConfigNode["elementInfoConfig"]["hessid"].asString();
            netConfig.elementInfoConfig.vendorElements =
                netConfigNode["elementInfoConfig"]["vendorElements"].asString();
            netConfig.elementInfoConfig.assocRespElements =
                netConfigNode["elementInfoConfig"]["assocRespElements"]
                    .asString();
            netConfig.interworking = static_cast<telux::wlan::ApInterworking>(
                telux::wlan::server::WlanServerUtils::
                    getApInterworkingFromString(
                        netConfigNode["interworking"].asString()));
            netConfig.apSecurity.mode = static_cast<telux::wlan::SecMode>(
                telux::wlan::server::WlanServerUtils::getSecModeFromString(
                    netConfigNode["apSecurity"]["mode"].asString()));
            netConfig.apSecurity.auth = static_cast<telux::wlan::SecAuth>(
                telux::wlan::server::WlanServerUtils::getSecAuthFromString(
                    netConfigNode["apSecurity"]["auth"].asString()));
            netConfig.apSecurity.encrypt = static_cast<telux::wlan::SecEncrypt>(
                telux::wlan::server::WlanServerUtils::getSecEncryptFromString(
                    netConfigNode["apSecurity"]["encrypt"].asString()));
            netConfig.passPhrase = netConfigNode["passPhrase"].asString();
            teluxApConfig.network.push_back(netConfig);
          }
        }
        *response->add_config() =
            telux::wlan::WlanCommonUtilsStub::convertApConfigToGrpc(
                teluxApConfig);
      }
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'configuredApConfig' not found or not an array in state JSON.");
    }

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::GetStatus(
    ServerContext *context, const ::google::protobuf::Empty *request,
    wlanStub::ApGetStatusResponse *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager", "getStatus",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__,
          "Operation denied. WLAN Device Manager is not enabled.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    int activeNumApBitmask =
        getRunningNumApBitmaskFromDeviceManagerState(); // Get the active
                                                        // bitmask
    if (activeNumApBitmask ==
        0) { // If no APs are active according to the bitmask
      LOG(INFO, __FUNCTION__,
          "No AP interfaces are active (numAp bitmask is 0). No status to "
          "return.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              data.error)); // Still return success, but with an empty list
      return grpc::Status::OK;
    }

    // Read 'apStatus' array from IApInterfaceManagerState.json
    const Json::Value &apStatusArray =
        stateRootObj["IApInterfaceManager"]["apStatus"];
    if (apStatusArray.isArray()) {
      for (const auto &apStatusNode : apStatusArray) {
        telux::wlan::Id teluxApId =
            telux::wlan::server::WlanServerUtils::convertIdFromGrpc(
                telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                    apStatusNode["id"].asString()));

        // Filter based on activeNumApBitmask: Check if the bit corresponding to
        // teluxApId is set. (static_cast<int>(teluxApId) - 1) gives the
        // 0-indexed bit position.
        if ((activeNumApBitmask & (1 << (static_cast<int>(teluxApId) - 1))) ==
            0) {
          LOG(INFO, __FUNCTION__, "Skipping AP ID ",
              static_cast<int>(teluxApId),
              " in status as it's not enabled by active numAp bitmask (",
              activeNumApBitmask, ")");
          continue; // Skip this AP if its bit is not set
        }

        telux::wlan::ApStatus teluxApStatus;
        teluxApStatus.id = teluxApId;
        teluxApStatus.name = apStatusNode["name"].asString();
        teluxApStatus.ipv4Address = apStatusNode["ipv4Address"].asString();
        teluxApStatus.macAddress = apStatusNode["macAddress"].asString();

        if (apStatusNode.isMember("network") &&
            apStatusNode["network"].isArray()) {
          for (const auto &netInfoNode : apStatusNode["network"]) {
            telux::wlan::ApNetInfo netInfo;
            netInfo.info.apRadio = static_cast<telux::wlan::BandType>(
                telux::wlan::server::WlanServerUtils::getBandTypeFromString(
                    netInfoNode["info"]["apRadio"].asString()));
            netInfo.info.apType = static_cast<telux::wlan::ApType>(
                telux::wlan::server::WlanServerUtils::getApTypeFromString(
                    netInfoNode["info"]["apType"].asString()));
            netInfo.ssid = netInfoNode["ssid"].asString();
            teluxApStatus.network.push_back(netInfo);
          }
        }
        *response->add_status() =
            telux::wlan::WlanCommonUtilsStub::convertApStatusToGrpc(
                teluxApStatus);
      }
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'apStatus' not found or not an array in state JSON.");
    }

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::GetConnectedDevices(
    ServerContext *context, const ::google::protobuf::Empty *request,
    wlanStub::GetConnectedDevicesResponse *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager",
      "getConnectedDevices", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__,
          "Operation denied. WLAN Device Manager is not enabled.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    // Get the active allowed number of APs (bitmask) from the Device Manager
    // state
    int activeNumApBitmask = getRunningNumApBitmaskFromDeviceManagerState();

    // If no APs are active according to the bitmask
    if (activeNumApBitmask == 0) {
      LOG(INFO, __FUNCTION__,
          "No AP interfaces are active (numAp bitmask is 0). No connected "
          "devices to return.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              data.error)); // Still return success, but with an empty list
      return grpc::Status::OK;
    }

    // Read 'connectedDevices' array from IApInterfaceManagerState.json
    const Json::Value &connectedDevicesArray =
        stateRootObj["IApInterfaceManager"]["connectedDevices"];
    if (connectedDevicesArray.isArray()) {
      for (const auto &deviceNode : connectedDevicesArray) {
        telux::wlan::Id connectedDeviceId = static_cast<telux::wlan::Id>(
            telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                deviceNode["id"].asString()));

        // Check if the connected device's AP ID is enabled by the active
        // bitmask (static_cast<int>(connectedDeviceId) - 1) gives the 0-indexed
        // bit position.
        if ((activeNumApBitmask &
             (1 << (static_cast<int>(connectedDeviceId) - 1))) == 0) {
          LOG(WARNING, __FUNCTION__, "Skipping connected device for AP ID ",
              static_cast<int>(connectedDeviceId),
              " as its AP is not enabled by active numAp bitmask (",
              activeNumApBitmask, ")");
          continue; // Skip this device as its AP ID is not currently allowed
        }

        telux::wlan::DeviceInfo teluxDeviceInfo;
        teluxDeviceInfo.id = connectedDeviceId;
        teluxDeviceInfo.name = deviceNode["name"].asString();
        teluxDeviceInfo.ipv4Address = deviceNode["ipv4Address"].asString();
        if (deviceNode.isMember("ipv6Address") &&
            deviceNode["ipv6Address"].isArray()) {
          for (const auto &ipv6 : deviceNode["ipv6Address"]) {
            teluxDeviceInfo.ipv6Address.push_back(ipv6.asString());
          }
        }
        teluxDeviceInfo.macAddress = deviceNode["macAddress"].asString();
        *response->add_clients_info() =
            telux::wlan::WlanCommonUtilsStub::convertDeviceInfoToGrpc(
                teluxDeviceInfo);
      }
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'connectedDevices' not found or not an array in state JSON.");
    }

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status ApInterfaceManagerServerImpl::ManageApService(
    ServerContext *context, const wlanStub::ManageApServiceRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IApInterfaceManager",
      "manageApService", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled()) {
      LOG(ERROR, __FUNCTION__,
          "Operation denied. WLAN Device Manager is not enabled.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    telux::wlan::Id teluxApId =
        telux::wlan::WlanCommonUtilsStub::convertIdFromGrpc(request->ap_id());
    telux::wlan::ServiceOperation teluxOperation =
        telux::wlan::WlanCommonUtilsStub::convertServiceOperationFromGrpc(
            request->opr());
    LOG(DEBUG, __FUNCTION__,
        " ManageApService for AP ID: ", static_cast<int>(teluxApId),
        ", Operation: ", static_cast<int>(teluxOperation));

    Json::Value &apStatusArray =
        stateRootObj["IApInterfaceManager"]["apStatus"];
    Json::Value &configuredApConfigArray =
        stateRootObj["IApInterfaceManager"]["configuredApConfig"];
    bool changed = false;

    // Check if the requested AP ID is configured (sequential check)
    int configuredApCount = getConfiguredNumApFromDeviceManagerState();
    if (static_cast<int>(teluxApId) > configuredApCount) {
      LOG(ERROR, __FUNCTION__, "Operation denied. AP ID ",
          static_cast<int>(teluxApId),
          " is not configured (configuredNumAp is ", configuredApCount,
          "). Cannot manage service.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    // Find the corresponding configured AP to get its details
    Json::Value *matchedConfiguredAp = nullptr;
    for (Json::ArrayIndex j = 0; j < configuredApConfigArray.size(); ++j) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  configuredApConfigArray[j]["id"].asString())) == teluxApId) {
        matchedConfiguredAp = &configuredApConfigArray[j];
        break;
      }
    }

    if (!matchedConfiguredAp) {
      LOG(ERROR, __FUNCTION__, "Configured AP config not found for ID: ",
          static_cast<int>(teluxApId),
          ". This should not happen if configuredNumAp is correct.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INTERNAL_ERROR));
      return grpc::Status::OK;
    }

    // Check if AP is currently in apStatus (running)
    bool isCurrentlyRunning = false;
    for (Json::ArrayIndex i = 0; i < apStatusArray.size(); ++i) {
      if (static_cast<telux::wlan::Id>(
              telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                  apStatusArray[i]["id"].asString())) == teluxApId) {
        isCurrentlyRunning = true;
        break;
      }
    }

    if (teluxOperation == telux::wlan::ServiceOperation::START ||
        teluxOperation == telux::wlan::ServiceOperation::RESTART) {
      if (isCurrentlyRunning &&
          teluxOperation == telux::wlan::ServiceOperation::START) {
        LOG(INFO, __FUNCTION__, "AP ID ", static_cast<int>(teluxApId),
            " is already running. Returning NO_EFFECT for START operation.");
        response->set_error(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::NO_EFFECT));
        return grpc::Status::OK;
      }

      // For RESTART or START (if not already running), we ensure it's removed
      // and then added.
      if (isCurrentlyRunning) { // If RESTART and it's running, or if START but
                                // it was running (should be NO_EFFECT above,
                                // but for robustness)
        LOG(INFO, __FUNCTION__, "AP ID ", static_cast<int>(teluxApId),
            " is currently running. Simulating STOP before START/RESTART.");
        Json::Value newApStatusArray(Json::arrayValue);
        for (Json::ArrayIndex i = 0; i < apStatusArray.size(); ++i) {
          if (static_cast<telux::wlan::Id>(
                  telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                      apStatusArray[i]["id"].asString())) != teluxApId) {
            newApStatusArray.append(apStatusArray[i]);
          }
        }
        apStatusArray = newApStatusArray;
        updateRunningNumApBitmaskInDeviceManagerState(
            teluxApId, telux::wlan::ServiceOperation::STOP);
        changed = true; // Mark as changed because apStatusArray was modified
      }

      // Now, simulate START
      LOG(INFO, __FUNCTION__, "AP ID ", static_cast<int>(teluxApId),
          " simulated START for operation: ",
          (teluxOperation == telux::wlan::ServiceOperation::START ? "START"
                                                                  : "RESTART"));
      Json::Value newApStatus;
      newApStatus["id"] =
          telux::wlan::server::WlanServerUtils::getTeluxIdString(teluxApId);

      // Get actual interface name and details
      std::vector<std::string> apInterfaceNames =
          telux::wlan::server::WlanServerUtils::getInterfaceNames(
              "sim.wlan.ap_interfaces");
      std::string ifaceName = "";
      if (static_cast<size_t>(static_cast<int>(teluxApId) - 1) <
          apInterfaceNames.size()) {
        ifaceName = apInterfaceNames[static_cast<int>(teluxApId) - 1];
        newApStatus["name"] = ifaceName;
        newApStatus["ipv4Address"] =
            telux::wlan::server::WlanServerUtils::getIpv4Address(ifaceName);
        newApStatus["macAddress"] =
            telux::wlan::server::WlanServerUtils::getMacAddress(ifaceName);
        newApStatus["ipv6Address"] =
            telux::wlan::server::WlanServerUtils::getIpv6Address(
                ifaceName); // Fetch actual IPv6
      } else {
        // Fallback to generated names/addresses if not in config
        ifaceName = "wlan_ap" + std::to_string(static_cast<int>(teluxApId));
        newApStatus["name"] = ifaceName; // Placeholder name
        newApStatus["ipv4Address"] =
            "192.168.0." +
            std::to_string(static_cast<int>(teluxApId)); // Placeholder IP
        newApStatus["macAddress"] =
            "00:1A:2B:3C:4D:" +
            ((static_cast<int>(teluxApId) < 10)
                 ? ("0" + std::to_string(static_cast<int>(teluxApId)))
                 : std::to_string(
                       static_cast<int>(teluxApId))); // Placeholder MAC
        newApStatus["ipv6Address"] = ""; // No dynamic IPv6 for fallback
      }

      // Copy network details from configuredApConfig
      if (matchedConfiguredAp->isMember("network") &&
          (*matchedConfiguredAp)["network"].isArray()) {
        for (const auto &netConfig : (*matchedConfiguredAp)["network"]) {
          Json::Value netInfoNode;
          netInfoNode["info"]["apRadio"] = netConfig["info"]["apRadio"];
          netInfoNode["info"]["apType"] = netConfig["info"]["apType"];
          netInfoNode["ssid"] = netConfig["ssid"];
          newApStatus["network"].append(netInfoNode);
        }
      }
      apStatusArray.append(newApStatus);
      changed = true;
      updateRunningNumApBitmaskInDeviceManagerState(
          teluxApId, telux::wlan::ServiceOperation::START);

    } else if (teluxOperation == telux::wlan::ServiceOperation::STOP) {
      LOG(INFO, __FUNCTION__, "AP ID ", static_cast<int>(teluxApId),
          " simulated STOP.");
      Json::Value newApStatusArray(Json::arrayValue);
      bool foundAndRemoved = false;
      for (Json::ArrayIndex i = 0; i < apStatusArray.size(); ++i) {
        if (static_cast<telux::wlan::Id>(
                telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                    apStatusArray[i]["id"].asString())) == teluxApId) {
          foundAndRemoved = true;
        } else {
          newApStatusArray.append(apStatusArray[i]);
        }
      }
      if (foundAndRemoved) {
        apStatusArray = newApStatusArray;
        changed = true;
        updateRunningNumApBitmaskInDeviceManagerState(
            teluxApId, telux::wlan::ServiceOperation::STOP);
      } else {
        LOG(INFO, __FUNCTION__, "AP ID ", static_cast<int>(teluxApId),
            " is not running. Returning NO_EFFECT.");
        response->set_error(static_cast<commonStub::ErrorCode>(
            telux::common::ErrorCode::NO_EFFECT));
        return grpc::Status::OK;
      }
    }

    if (changed) {
      JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

void ApInterfaceManagerServerImpl::onEventUpdate(
    ::eventService::UnsolicitedEvent message) {
  if (message.filter() == AP_FILTER) {
    onEventUpdate(message.event());
  }
}

void ApInterfaceManagerServerImpl::onEventUpdate(std::string event) {
  std::string token =
      telux::common::EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
  LOG(DEBUG, __FUNCTION__, "Received event token: ", token);
  if (token == "onApDeviceStatusChanged") {
    handleDeviceStatusChangedEvent(event);
  } else if (token == "onApBandChanged") {
    handleApBandChangedEvent(event);
  } else if (token == "onApConfigChanged") {
    handleApConfigChangedEvent(event);
  } else {
    LOG(ERROR, __FUNCTION__, "Unhandled event flag: ", token);
  }
}

void ApInterfaceManagerServerImpl::handleDeviceStatusChangedEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);

  std::string apIdStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  std::string eventTypeStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  std::string nameStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  std::string ipv4AddressStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  std::string macAddressStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  std::string ipv6AddressesStr = telux::common::EventParserUtil::getNextToken(
      eventStr, DEFAULT_DELIMITER); // Comma-separated list

  try {
    telux::wlan::Id apId = static_cast<telux::wlan::Id>(std::stoi(apIdStr));

    // Before processing, check if this AP ID is currently active (part of the
    // bitmask)
    int activeNumApBitmask = getRunningNumApBitmaskFromDeviceManagerState();
    // (static_cast<int>(apId) - 1) gives the 0-indexed bit position.
    if ((activeNumApBitmask & (1 << (static_cast<int>(apId) - 1))) == 0) {
      LOG(INFO, __FUNCTION__, "Ignoring deviceStatusChanged event for AP ID ",
          static_cast<int>(apId),
          " as it's not currently active by numAp bitmask.");
      return; // Don't process event for inactive APs
    }

    telux::wlan::ApDeviceConnectionEvent event =
        static_cast<telux::wlan::ApDeviceConnectionEvent>(
            std::stoi(eventTypeStr));

    telux::wlan::DeviceInfo fullDeviceInfo;
    fullDeviceInfo.id = apId;
    fullDeviceInfo.name = nameStr;
    fullDeviceInfo.ipv4Address = ipv4AddressStr;
    fullDeviceInfo.macAddress = macAddressStr;
    // Parse comma-separated IPv6 addresses
    std::istringstream iss(ipv6AddressesStr);
    std::string ipv6;
    while (std::getline(iss, ipv6, ',')) {
      fullDeviceInfo.ipv6Address.push_back(ipv6);
    }

    // Create DeviceIndInfo from fullDeviceInfo for the gRPC event
    telux::wlan::DeviceIndInfo deviceIndInfo;
    deviceIndInfo.id = fullDeviceInfo.id;
    deviceIndInfo.macAddress = fullDeviceInfo.macAddress;

    std::vector<telux::wlan::DeviceIndInfo>
        indInfoVector; // This vector now holds DeviceIndInfo
    indInfoVector.push_back(deviceIndInfo);

    // If the event is a connection, add to connectedDevices. If disconnection,
    // remove.
    Json::Value stateRootObj_ap; // Renamed to avoid conflict with outer scope
    JsonParser::readFromJsonFile(stateRootObj_ap, WLAN_STATE_JSON);
    Json::Value &connectedDevicesArray =
        stateRootObj_ap["IApInterfaceManager"]["connectedDevices"];

    if (event == telux::wlan::ApDeviceConnectionEvent::CONNECTED) {
      Json::Value newConnectedDevice;
      newConnectedDevice["id"] =
          telux::wlan::server::WlanServerUtils::getTeluxIdString(
              fullDeviceInfo.id);
      newConnectedDevice["name"] = fullDeviceInfo.name;
      newConnectedDevice["ipv4Address"] = fullDeviceInfo.ipv4Address;
      newConnectedDevice["macAddress"] = fullDeviceInfo.macAddress;
      for (const auto &addr : fullDeviceInfo.ipv6Address) {
        newConnectedDevice["ipv6Address"].append(addr);
      }

      // Check if already exists to avoid duplicates based on MAC address
      bool found = false;
      for (Json::ArrayIndex i = 0; i < connectedDevicesArray.size(); ++i) {
        if (connectedDevicesArray[i]["macAddress"].asString() ==
            fullDeviceInfo.macAddress) {
          // Update existing entry if found (e.g., IPv4/IPv6 address might
          // change)
          connectedDevicesArray[i] =
              newConnectedDevice; // Replace with new data
          found = true;
          LOG(DEBUG, __FUNCTION__, " Updated existing device ",
              fullDeviceInfo.macAddress, " in connectedDevices.");
          break;
        }
      }
      if (!found) {
        connectedDevicesArray.append(newConnectedDevice);
        LOG(DEBUG, __FUNCTION__, " Added new device ",
            fullDeviceInfo.macAddress, " to connectedDevices.");
      }
      JsonParser::writeToJsonFile(stateRootObj_ap, WLAN_STATE_JSON);
    } else if (event == telux::wlan::ApDeviceConnectionEvent::DISCONNECTED) {
      Json::Value newConnectedDevicesArray(Json::arrayValue);
      bool removed = false;
      for (Json::ArrayIndex i = 0; i < connectedDevicesArray.size(); ++i) {
        if (connectedDevicesArray[i]["macAddress"].asString() !=
            fullDeviceInfo.macAddress) {
          newConnectedDevicesArray.append(connectedDevicesArray[i]);
        } else {
          removed = true;
        }
      }
      if (removed) {
        connectedDevicesArray = newConnectedDevicesArray;
        JsonParser::writeToJsonFile(stateRootObj_ap, WLAN_STATE_JSON);
        LOG(DEBUG, __FUNCTION__, " Removed device ", fullDeviceInfo.macAddress,
            " from connectedDevices.");
      } else {
        LOG(WARNING, __FUNCTION__, " Disconnect event for device ",
            fullDeviceInfo.macAddress, " but not found in connectedDevices.");
      }
    }
    // No persistence needed for IPV4_UPDATED or IPV6_UPDATED if
    // AP_DEVICE_CONNECTED handles the update. If they are meant to be
    // standalone events, additional logic would be needed.

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ::eventService::EventResponse anyResponse;
    wlanStub::OnApDeviceStatusChanged grpcEvent;
    grpcEvent.set_event(static_cast<wlanStub::ApDeviceConnectionEvent>(event));
    for (const auto &teluxInfo :
         indInfoVector) { // Now indInfoVector holds DeviceIndInfo
      *grpcEvent.add_info() =
          telux::wlan::WlanCommonUtilsStub::convertDeviceIndInfoToGrpc(
              teluxInfo);
    }

    anyResponse.set_filter(AP_FILTER);
    anyResponse.mutable_any()->PackFrom(grpcEvent);
    EventService::getInstance().updateEventQueue(anyResponse);
    LOG(DEBUG, __FUNCTION__,
        " Posted OnApDeviceStatusChanged event for AP ID: ",
        static_cast<int>(apId));

  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing deviceStatusChanged event: ",
        ex.what());
  }
}

void ApInterfaceManagerServerImpl::handleApBandChangedEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);
  std::string bandTypeStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  try {
    // The getBandTypeFromString now returns wlanStub::BandType, so cast it back
    // to telux::wlan::BandType
    telux::wlan::BandType band = static_cast<telux::wlan::BandType>(
        telux::wlan::server::WlanServerUtils::getBandTypeFromString(
            bandTypeStr));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ::eventService::EventResponse anyResponse;
    wlanStub::OnApBandChanged grpcEvent;
    grpcEvent.set_radio(
        telux::wlan::WlanCommonUtilsStub::convertBandTypeToGrpc(band));
    anyResponse.set_filter(AP_FILTER);
    anyResponse.mutable_any()->PackFrom(grpcEvent);
    EventService::getInstance().updateEventQueue(anyResponse);
    LOG(DEBUG, __FUNCTION__,
        " Posted OnApBandChanged event for Band: ", static_cast<int>(band));

  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing onApBandChanged event: ", ex.what());
  }
}

void ApInterfaceManagerServerImpl::handleApConfigChangedEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);
  std::string apIdStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  try {
    telux::wlan::Id apId = static_cast<telux::wlan::Id>(std::stoi(apIdStr));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ::eventService::EventResponse anyResponse;
    wlanStub::OnApConfigChanged grpcEvent;
    grpcEvent.set_ap_id(
        telux::wlan::WlanCommonUtilsStub::convertIdToGrpc(apId));
    anyResponse.set_filter(AP_FILTER);
    anyResponse.mutable_any()->PackFrom(grpcEvent);
    EventService::getInstance().updateEventQueue(anyResponse);
    LOG(DEBUG, __FUNCTION__,
        " Posted OnApConfigChanged event for AP ID: ", static_cast<int>(apId));

  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing onApConfigChanged event: ",
        ex.what());
  }
}