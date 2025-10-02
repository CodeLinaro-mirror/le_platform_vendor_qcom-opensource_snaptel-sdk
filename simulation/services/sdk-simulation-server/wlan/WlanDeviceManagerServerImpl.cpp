/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <chrono>
#include <thread>
#include <vector>

#include "WlanDeviceManagerServerImpl.hpp"
#include "WlanServerUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"

// Define API and State JSON file paths
#define WLAN_API_LOCAL_JSON "api/wlan/IWlanDeviceManager.json"
#define WLAN_STATE_JSON "system-state/wlan/IWlanDeviceManagerState.json"
#define AP_STATE_JSON "system-state/wlan/IApInterfaceManagerState.json"
#define STA_STATE_JSON "system-state/wlan/IStaInterfaceManagerState.json"

#include "libs/common/SimulationConfigParser.hpp" // Make sure this is included after the defines

#define DEFAULT_DELIMITER " "
#define WLAN_FILTER "wlan_dev"

// Helper function: Converts sequential count (1, 2, 3) to bitmask (1, 3, 7)
// Assumes AP/STA IDs are 1-indexed (PRIMARY=1, SECONDARY=2, etc.)
static int convertSequentialCountToBitmask(int sequentialCount) {
  if (sequentialCount <= 0)
    return 0;
  // (1 << count) - 1 gives a bitmask with 'count' number of 1s.
  // e.g., count=1 -> (1<<1)-1 = 1 (PRIMARY)
  // e.g., count=2 -> (1<<2)-1 = 3 (PRIMARY | SECONDARY)
  // e.g., count=3 -> (1<<3)-1 = 7 (PRIMARY | SECONDARY | TERTIARY)
  return (1 << sequentialCount) - 1;
}

// Helper function to check if WLAN is enabled
static bool isWlanEnabled() {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error =
      JsonParser::readFromJsonFile(deviceStateRootObj, WLAN_STATE_JSON);
  if (error == telux::common::ErrorCode::SUCCESS &&
      deviceStateRootObj.isMember("IWlanDeviceManager") &&
      deviceStateRootObj["IWlanDeviceManager"].isMember("isEnabled")) {
    return deviceStateRootObj["IWlanDeviceManager"]["isEnabled"].asBool();
  }
  LOG(WARNING, "isWlanEnabled", "Could not read isEnabled from ",
      WLAN_STATE_JSON, ". Defaulting to false.");
  return false; // Default to false if file not found or isEnabled not specified
}

// Helper function to get active numAp (bitmask) from
// IWlanDeviceManagerState.json
static int getNumApFromDeviceManagerState() {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error =
      JsonParser::readFromJsonFile(deviceStateRootObj, WLAN_STATE_JSON);
  if (error == telux::common::ErrorCode::SUCCESS &&
      deviceStateRootObj.isMember("IWlanDeviceManager") &&
      deviceStateRootObj["IWlanDeviceManager"].isMember("numAp")) {
    return deviceStateRootObj["IWlanDeviceManager"]["numAp"].asInt();
  }
  LOG(WARNING, "getNumApFromDeviceManagerState", "Could not read numAp from ",
      WLAN_STATE_JSON, ". Defaulting to 0.");
  return 0; // Default to 0 if file not found or numAp not specified
}

// Helper function to get active numSta (bitmask) from
// IWlanDeviceManagerState.json
static int getNumStaFromDeviceManagerState() {
  Json::Value deviceStateRootObj;
  telux::common::ErrorCode error =
      JsonParser::readFromJsonFile(deviceStateRootObj, WLAN_STATE_JSON);
  if (error == telux::common::ErrorCode::SUCCESS &&
      deviceStateRootObj.isMember("IWlanDeviceManager") &&
      deviceStateRootObj["IWlanDeviceManager"].isMember("numSta")) {
    return deviceStateRootObj["IWlanDeviceManager"]["numSta"].asInt();
  }
  LOG(WARNING, "getNumStaFromDeviceManagerState", "Could not read numSta from ",
      WLAN_STATE_JSON, ". Defaulting to 0.");
  return 0; // Default to 0 if file not found or numSta not specified
}

WlanDeviceManagerServerImpl::WlanDeviceManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
  taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

WlanDeviceManagerServerImpl::~WlanDeviceManagerServerImpl() {
  LOG(DEBUG, __FUNCTION__);
}

grpc::Status WlanDeviceManagerServerImpl::InitService(
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

  int cbDelay = rootObj["IWlanDeviceManager"]["IsSubsystemReadyDelay"].asInt();
  std::string cbStatus =
      rootObj["IWlanDeviceManager"]["IsSubsystemReady"].asString();
  telux::common::ServiceStatus status =
      telux::common::CommonUtils::mapServiceStatus(cbStatus);
  LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

  response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
  response->set_delay(cbDelay);

  if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
    std::vector<std::string> filters = {WLAN_FILTER};
    auto &serverEventManager = ServerEventManager::getInstance();
    serverEventManager.registerListener(shared_from_this(), filters);
  }

  return grpc::Status::OK;
}

grpc::Status
WlanDeviceManagerServerImpl::Enable(ServerContext *context,
                                    const wlanStub::EnableRequest *request,
                                    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value deviceManagerStateRootObj;
  JsonParser::readFromJsonFile(deviceManagerStateRootObj, WLAN_STATE_JSON);

  Json::Value apManagerStateRootObj;
  JsonParser::readFromJsonFile(apManagerStateRootObj, AP_STATE_JSON);

  Json::Value staManagerStateRootObj;
  JsonParser::readFromJsonFile(staManagerStateRootObj, STA_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IWlanDeviceManager", "enable",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    bool currentEnableState =
        deviceManagerStateRootObj["IWlanDeviceManager"]["isEnabled"].asBool();
    bool requestedEnableState = request->enable();

    if (currentEnableState == requestedEnableState) {
      LOG(INFO, __FUNCTION__, " WLAN enable state is already ",
          (requestedEnableState ? "enabled" : "disabled"),
          ". Returning NO_EFFECT.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::NO_EFFECT));
      return grpc::Status::OK;
    }

    deviceManagerStateRootObj["IWlanDeviceManager"]["isEnabled"] =
        requestedEnableState;

    // Simulate onWlanEnableChanged event
    wlanStub::OnWlanEnableChanged enableEvent;
    enableEvent.set_enable(requestedEnableState);
    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter(WLAN_FILTER);
    anyResponse.mutable_any()->PackFrom(enableEvent);
    EventService::getInstance().updateEventQueue(anyResponse);

    // Update active numAp and numSta based on enable/disable state
    if (requestedEnableState) {
      // Convert configured sequential counts to bitmasks for active state
      int configuredApCount =
          deviceManagerStateRootObj["IWlanDeviceManager"]["configuredNumAp"]
              .asInt();
      int configuredStaCount =
          deviceManagerStateRootObj["IWlanDeviceManager"]["configuredNumSta"]
              .asInt();

      deviceManagerStateRootObj["IWlanDeviceManager"]["numAp"] =
          convertSequentialCountToBitmask(configuredApCount);
      deviceManagerStateRootObj["IWlanDeviceManager"]["numSta"] =
          convertSequentialCountToBitmask(configuredStaCount);

      // Populate apStatus in IApInterfaceManagerState.json for enabled APs
      if (apManagerStateRootObj.isMember("IApInterfaceManager") &&
          apManagerStateRootObj["IApInterfaceManager"].isMember(
              "configuredApConfig")) {
        apManagerStateRootObj["IApInterfaceManager"]["apStatus"]
            .clear(); // Clear existing status

        // Get actual AP interface names from config
        std::vector<std::string> apInterfaceNames =
            telux::wlan::server::WlanServerUtils::getInterfaceNames(
                "sim.wlan.ap_interfaces");

        for (int i = 1; i <= configuredApCount;
             ++i) { // Iterate up to the configured count
          std::string apIdStr =
              telux::wlan::server::WlanServerUtils::getTeluxIdString(
                  static_cast<telux::wlan::Id>(i));
          // Find the matching configured AP and add it to apStatus
          for (const auto &configuredAp :
               apManagerStateRootObj["IApInterfaceManager"]
                                    ["configuredApConfig"]) {
            if (configuredAp["id"].asString() == apIdStr) {
              Json::Value newApStatus;
              newApStatus["id"] = apIdStr;

              std::string ifaceName;
              if (static_cast<size_t>(i - 1) < apInterfaceNames.size()) {
                ifaceName = apInterfaceNames[i - 1];
                newApStatus["name"] = ifaceName;
                newApStatus["ipv4Address"] =
                    telux::wlan::server::WlanServerUtils::getIpv4Address(
                        ifaceName);
                newApStatus["macAddress"] =
                    telux::wlan::server::WlanServerUtils::getMacAddress(
                        ifaceName);
                newApStatus["ipv6Address"] =
                    telux::wlan::server::WlanServerUtils::getIpv6Address(
                        ifaceName);
              } else {
                // Fallback to generated names/addresses if not in config
                ifaceName = "wlan_ap" + std::to_string(i);
                newApStatus["name"] = ifaceName;
                newApStatus["ipv4Address"] =
                    "192.168.0." + std::to_string(i); // Placeholder IP
                newApStatus["macAddress"] =
                    "00:1A:2B:3C:4D:" +
                    ((i < 10) ? ("0" + std::to_string(i))
                              : std::to_string(i)); // Placeholder MAC
                newApStatus["ipv6Address"] = ""; // No dynamic IPv6 for fallback
              }

              if (configuredAp.isMember("network") &&
                  configuredAp["network"].isArray()) {
                newApStatus["network"] = configuredAp["network"];
              }
              apManagerStateRootObj["IApInterfaceManager"]["apStatus"].append(
                  newApStatus);
              break;
            }
          }
        }
        JsonParser::writeToJsonFile(apManagerStateRootObj, AP_STATE_JSON);
      }

      // Populate staStatus in IStaInterfaceManagerState.json for enabled STAs
      // and send events
      if (staManagerStateRootObj.isMember("IStaInterfaceManager") &&
          staManagerStateRootObj["IStaInterfaceManager"].isMember(
              "configuredStaConfig")) {
        staManagerStateRootObj["IStaInterfaceManager"]["staStatus"]
            .clear(); // Clear existing status

        // Get actual STA interface names from config
        std::vector<std::string> staInterfaceNames =
            telux::wlan::server::WlanServerUtils::getInterfaceNames(
                "sim.wlan.sta_interfaces");

        for (int i = 1; i <= configuredStaCount;
             ++i) { // Iterate up to the configured count
          telux::wlan::Id staId = static_cast<telux::wlan::Id>(i);
          std::string staIdStr =
              telux::wlan::server::WlanServerUtils::getTeluxIdString(staId);
          // Find the matching configured STA and add it to staStatus
          for (const auto &configuredSta :
               staManagerStateRootObj["IStaInterfaceManager"]
                                     ["configuredStaConfig"]) {
            if (configuredSta["staId"].asString() == staIdStr) {
              Json::Value newStaStatus;
              newStaStatus["id"] = staIdStr;

              std::string ifaceName;
              if (static_cast<size_t>(i - 1) < staInterfaceNames.size()) {
                ifaceName = staInterfaceNames[i - 1];
                newStaStatus["name"] = ifaceName;
                newStaStatus["ipv4Address"] =
                    ""; // Initially disconnected, will be populated on connect
                newStaStatus["ipv6Address"] = ""; // Initially disconnected
                newStaStatus["macAddress"] = "";  // Initially disconnected
              } else {
                // Fallback to generated names if not in config
                ifaceName = "wlan_sta" + std::to_string(i);
                newStaStatus["name"] = ifaceName;
                newStaStatus["ipv4Address"] =
                    ""; // Initially disconnected, will be populated on connect
                newStaStatus["ipv6Address"] = ""; // Initially disconnected
                newStaStatus["macAddress"] = "";  // Initially disconnected
              }

              newStaStatus["status"] = telux::wlan::server::WlanServerUtils::
                  getStaInterfaceStatusString(
                      telux::wlan::StaInterfaceStatus::DISCONNECTED);
              newStaStatus["connectionStatus"] = telux::wlan::server::
                  WlanServerUtils::getStaConnectionStatusString(
                      telux::wlan::StaConnectionStatus::UNKNOWN);
              staManagerStateRootObj["IStaInterfaceManager"]["staStatus"]
                  .append(newStaStatus);

              // Simulate CONNECTING and CONNECTED events
              // 1. CONNECTING state
              // Update the entry in the JSON object (temporarily for event
              // dispatch)
              Json::Value *currentStaNode =
                  telux::wlan::server::WlanServerUtils::findStaStatusEntry(
                      staManagerStateRootObj["IStaInterfaceManager"]
                                            ["staStatus"],
                      staId);
              if (currentStaNode) {
                (*currentStaNode)["status"] = telux::wlan::server::
                    WlanServerUtils::getStaInterfaceStatusString(
                        telux::wlan::StaInterfaceStatus::CONNECTING);
                (*currentStaNode)["connectionStatus"] = telux::wlan::server::
                    WlanServerUtils::getStaConnectionStatusString(
                        telux::wlan::StaConnectionStatus::UNKNOWN);
              }
              telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
                  staId, telux::wlan::StaInterfaceStatus::CONNECTING,
                  telux::wlan::StaConnectionStatus::UNKNOWN,
                  newStaStatus["name"].asString(), "",
                  ""); // Name from newStaStatus["name"], IP, MAC not yet known
                       // in CONNECTING
              std::this_thread::sleep_for(
                  std::chrono::milliseconds(200)); // Simulate connection delay

              // 2. CONNECTED state
              // Dynamically get IP/MAC if actual interface, else use
              // placeholders
              if (static_cast<size_t>(i - 1) < staInterfaceNames.size()) {
                newStaStatus["ipv4Address"] =
                    telux::wlan::server::WlanServerUtils::getIpv4Address(
                        ifaceName);
                newStaStatus["macAddress"] =
                    telux::wlan::server::WlanServerUtils::getMacAddress(
                        ifaceName);
                newStaStatus["ipv6Address"] =
                    telux::wlan::server::WlanServerUtils::getIpv6Address(
                        ifaceName);
              } else {
                // Fallback for IP/MAC addresses
                newStaStatus["ipv4Address"] =
                    "192.168.1." +
                    std::to_string(static_cast<int>(staId)); // Placeholder IP
                newStaStatus["macAddress"] =
                    "00:AA:BB:CC:DD:" +
                    ((static_cast<int>(staId) < 10)
                         ? ("0" + std::to_string(static_cast<int>(staId)))
                         : std::to_string(
                               static_cast<int>(staId))); // Placeholder MAC
                newStaStatus["ipv6Address"] =
                    ""; // No dynamic IPv6 for fallback
              }

              newStaStatus["status"] = telux::wlan::server::WlanServerUtils::
                  getStaInterfaceStatusString(
                      telux::wlan::StaInterfaceStatus::CONNECTED);
              newStaStatus["connectionStatus"] = telux::wlan::server::
                  WlanServerUtils::getStaConnectionStatusString(
                      telux::wlan::StaConnectionStatus::SUCCESS);

              // Update the entry in the JSON object
              if (currentStaNode) {
                (*currentStaNode)["name"] = newStaStatus["name"];
                (*currentStaNode)["ipv4Address"] = newStaStatus["ipv4Address"];
                (*currentStaNode)["ipv6Address"] = newStaStatus["ipv6Address"];
                (*currentStaNode)["macAddress"] = newStaStatus["macAddress"];
                (*currentStaNode)["status"] = newStaStatus["status"];
                (*currentStaNode)["connectionStatus"] =
                    newStaStatus["connectionStatus"];
              }

              telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
                  staId, telux::wlan::StaInterfaceStatus::CONNECTED,
                  telux::wlan::StaConnectionStatus::SUCCESS,
                  newStaStatus["name"].asString(),
                  newStaStatus["ipv4Address"].asString(),
                  newStaStatus["macAddress"].asString());
              break;
            }
          }
        }
        JsonParser::writeToJsonFile(staManagerStateRootObj, STA_STATE_JSON);
      }

    } else { // requestedEnableState is false (Disabling WLAN)
      // When disabling, active counts (bitmasks) are 0
      deviceManagerStateRootObj["IWlanDeviceManager"]["numAp"] = 0;
      deviceManagerStateRootObj["IWlanDeviceManager"]["numSta"] = 0;

      // Clear apStatus in IApInterfaceManagerState.json when WLAN is disabled
      if (apManagerStateRootObj.isMember("IApInterfaceManager") &&
          apManagerStateRootObj["IApInterfaceManager"].isMember("apStatus")) {
        apManagerStateRootObj["IApInterfaceManager"]["apStatus"].clear();
        JsonParser::writeToJsonFile(apManagerStateRootObj, AP_STATE_JSON);
      }

      // Send DISCONNECTED events for all currently active STAs, then clear
      // staStatus
      if (staManagerStateRootObj.isMember("IStaInterfaceManager") &&
          staManagerStateRootObj["IStaInterfaceManager"].isMember(
              "staStatus")) {
        // Iterate over a copy or stored list of current STAs to send events
        Json::Value currentStaStatuses =
            staManagerStateRootObj["IStaInterfaceManager"]["staStatus"];
        for (Json::ArrayIndex i = 0; i < currentStaStatuses.size(); ++i) {
          telux::wlan::Id staId =
              telux::wlan::server::WlanServerUtils::convertIdFromGrpc(
                  telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                      currentStaStatuses[i]["id"].asString()));
          telux::wlan::server::WlanServerUtils::sendStaStatusChangedEvent(
              staId, telux::wlan::StaInterfaceStatus::DISCONNECTED,
              telux::wlan::StaConnectionStatus::UNKNOWN, "", "",
              ""); // Clear details for disconnected
        }
        staManagerStateRootObj["IStaInterfaceManager"]["staStatus"].clear();
        JsonParser::writeToJsonFile(staManagerStateRootObj, STA_STATE_JSON);
      }
    }

    JsonParser::writeToJsonFile(deviceManagerStateRootObj, WLAN_STATE_JSON);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status
WlanDeviceManagerServerImpl::SetMode(ServerContext *context,
                                     const wlanStub::SetModeRequest *request,
                                     wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IWlanDeviceManager", "setMode",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    // These are the sequential counts as per the user's clarification (1, 2, or
    // 3)
    int numOfApSequentialCount = request->num_of_ap();
    int numOfStaSequentialCount = request->num_of_sta();

    // Validate num_of_ap sequential count
    if (numOfApSequentialCount < 0 || numOfApSequentialCount > 3) {
      LOG(ERROR, __FUNCTION__,
          " Invalid argument: num_of_ap must be between 0 and 3 (inclusive, "
          "representing count of PRIMARY, SECONDARY, TERTIARY). Provided: ",
          numOfApSequentialCount);
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }
    // Validate num_of_sta sequential count
    if (numOfStaSequentialCount < 0 ||
        numOfStaSequentialCount >
            1) { // Assuming only PRIMARY STA can be enabled sequentially
      LOG(ERROR, __FUNCTION__,
          " Invalid argument: num_of_sta must be 0 or 1. Provided: ",
          numOfStaSequentialCount);
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }
    // Do not allow both to be zero
    if (numOfApSequentialCount == 0 && numOfStaSequentialCount == 0) {
      LOG(ERROR, __FUNCTION__,
          " Invalid argument: num_of_ap and num_of_sta cannot both be zero.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    // Store these as configured sequential counts.
    stateRootObj["IWlanDeviceManager"]["configuredNumAp"] =
        numOfApSequentialCount;
    stateRootObj["IWlanDeviceManager"]["configuredNumSta"] =
        numOfStaSequentialCount;

    JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status
WlanDeviceManagerServerImpl::GetConfig(ServerContext *context,
                                       const ::google::protobuf::Empty *request,
                                       wlanStub::GetConfigResponse *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IWlanDeviceManager", "getConfig",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    // Return configured sequential counts directly
    response->set_num_ap(
        stateRootObj["IWlanDeviceManager"]["configuredNumAp"].asInt());
    response->set_num_sta(
        stateRootObj["IWlanDeviceManager"]["configuredNumSta"].asInt());

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status
WlanDeviceManagerServerImpl::GetStatus(ServerContext *context,
                                       const ::google::protobuf::Empty *request,
                                       wlanStub::GetStatusResponse *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value deviceManagerStateRootObj;
  JsonParser::readFromJsonFile(deviceManagerStateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IWlanDeviceManager", "getStatus",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    response->set_is_enabled(
        deviceManagerStateRootObj["IWlanDeviceManager"]["isEnabled"].asBool());

    // Get active allowed number of APs and STAs (these are now bitmasks)
    int activeNumApBitmask = getNumApFromDeviceManagerState();
    int activeNumStaBitmask = getNumStaFromDeviceManagerState();

    // 1. Fetch AP Status from IApInterfaceManagerState.json
    Json::Value apManagerRootObj;
    if (JsonParser::readFromJsonFile(apManagerRootObj, AP_STATE_JSON) ==
        telux::common::ErrorCode::SUCCESS) {
      const Json::Value &apStatusArray =
          apManagerRootObj["IApInterfaceManager"]["apStatus"];
      if (apStatusArray.isArray()) {
        for (const auto &apStatusNode : apStatusArray) {
          telux::wlan::Id teluxApId =
              telux::wlan::server::WlanServerUtils::convertIdFromGrpc(
                  telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                      apStatusNode["id"].asString()));

          // Filter based on activeNumApBitmask: Check if the bit corresponding
          // to teluxApId is set. (static_cast<int>(teluxApId) - 1) gives the
          // 0-indexed bit position.
          if ((activeNumApBitmask & (1 << (static_cast<int>(teluxApId) - 1))) ==
              0) {
            LOG(INFO, __FUNCTION__, "Skipping AP ID ",
                static_cast<int>(teluxApId),
                " in status as it's not enabled by active numAp bitmask (",
                activeNumApBitmask, ")");
            continue; // Skip this AP if its bit is not set
          }

          wlanStub::InterfaceStatus apIfaceStatus;
          apIfaceStatus.set_device(
              telux::wlan::server::WlanServerUtils::getHwDeviceTypeFromString(
                  "QCA6696")); // Placeholder
          *apIfaceStatus.add_ap_status() =
              telux::wlan::server::WlanServerUtils::getApStatusFromPtree(
                  apStatusNode); // Corrected
          *response->add_status() = apIfaceStatus;
        }
      } else {
        LOG(WARNING, __FUNCTION__,
            " 'apStatus' not found or not an array in "
            "IApInterfaceManagerState.json.");
      }
    } else {
      LOG(WARNING, __FUNCTION__,
          " Failed to read IApInterfaceManagerState.json for AP status.");
    }

    // 2. Fetch STA Status from IStaInterfaceManagerState.json
    Json::Value staManagerRootObj;
    if (JsonParser::readFromJsonFile(staManagerRootObj, STA_STATE_JSON) ==
        telux::common::ErrorCode::SUCCESS) {
      const Json::Value &staStatusArray =
          staManagerRootObj["IStaInterfaceManager"]["staStatus"];
      if (staStatusArray.isArray()) {
        for (const auto &staStatusNode : staStatusArray) {
          telux::wlan::Id teluxStaId =
              telux::wlan::server::WlanServerUtils::convertIdFromGrpc(
                  telux::wlan::server::WlanServerUtils::getTeluxIdFromString(
                      staStatusNode["id"].asString()));

          // Filter based on activeNumStaBitmask: Check if the bit corresponding
          // to teluxStaId is set. (static_cast<int>(teluxStaId) - 1) gives the
          // 0-indexed bit position.
          if ((activeNumStaBitmask &
               (1 << (static_cast<int>(teluxStaId) - 1))) == 0) {
            LOG(INFO, __FUNCTION__, "Skipping STA ID ",
                static_cast<int>(teluxStaId),
                " in status as it's not enabled by active numSta bitmask (",
                activeNumStaBitmask, ")");
            continue; // Skip this STA if its bit is not set
          }

          wlanStub::InterfaceStatus staIfaceStatus;
          staIfaceStatus.set_device(
              telux::wlan::server::WlanServerUtils::getHwDeviceTypeFromString(
                  "QCA6696")); // Placeholder
          *staIfaceStatus.add_sta_status() =
              telux::wlan::server::WlanServerUtils::getStaStatusFromPtree(
                  staStatusNode); // Corrected
          *response->add_status() = staIfaceStatus;
        }
      } else {
        LOG(WARNING, __FUNCTION__,
            " 'staStatus' not found or not an array in "
            "IStaInterfaceManagerState.json.");
      }
    } else {
      LOG(WARNING, __FUNCTION__,
          " Failed to read IStaInterfaceManagerState.json for STA status.");
    }

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status WlanDeviceManagerServerImpl::SetActiveCountry(
    ServerContext *context, const wlanStub::SetActiveCountryRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IWlanDeviceManager",
      "setActiveCountry", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    // Ensure "simulatedRegulatoryParams" object exists
    if (!stateRootObj["IWlanDeviceManager"].isMember(
            "simulatedRegulatoryParams")) {
      LOG(ERROR, __FUNCTION__,
          " 'simulatedRegulatoryParams' object not found in state JSON. Cannot "
          "set country.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INTERNAL_ERR));
      return grpc::Status::OK;
    }

    std::string currentCountry =
        stateRootObj["IWlanDeviceManager"]["simulatedRegulatoryParams"]
                    ["country"]
                        .asString();
    std::string requestedCountry = request->country();

    // Validate country code length
    if (requestedCountry.length() != 2) {
      LOG(ERROR, __FUNCTION__,
          " Invalid argument: Country code must be exactly 2 characters long. "
          "Provided: '",
          requestedCountry, "'");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_ARGUMENTS));
      return grpc::Status::OK;
    }

    // Check if the requested country is the same as the current one
    if (currentCountry == requestedCountry) {
      LOG(INFO, __FUNCTION__, " Requested country '", requestedCountry,
          "' is already active. Returning NO_EFFECT.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::NO_EFFECT));
      return grpc::Status::OK;
    }

    // Update the country within simulatedRegulatoryParams
    stateRootObj["IWlanDeviceManager"]["simulatedRegulatoryParams"]["country"] =
        requestedCountry;
    JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status WlanDeviceManagerServerImpl::GetRegulatoryParams(
    ServerContext *context, const ::google::protobuf::Empty *request,
    wlanStub::GetRegulatoryParamsResponse *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IWlanDeviceManager",
      "getRegulatoryParams", data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled() || (getNumApFromDeviceManagerState() == 0 &&
                             getNumStaFromDeviceManagerState() == 0)) {
      LOG(ERROR, __FUNCTION__,
          "Operation denied. WLAN is not enabled or no AP/STA interfaces are "
          "active.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    if (stateRootObj["IWlanDeviceManager"].isMember(
            "simulatedRegulatoryParams")) {
      *response->mutable_regulatory_params() =
          telux::wlan::server::WlanServerUtils::getRegulatoryParamsFromPtree(
              stateRootObj["IWlanDeviceManager"]["simulatedRegulatoryParams"]);
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(data.error));
    } else {
      LOG(ERROR, __FUNCTION__,
          " 'simulatedRegulatoryParams' not found in state JSON. Returning "
          "INTERNAL_ERROR.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INTERNAL_ERROR));
    }
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status WlanDeviceManagerServerImpl::SetTxPower(
    ServerContext *context, const wlanStub::SetTxPowerRequest *request,
    wlanStub::DefaultReply *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IWlanDeviceManager", "setTxPower",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled() || (getNumApFromDeviceManagerState() == 0 &&
                             getNumStaFromDeviceManagerState() == 0)) {
      LOG(ERROR, __FUNCTION__,
          "Operation denied. WLAN is not enabled or no AP/STA interfaces are "
          "active.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    // Ensure "simulatedRegulatoryParams" object exists
    if (!stateRootObj["IWlanDeviceManager"].isMember(
            "simulatedRegulatoryParams")) {
      LOG(ERROR, __FUNCTION__,
          " 'simulatedRegulatoryParams' object not found in state JSON. Cannot "
          "set TxPower.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::INTERNAL_ERR));
      return grpc::Status::OK;
    }

    uint32_t currentTxPower =
        stateRootObj["IWlanDeviceManager"]["simulatedRegulatoryParams"]
                    ["txPowerMw"]
                        .asUInt();
    uint32_t requestedTxPower = request->tx_power_mw();

    uint32_t maxTxPower = 0;
    if (stateRootObj["IWlanDeviceManager"]["simulatedRegulatoryParams"]
            .isMember("maxTxPowerMw")) {
      maxTxPower = stateRootObj["IWlanDeviceManager"]
                               ["simulatedRegulatoryParams"]["maxTxPowerMw"]
                                   .asUInt();
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'maxTxPowerMw' not found in state JSON. Using default max of 0.");
    }

    if (requestedTxPower > maxTxPower) {
      LOG(WARNING, __FUNCTION__, " Requested TxPower (", requestedTxPower,
          " mW) exceeds maxTxPower (", maxTxPower,
          " mW). Setting to maxTxPower.");
      requestedTxPower = maxTxPower; // Ceil to max allowed value
    }

    if (currentTxPower == requestedTxPower) {
      LOG(INFO, __FUNCTION__, " Requested TxPower (", requestedTxPower,
          " mW) is already set. Returning NO_EFFECT.");
      response->set_error(static_cast<commonStub::ErrorCode>(
          telux::common::ErrorCode::NO_EFFECT));
      return grpc::Status::OK;
    }

    stateRootObj["IWlanDeviceManager"]["simulatedRegulatoryParams"]
                ["txPowerMw"] = requestedTxPower;
    JsonParser::writeToJsonFile(stateRootObj, WLAN_STATE_JSON);

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->set_error(static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

grpc::Status WlanDeviceManagerServerImpl::GetTxPower(
    ServerContext *context, const ::google::protobuf::Empty *request,
    wlanStub::GetTxPowerResponse *response) {

  LOG(DEBUG, __FUNCTION__);
  Json::Value stateRootObj;
  JsonParser::readFromJsonFile(stateRootObj, WLAN_STATE_JSON);

  telux::common::JsonData data;
  telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
      WLAN_API_LOCAL_JSON, WLAN_STATE_JSON, "IWlanDeviceManager", "getTxPower",
      data);

  if (error == telux::common::ErrorCode::SUCCESS) {
    if (!isWlanEnabled() || (getNumApFromDeviceManagerState() == 0 &&
                             getNumStaFromDeviceManagerState() == 0)) {
      LOG(ERROR, __FUNCTION__,
          "Operation denied. WLAN is not enabled or no AP/STA interfaces are "
          "active.");
      response->mutable_default_reply()->set_error(
          static_cast<commonStub::ErrorCode>(
              telux::common::ErrorCode::INVALID_STATE));
      return grpc::Status::OK;
    }

    if (stateRootObj["IWlanDeviceManager"].isMember(
            "simulatedRegulatoryParams") &&
        stateRootObj["IWlanDeviceManager"]["simulatedRegulatoryParams"]
            .isMember("txPowerMw")) {
      response->set_tx_power_mw(
          stateRootObj["IWlanDeviceManager"]["simulatedRegulatoryParams"]
                      ["txPowerMw"]
                          .asUInt());
    } else {
      LOG(WARNING, __FUNCTION__,
          " 'simulatedRegulatoryParams' or 'txPowerMw' not found in state "
          "JSON. Returning default TxPower 0.");
      response->set_tx_power_mw(0); // Default or error value
    }

    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(data.error));
  } else {
    response->mutable_default_reply()->set_error(
        static_cast<commonStub::ErrorCode>(error));
  }
  return grpc::Status::OK;
}

void WlanDeviceManagerServerImpl::onEventUpdate(
    ::eventService::UnsolicitedEvent message) {
  if (message.filter() == WLAN_FILTER) {
    onEventUpdate(message.event());
  }
}

void WlanDeviceManagerServerImpl::onEventUpdate(std::string event) {
  std::string token =
      telux::common::EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
  LOG(DEBUG, __FUNCTION__, "Received event token: ", token);
  if (token == "onWlanServiceStatusChange") {
    handleWlanServiceStatusChangeEvent(event);
  } else if (token == "onWlanTempCrossed") {
    handleWlanTempCrossedEvent(event);
  } else if (token == "onWlanEnableChanged") {
    handleWlanEnableChangedEvent(event);
  } else {
    LOG(ERROR, __FUNCTION__, "Unhandled event flag: ", token);
  }
}

void WlanDeviceManagerServerImpl::handleWlanServiceStatusChangeEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);
  std::string statusStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  try {
    telux::common::ServiceStatus serviceStatus =
        telux::common::CommonUtils::mapServiceStatus(statusStr);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ::eventService::EventResponse anyResponse;
    wlanStub::OnWlanServiceStatusChange grpcEvent;
    grpcEvent.set_status(static_cast<commonStub::ServiceStatus>(serviceStatus));
    anyResponse.set_filter(WLAN_FILTER);
    anyResponse.mutable_any()->PackFrom(grpcEvent);
    EventService::getInstance().updateEventQueue(anyResponse);
    LOG(DEBUG, __FUNCTION__,
        " Posted OnWlanServiceStatusChange event with status: ", statusStr);

  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing onWlanServiceStatusChange event: ",
        ex.what());
  }
}

void WlanDeviceManagerServerImpl::handleWlanTempCrossedEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);

  // Check if WLAN is enabled before processing temperature events
  if (!isWlanEnabled()) {
    LOG(INFO, __FUNCTION__,
        " WLAN is not enabled. Skipping onWlanTempCrossed event processing.");
    return;
  }

  std::string tempStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  std::string perfStateStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  try {
    float temperature = std::stof(tempStr);
    // Cast the return of getDevicePerfStateFromString to
    // telux::wlan::DevicePerfState
    telux::wlan::DevicePerfState perfState =
        static_cast<telux::wlan::DevicePerfState>(
            telux::wlan::server::WlanServerUtils::getDevicePerfStateFromString(
                perfStateStr));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ::eventService::EventResponse anyResponse;
    wlanStub::OnWlanTempCrossed grpcEvent;
    grpcEvent.set_temperature(temperature);
    grpcEvent.set_perf_state(static_cast<wlanStub::DevicePerfState>(perfState));
    anyResponse.set_filter(WLAN_FILTER);
    anyResponse.mutable_any()->PackFrom(grpcEvent);
    EventService::getInstance().updateEventQueue(anyResponse);
    LOG(DEBUG, __FUNCTION__,
        " Posted OnWlanTempCrossed event with temp: ", tempStr,
        ", perf state: ", perfStateStr);

  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing onWlanTempCrossed event: ",
        ex.what());
  }
}

void WlanDeviceManagerServerImpl::handleWlanEnableChangedEvent(
    std::string eventStr) {
  LOG(DEBUG, __FUNCTION__, " event:", eventStr);
  std::string enableStr =
      telux::common::EventParserUtil::getNextToken(eventStr, DEFAULT_DELIMITER);
  try {
    bool enable = (enableStr == "true" || enableStr == "1");

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    ::eventService::EventResponse anyResponse;
    wlanStub::OnWlanEnableChanged grpcEvent;
    grpcEvent.set_enable(enable);
    anyResponse.set_filter(WLAN_FILTER);
    anyResponse.mutable_any()->PackFrom(grpcEvent);
    EventService::getInstance().updateEventQueue(anyResponse);
    LOG(DEBUG, __FUNCTION__,
        " Posted OnWlanEnableChanged event with enable: ", enableStr);

  } catch (const std::exception &ex) {
    LOG(ERROR, __FUNCTION__,
        "Exception occurred while parsing onWlanEnableChanged event: ",
        ex.what());
  }
}