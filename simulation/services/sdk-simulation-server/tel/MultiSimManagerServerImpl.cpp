/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "MultiSimManagerServerImpl.hpp"
#include "libs/tel/TelDefinesStub.hpp"
#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/MultiSimDefines.hpp>
#include <iterator>
#include <set>
#include <sstream>

#define JSON_PATH1 MULTI_SIM_STATE_JSON_PATH1
#define JSON_PATH2 "system-state/tel/IMultiSimManagerStateSlot2.json"
#define JSON_PATH3 "api/tel/IMultiSimManagerSlot1.json"
#define JSON_PATH4 "api/tel/IMultiSimManagerSlot2.json"

#define CARDMGR_JSON_SLOT1 "api/tel/ICardManagerSlot1.json"
#define CARDMGR_JSON_SLOT2 "api/tel/ICardManagerSlot2.json"

#define DEFAULT_SLOT_ID 1
#define MAX_SLOT_ID 2
#define INVALID_SLOT_ID -1

MultiSimManagerServerImpl::MultiSimManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    readJson();
}

MultiSimManagerServerImpl::~MultiSimManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status MultiSimManagerServerImpl::readJson() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error
        = JsonParser::readFromJsonFile(rootObjSystemStateSlot1_, JSON_PATH1);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH1);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjSystemStateSlot2_, JSON_PATH2);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH2);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjApiResponseSlot1_, JSON_PATH3);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH3);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    error = JsonParser::readFromJsonFile(rootObjApiResponseSlot2_, JSON_PATH4);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH4);
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    jsonObjSystemStateSlot_[1]     = rootObjSystemStateSlot1_;
    jsonObjSystemStateSlot_[2]     = rootObjSystemStateSlot2_;
    jsonObjSystemStateFileName_[1] = JSON_PATH1;
    jsonObjSystemStateFileName_[2] = JSON_PATH2;
    // Api response
    jsonObjApiResponseSlot_[1]     = rootObjApiResponseSlot1_;
    jsonObjApiResponseSlot_[2]     = rootObjApiResponseSlot2_;
    jsonObjApiResponseFileName_[1] = JSON_PATH3;
    jsonObjApiResponseFileName_[2] = JSON_PATH4;
    return grpc::Status::OK;
}

void MultiSimManagerServerImpl::getJsonForSystemData(
    int phoneId, std::string &jsonfilename, Json::Value &rootObj) {
    jsonfilename = jsonObjSystemStateFileName_[phoneId];
    rootObj      = jsonObjSystemStateSlot_[phoneId];
}

void MultiSimManagerServerImpl::getJsonForApiResponseSlot(
    int phoneId, std::string &jsonfilename, Json::Value &rootObj) {
    jsonfilename = jsonObjApiResponseFileName_[phoneId];
    rootObj      = jsonObjApiResponseSlot_[phoneId];
}

void MultiSimManagerServerImpl::getApiConfigureFromJson(const int slotId, const std::string apiname,
    telux::common::Status &status, telux::common::ErrorCode &ec, int &delay) {
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    if (readJson().ok()) {
        getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, rootObj);

        CommonUtils::getValues(rootObj, "IMultiSimManager", apiname, status, ec, delay);
    }
}

bool MultiSimManagerServerImpl::isCallbackNeeded(Json::Value rootObj, std::string apiname) {
    int value       = rootObj["IMultiSimManager"][apiname]["callbackDelay"].asInt();
    bool isCallback = true;
    if (value == -1) {
        isCallback = false;
    }
    return isCallback;
}

std::string MultiSimManagerServerImpl::getCardMgrJsonPathForLogicalSlot(int logicalSlotId) {
    switch (static_cast<telStub::LogicalSlotId>(logicalSlotId)) {
        case telStub::LogicalSlotId::LOGICAL_SLOT_ID_1:
            return CARDMGR_JSON_SLOT1;
        case telStub::LogicalSlotId::LOGICAL_SLOT_ID_2:
            return CARDMGR_JSON_SLOT2;
        default:
            return "";  // invalid logical slot
    }
}

std::string MultiSimManagerServerImpl::readCardModeForLogicalSlot(int logicalSlotId) {
    std::string path = getCardMgrJsonPathForLogicalSlot(logicalSlotId);
    if (path.empty()) {
        return "NONE";
    }

    Json::Value cardRoot;
    telux::common::ErrorCode ec = JsonParser::readFromJsonFile(cardRoot, path);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to read card manager json: ", path);
        return "NONE";
    }

    return cardRoot["ICardManager"].get("mode", "NONE").asString();
}

bool MultiSimManagerServerImpl::isPortAllowedForMode(const std::string &mode, int portId) {
    if (mode == "NONE") {
        return (portId == 0);
    } else if (mode == "MEP_B") {
        return (portId == 0 || portId == 1);
    } else if (mode == "MEP_A1") {
        return (portId == 1 || portId == 2);
    }
    return false;
}

grpc::Status MultiSimManagerServerImpl::GetLogicalSlotMapping(ServerContext *context,
    const google::protobuf::Empty *request, telStub::GetLogicalSlotMappingResponse *response) {
    LOG(DEBUG, __FUNCTION__);

    std::string jsonfilename = "";
    Json::Value rootObj;

    grpc::Status readStatus = readJson();
    if (!readStatus.ok()) {
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    getJsonForSystemData(DEFAULT_SLOT_ID, jsonfilename, rootObj);

    const Json::Value &configuredMapping
        = rootObj["IMultiSimManager"]["logicalSlotMapping"]["mapInfo"];
    if (!configuredMapping.isArray()) {
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    Json::Value mapInfoArray = configuredMapping;

    if (mapInfoArray.isNull() || !mapInfoArray.isArray()) {
        LOG(WARNING, "mapInfo not found or invalid format in JSON");
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    // Clear existing response
    response->clear_map_info();

    for (Json::ArrayIndex i = 0; i < mapInfoArray.size(); ++i) {
        const Json::Value &mapInfo = mapInfoArray[i];

        std::string logicalSlotStr  = mapInfo.get("logicalSlotId", "SLOT_ID_INVALID").asString();
        std::string physicalSlotStr = mapInfo.get("physicalSlotId", "SLOT_ID_INVALID").asString();
        int portId                  = mapInfo.get("portId", 0).asInt();

        // Convert logical slot string -> enum
        telStub::LogicalSlotId logicalSlotId = telStub::LogicalSlotId::LOGICAL_SLOT_ID_INVALID;

        if (logicalSlotStr == "SLOT_ID_1") {
            logicalSlotId = telStub::LogicalSlotId::LOGICAL_SLOT_ID_1;
        } else if (logicalSlotStr == "SLOT_ID_2") {
            logicalSlotId = telStub::LogicalSlotId::LOGICAL_SLOT_ID_2;
        }

        // Convert physical slot string -> enum
        telStub::PhysicalSlotId physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_INVALID;

        if (physicalSlotStr == "SLOT_ID_1") {
            physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_1;
        } else if (physicalSlotStr == "SLOT_ID_2") {
            physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_2;
        }

        // Validate enum values
        if (logicalSlotId == telStub::LogicalSlotId::LOGICAL_SLOT_ID_INVALID
            || physicalSlotId == telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_INVALID) {
            LOG(WARNING, "Invalid slot mapping ignored: logical=%s physical=%s port=%d",
                logicalSlotStr.c_str(), physicalSlotStr.c_str(), portId);
            continue;
        }

        // OPTIONAL robustness: if JSON accidentally repeats a logical slot,
        // update the existing entry instead of adding duplicates.
        telStub::LogicalSlotMappingEntry *entry = nullptr;
        for (int j = 0; j < response->map_info_size(); ++j) {
            if (response->map_info(j).logical_slot_id() == static_cast<int32_t>(logicalSlotId)) {
                entry = response->mutable_map_info(j);
                break;
            }
        }

        if (!entry) {
            entry = response->add_map_info();
        }

        entry->set_logical_slot_id(logicalSlotId);

        telStub::LogicalSlotMapInfo *mapping = entry->mutable_map_info();
        mapping->set_physical_slot(physicalSlotId);
        mapping->set_port_id(portId);
    }

    response->set_error(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

grpc::Status MultiSimManagerServerImpl::ConfigureLogicalSlotMapping(ServerContext * /*context*/,
    const ::telStub::ConfigureLogicalSlotMappingRequest *request,
    ::telStub::ConfigureLogicalSlotMappingResponse *response) {

    LOG(INFO, __FUNCTION__, " ConfigureLogicalSlotMapping called");
    LOG(DEBUG, __FUNCTION__, " map_info size=", request->map_info().size());

    // Set both response fields explicitly.  The client reports the immediate
    // status and the asynchronous callback reports the error independently.
    response->set_status(commonStub::Status::SUCCESS);
    response->set_error(commonStub::ErrorCode::ERROR_CODE_SUCCESS);

    Json::Value rootObj;

    // -------- Read Slot1 system-state JSON --------
    telux::common::ErrorCode ec = JsonParser::readFromJsonFile(rootObj, JSON_PATH1);
    LOG(DEBUG, __FUNCTION__, " Read JSON_PATH1=", JSON_PATH1, " ec=", static_cast<int>(ec),
        " isNull=", rootObj.isNull(), " isObject=", rootObj.isObject());

    if (ec != telux::common::ErrorCode::SUCCESS || rootObj.isNull() || !rootObj.isObject()) {
        LOG(WARNING, __FUNCTION__, " Initializing empty rootObj");
        rootObj = Json::objectValue;
    }

    if (rootObj["IMultiSimManager"].isNull() || !rootObj["IMultiSimManager"].isObject()) {
        LOG(DEBUG, __FUNCTION__, " Creating IMultiSimManager object");
        rootObj["IMultiSimManager"] = Json::objectValue;
    }

    // -------- Build new logicalSlotMapping.mapInfo --------
    Json::Value newMapInfoArray(Json::arrayValue);

    // A physical slot may be shared only by two logical slots using the same
    // MEP mode: MEP_A1+MEP_A1 or MEP_B+MEP_B.  Any combination involving
    // NONE, or mixed MEP modes, is rejected.
    std::map<int, std::vector<std::string>> physicalSlotModes;
    std::set<std::pair<int, int>> usedPhysicalPorts;
    std::set<int> usedLogicalSlots;
    for (const auto &entry : request->map_info()) {
        const int logicalSlot  = entry.logical_slot_id();
        const int physicalSlot = entry.map_info().physical_slot();
        const int portId       = entry.map_info().port_id();

        if (logicalSlot != telStub::LogicalSlotId::LOGICAL_SLOT_ID_1
            && logicalSlot != telStub::LogicalSlotId::LOGICAL_SLOT_ID_2) {
            LOG(ERROR, __FUNCTION__, " Rejecting invalid logical slot ID: ", logicalSlot);
            response->set_status(commonStub::Status::FAILED);
            response->set_error(commonStub::ErrorCode::INVALID_ARGUMENTS);
            return grpc::Status::OK;
        }

        if (!usedLogicalSlots.insert(logicalSlot).second
            || !usedPhysicalPorts.emplace(physicalSlot, portId).second) {
            LOG(ERROR, __FUNCTION__,
                " Rejecting duplicate logical slot or physical port mapping: logicalSlot=",
                logicalSlot, " physicalSlot=", physicalSlot, " portId=", portId);
            response->set_status(commonStub::Status::FAILED);
            response->set_error(commonStub::ErrorCode::INVALID_ARGUMENTS);
            return grpc::Status::OK;
        }

        if (physicalSlot != telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_1
            && physicalSlot != telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_2) {
            LOG(ERROR, __FUNCTION__, " Rejecting invalid physical slot ID: ", physicalSlot);
            response->set_status(commonStub::Status::FAILED);
            response->set_error(commonStub::ErrorCode::INVALID_ARGUMENTS);
            return grpc::Status::OK;
        }

        const std::string mode = readCardModeForLogicalSlot(entry.logical_slot_id());
        physicalSlotModes[physicalSlot].push_back(mode);
    }

    for (const auto &entry : physicalSlotModes) {
        if (entry.second.size() <= 1) {
            continue;
        }

        const bool validSharedSlot
            = entry.second.size() == 2
              && ((entry.second[0] == "MEP_A1" && entry.second[1] == "MEP_A1")
                  || (entry.second[0] == "MEP_B" && entry.second[1] == "MEP_B"));

        if (!validSharedSlot) {
            LOG(ERROR, __FUNCTION__, " Rejecting shared physical slot ", entry.first,
                ": unsupported mode combination");
            response->set_status(commonStub::Status::FAILED);
            response->set_error(commonStub::ErrorCode::INVALID_ARGUMENTS);
            return grpc::Status::OK;
        }
    }

    for (const auto &entry : request->map_info()) {

        int logicalSlot     = entry.logical_slot_id();
        const auto &mapInfo = entry.map_info();
        int portId          = mapInfo.port_id();

        LOG(DEBUG, __FUNCTION__, " Entry: logicalSlot=", logicalSlot,
            " physicalSlot=", mapInfo.physical_slot(), " portId=", portId);

        // Validate the port using the mode of this logical slot.  For a
        // shared NONE+MEP physical slot, the NONE logical slot uses port 0
        // and the MEP logical slot uses its MEP-specific port range.
        std::string modeStr = readCardModeForLogicalSlot(logicalSlot);
        LOG(DEBUG, __FUNCTION__, " Card mode for logicalSlot=", logicalSlot, " is ", modeStr);

        if (!isPortAllowedForMode(modeStr, portId)) {
            LOG(ERROR, __FUNCTION__, " Rejecting mapping: logicalSlot=", logicalSlot,
                " mode=", modeStr, " portId=", portId);
            response->set_status(commonStub::Status::FAILED);
            response->set_error(commonStub::ErrorCode::INVALID_ARGUMENTS);
            return grpc::Status::OK;
        }

        Json::Value mapEntry(Json::objectValue);

        mapEntry["logicalSlotId"] = (logicalSlot == telStub::LogicalSlotId::LOGICAL_SLOT_ID_1)
                                        ? "SLOT_ID_1"
                                        : "SLOT_ID_2";

        mapEntry["physicalSlotId"]
            = (mapInfo.physical_slot() == telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_1)
                  ? "SLOT_ID_1"
                  : "SLOT_ID_2";

        mapEntry["portId"] = portId;

        LOG(DEBUG, __FUNCTION__, " Adding mapEntry logical=", mapEntry["logicalSlotId"].asString(),
            " physical=", mapEntry["physicalSlotId"].asString(), " portId=", portId);

        newMapInfoArray.append(mapEntry);
    }

    // -------- Detect mapping change --------
    const Json::Value oldMapInfoArray
        = rootObj["IMultiSimManager"]["logicalSlotMapping"]["mapInfo"];

    const bool mappingChanged
        = (!oldMapInfoArray.isArray())
          || (oldMapInfoArray.toStyledString() != newMapInfoArray.toStyledString());

    LOG(DEBUG, __FUNCTION__, " mappingChanged=", mappingChanged,
        " oldSize=", oldMapInfoArray.size(), " newSize=", newMapInfoArray.size());

    rootObj["IMultiSimManager"]["logicalSlotMapping"]            = Json::objectValue;
    rootObj["IMultiSimManager"]["logicalSlotMapping"]["mapInfo"] = newMapInfoArray;

    // -------- Recompute getPhysicalSlotStatus if changed --------
    if (mappingChanged) {

        LOG(INFO, __FUNCTION__, " Mapping changed, recomputing getPhysicalSlotStatus");

        auto slotStrToPhy = [](const std::string &s) -> int {
            if (s == "SLOT_ID_1")
                return 1;
            if (s == "SLOT_ID_2")
                return 2;
            return -1;
        };

        auto phyToSlotStr = [](int id) -> std::string {
            if (id == 1)
                return "SLOT_ID_1";
            if (id == 2)
                return "SLOT_ID_2";
            return "SLOT_ID_INVALID";
        };

        auto logicalStrToEnum = [](const std::string &s) -> int {
            if (s == "SLOT_ID_1")
                return static_cast<int>(telStub::LogicalSlotId::LOGICAL_SLOT_ID_1);
            if (s == "SLOT_ID_2")
                return static_cast<int>(telStub::LogicalSlotId::LOGICAL_SLOT_ID_2);
            return static_cast<int>(telStub::LogicalSlotId::LOGICAL_SLOT_ID_INVALID);
        };

        auto portsForMode = [](const std::string &mode) -> std::vector<int> {
            if (mode == "MEP_B")
                return {0, 1};
            if (mode == "MEP_A1")
                return {1, 2};
            return {};
        };

        // -------- Build physical->port->logical lookup --------
        std::map<int, std::map<int, std::string>> phyPortToLogical;

        for (Json::ArrayIndex i = 0; i < newMapInfoArray.size(); ++i) {
            const Json::Value &m = newMapInfoArray[i];

            std::string logicalStr  = m["logicalSlotId"].asString();
            std::string physicalStr = m["physicalSlotId"].asString();
            int portId              = m["portId"].asInt();

            int phy = slotStrToPhy(physicalStr);
            if (phy > 0) {
                phyPortToLogical[phy][portId] = logicalStr;
                LOG(DEBUG, __FUNCTION__, " Mapping phy=", phy, " port=", portId,
                    " logical=", logicalStr);
            }
        }

        // -------- Read ICCIDs from CardManager configuration --------
        // ICCID is a card/profile property.  Do not use the generated
        // MultiSim state JSON as its source; that file is populated below.
        Json::Value card1, card2;
        JsonParser::readFromJsonFile(card1, CARDMGR_JSON_SLOT1);
        JsonParser::readFromJsonFile(card2, CARDMGR_JSON_SLOT2);

        const std::string iccidL1 = card1["ICardManager"].get("ICCID", "").asString();
        const std::string iccidL2 = card2["ICardManager"].get("ICCID", "").asString();

        LOG(DEBUG, __FUNCTION__, " ICCID SLOT1=", iccidL1, " SLOT2=", iccidL2);

        auto iccidForLogical = [&](const std::string &logicalStr) -> std::string {
            if (logicalStr == "SLOT_ID_1")
                return iccidL1;
            if (logicalStr == "SLOT_ID_2")
                return iccidL2;
            return "";
        };

        // -------- Build slotStatus --------
        Json::Value newSlotStatusArr(Json::arrayValue);

        for (int phy = 1; phy <= MAX_SLOT_ID; ++phy) {

            bool phyInMapping = (phyPortToLogical.find(phy) != phyPortToLogical.end());

            LOG(DEBUG, __FUNCTION__, " Building status for physicalSlot=", phy,
                " inMapping=", phyInMapping);

            // Report only physical slots participating in the current
            // logical-slot mapping.  An unmapped physical slot must not be
            // represented as an inactive/unknown slot in the response.
            if (!phyInMapping) {
                continue;
            }

            Json::Value slotEntry(Json::objectValue);
            slotEntry["physicalSlotId"] = phyToSlotStr(phy);
            slotEntry["slotState"]      = phyInMapping ? "ACTIVE" : "INACTIVE";
            slotEntry["cardState"]      = phyInMapping ? "CARDSTATE_PRESENT" : "CARDSTATE_UNKNOWN";
            slotEntry["cardError"]      = "CARD_ERROR_NONE";

            std::string mode = "NONE";
            if (phyInMapping) {
                for (const auto &kv : phyPortToLogical[phy]) {
                    int logicalEnum = logicalStrToEnum(kv.second);
                    if (logicalEnum
                        != static_cast<int>(telStub::LogicalSlotId::LOGICAL_SLOT_ID_INVALID)) {
                        std::string m = readCardModeForLogicalSlot(logicalEnum);
                        if (mode == "NONE" || m != "NONE") {
                            mode = m;
                        }
                    }
                }
            }

            bool isMep = (mode == "MEP_A1" || mode == "MEP_B");
            LOG(DEBUG, __FUNCTION__, " physicalSlot=", phy, " mode=", mode, " isMep=", isMep);

            slotEntry["isMep"] = isMep;

            Json::Value mepInfo(Json::objectValue);
            mepInfo["mode"] = mode;

            Json::Value portArr(Json::arrayValue);

            if (isMep) {
                for (int portId : portsForMode(mode)) {

                    const bool portMapped
                        = phyInMapping && (phyPortToLogical[phy].count(portId) != 0);

                    if (!portMapped) {
                        continue;  // ✅ skip unmapped ports
                    }

                    Json::Value p(Json::objectValue);
                    p["state"] = "ACTIVE";

                    const std::string &logicalStr = phyPortToLogical[phy][portId];
                    p["slotId"]                   = logicalStr;

                    // ICCID is valid only for a logical slot whose
                    // CardManager configuration has a MEP mode.  A logical
                    // slot with mode NONE remains an active port but has no
                    // ICCID in the generated MultiSim state.
                    const int logicalEnum         = logicalStrToEnum(logicalStr);
                    const std::string logicalMode = readCardModeForLogicalSlot(logicalEnum);
                    if (logicalMode != "NONE") {
                        p["iccId"] = iccidForLogical(logicalStr);
                    }

                    LOG(DEBUG, __FUNCTION__, " phy=", phy, " port=", portId,
                        " mappedLogical=", logicalStr, " mode=", logicalMode,
                        " iccid=", p.get("iccId", "").asString());
                    portArr.append(p);
                }
            }

            mepInfo["port"]          = portArr;
            slotEntry["mepSlotInfo"] = mepInfo;

            newSlotStatusArr.append(slotEntry);
        }

        rootObj["IMultiSimManager"]["getPhysicalSlotStatus"]               = Json::objectValue;
        rootObj["IMultiSimManager"]["getPhysicalSlotStatus"]["slotStatus"] = newSlotStatusArr;
    }

    // -------- Write JSON --------
    LOG(INFO, __FUNCTION__, " Writing Slot1 JSON to ", JSON_PATH1);

    ec = JsonParser::writeToJsonFile(rootObj, JSON_PATH1);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to write JSON ", JSON_PATH1);
        response->set_status(commonStub::Status::FAILED);
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    jsonObjSystemStateSlot_[1] = rootObj;

    LOG(INFO, __FUNCTION__, " Updated logicalSlotMapping and getPhysicalSlotStatus successfully");

    if (mappingChanged) {
        notifySlotStatusChanged(rootObj["IMultiSimManager"]["getPhysicalSlotStatus"]["slotStatus"]);
    }

    response->set_error(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

void MultiSimManagerServerImpl::notifySlotStatusChanged(const Json::Value &slotStatusArray) {
    if (!slotStatusArray.isArray()) {
        LOG(WARNING, __FUNCTION__, " Slot status is not an array");
        return;
    }

    ::telStub::SlotStatusChangedEvent slotStatusChangedEvent;
    slotStatusChangedEvent.set_phone_id(DEFAULT_SLOT_ID);

    for (const auto &slotStatus : slotStatusArray) {
        const std::string physicalSlotStr
            = slotStatus.get("physicalSlotId", "SLOT_ID_INVALID").asString();

        telStub::PhysicalSlotId physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_INVALID;
        if (physicalSlotStr == "SLOT_ID_1") {
            physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_1;
        } else if (physicalSlotStr == "SLOT_ID_2") {
            physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_2;
        } else {
            continue;
        }

        auto *slotStatusEntry = slotStatusChangedEvent.add_slotstatus();
        slotStatusEntry->set_physical_slot(physicalSlotId);
        auto *status = slotStatusEntry->mutable_status();
        status->set_is_mep(slotStatus.get("isMep", false).asBool());

        const Json::Value &mepInfo = slotStatus["mepSlotInfo"];
        const std::string mode     = mepInfo.get("mode", "NONE").asString();
        if (mode == "MEP_A1") {
            status->mutable_mep_slot_info()->set_mode(telStub::MepMode::MEP_MODE_MEP_A1);
        } else if (mode == "MEP_B") {
            status->mutable_mep_slot_info()->set_mode(telStub::MepMode::MEP_MODE_MEP_B);
        } else {
            status->mutable_mep_slot_info()->set_mode(telStub::MepMode::MEP_MODE_NONE);
        }

        if (mepInfo["port"].isArray()) {
            for (const auto &port : mepInfo["port"]) {
                auto *portInfo          = status->mutable_mep_slot_info()->add_port();
                const std::string state = port.get("state", "UNKNOWN").asString();
                if (state == "ACTIVE") {
                    portInfo->set_state(telStub::PortState::PORT_STATE_ACTIVE);
                } else if (state == "INACTIVE") {
                    portInfo->set_state(telStub::PortState::PORT_STATE_INACTIVE);
                } else {
                    portInfo->set_state(telStub::PortState::PORT_STATE_UNKNOWN);
                }

                portInfo->set_icc_id(port.get("iccId", "").asString());
                const std::string logicalSlot = port.get("slotId", "SLOT_ID_INVALID").asString();
                if (logicalSlot == "SLOT_ID_1") {
                    portInfo->set_slot_id(telStub::LogicalSlotId::LOGICAL_SLOT_ID_1);
                } else if (logicalSlot == "SLOT_ID_2") {
                    portInfo->set_slot_id(telStub::LogicalSlotId::LOGICAL_SLOT_ID_2);
                } else {
                    portInfo->set_slot_id(telStub::LogicalSlotId::LOGICAL_SLOT_ID_INVALID);
                }
            }
        }

        const std::string slotState = slotStatus.get("slotState", "UNKNOWN").asString();
        if (slotState == "ACTIVE") {
            status->set_slot_state(telStub::SlotState::SLOT_STATE_ACTIVE);
        } else if (slotState == "INACTIVE") {
            status->set_slot_state(telStub::SlotState::SLOT_STATE_INACTIVE);
        } else {
            status->set_slot_state(telStub::SlotState::SLOT_STATE_UNKNOWN);
        }

        const std::string cardState = slotStatus.get("cardState", "CARDSTATE_UNKNOWN").asString();
        if (cardState == "CARDSTATE_ABSENT") {
            status->set_card_state(telStub::CardState::CARDSTATE_ABSENT);
        } else if (cardState == "CARDSTATE_PRESENT") {
            status->set_card_state(telStub::CardState::CARDSTATE_PRESENT);
        } else if (cardState == "CARDSTATE_ERROR") {
            status->set_card_state(telStub::CardState::CARDSTATE_ERROR);
        } else if (cardState == "CARDSTATE_RESTRICTED") {
            status->set_card_state(telStub::CardState::CARDSTATE_RESTRICTED);
        } else {
            status->set_card_state(telStub::CardState::CARDSTATE_UNKNOWN);
        }

        status->set_card_error(telStub::CardError::CARD_ERROR_NONE);
    }

    ::eventService::EventResponse notification;
    notification.set_filter(telux::tel::TEL_MULTISIM_FILTER);
    notification.mutable_any()->PackFrom(slotStatusChangedEvent);
    EventService::getInstance().updateEventQueue(notification);
    LOG(DEBUG, __FUNCTION__, " Published slot status change notification");
}

grpc::Status MultiSimManagerServerImpl::GetSimSlotStatus(ServerContext *context,
    const google::protobuf::Empty *request, ::telStub::GetSimSlotStatusResponse *response) {

    LOG(DEBUG, "%s: Enter", __FUNCTION__);

    std::string jsonfilename = "";
    Json::Value rootObj;

    grpc::Status readStatus = readJson();
    if (!readStatus.ok()) {
        LOG(ERROR, "%s: readJson failed", __FUNCTION__);
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    getJsonForSystemData(DEFAULT_SLOT_ID, jsonfilename, rootObj);

    // Synchronize the physical-slot state from the CardManager API files
    // before serving the response.  CardManagerSlotN describes logical slot
    // N; the logicalSlotMapping tells us which physical slot and MEP port it
    // belongs to.
    Json::Value card1, card2;
    if (JsonParser::readFromJsonFile(card1, CARDMGR_JSON_SLOT1) != telux::common::ErrorCode::SUCCESS
        || JsonParser::readFromJsonFile(card2, CARDMGR_JSON_SLOT2)
               != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, "%s: failed to read CardManager API JSON", __FUNCTION__);
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    const std::string cardMode[3]  = {"NONE", card1["ICardManager"].get("mode", "NONE").asString(),
         card2["ICardManager"].get("mode", "NONE").asString()};
    const std::string cardIccid[3] = {"", card1["ICardManager"].get("ICCID", "").asString(),
        card2["ICardManager"].get("ICCID", "").asString()};

    const auto logicalId = [](const std::string &value) -> int {
        if (value == "SLOT_ID_1")
            return 1;
        if (value == "SLOT_ID_2")
            return 2;
        return -1;
    };
    const auto physicalId = [](const std::string &value) -> int {
        if (value == "SLOT_ID_1")
            return 1;
        if (value == "SLOT_ID_2")
            return 2;
        return -1;
    };
    const auto slotName
        = [](int value) -> std::string { return value == 1 ? "SLOT_ID_1" : "SLOT_ID_2"; };
    const auto portAllowed = [](const std::string &mode, int port) {
        return mode == "MEP_A1"  ? (port == 1 || port == 2)
               : mode == "MEP_B" ? (port == 0 || port == 1)
                                 : port == 0;
    };

    // Build the logical-to-physical mapping from CardManager data.  The
    // logical slot number is the CardManager file number; physicalSlotId is
    // now explicitly carried by that file.  MEP ports are assigned in order
    // per physical card: A1 starts at 1, B starts at 0.
    Json::Value mapping(Json::arrayValue);
    int nextMepPort[3] = {0, 0, 0};
    for (int logical = 1; logical <= 2; ++logical) {
        const Json::Value &card    = logical == 1 ? card1 : card2;
        const std::string physical = card["ICardManager"].get("physicalSlotId", "").asString();
        const int physicalNumber   = physicalId(physical);
        if (physicalNumber < 1 || physicalNumber > MAX_SLOT_ID) {
            LOG(ERROR, "%s: invalid physicalSlotId for logical slot %d: %s", __FUNCTION__, logical,
                physical.c_str());
            response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
            return grpc::Status::OK;
        }

        const std::string &mode = cardMode[logical];
        const int port          = mode == "MEP_A1"  ? 1 + nextMepPort[physicalNumber]++
                                  : mode == "MEP_B" ? nextMepPort[physicalNumber]++
                                                    : 0;

        Json::Value mapEntry(Json::objectValue);
        mapEntry["logicalSlotId"]  = slotName(logical);
        mapEntry["physicalSlotId"] = physical;
        mapEntry["portId"]         = port;
        mapping.append(mapEntry);
    }

    rootObj["IMultiSimManager"]["logicalSlotMapping"]["mapInfo"] = mapping;
    Json::Value synchronized(Json::arrayValue);

    for (int physical = 1; physical <= MAX_SLOT_ID; ++physical) {
        Json::Value entry(Json::objectValue);
        entry["physicalSlotId"] = slotName(physical);

        bool physicalPresent = false;
        if (mapping.isArray()) {
            for (const auto &item : mapping) {
                if (physicalId(item.get("physicalSlotId", "").asString()) == physical) {
                    physicalPresent = true;
                    break;
                }
            }
        }

        entry["slotState"] = physicalPresent ? "ACTIVE" : "INACTIVE";
        entry["cardState"] = physicalPresent ? "CARDSTATE_PRESENT" : "CARDSTATE_ABSENT";
        entry["cardError"] = physicalPresent ? "CARD_ERROR_NONE" : "CARD_ERROR_UNKNOWN";

        std::string mode = "NONE";
        if (physicalPresent && mapping.isArray()) {
            for (const auto &item : mapping) {
                if (physicalId(item.get("physicalSlotId", "").asString()) != physical)
                    continue;
                const int logical = logicalId(item.get("logicalSlotId", "").asString());
                if (logical >= 1 && logical <= 2 && cardMode[logical] != "NONE") {
                    mode = cardMode[logical];
                    break;
                }
            }
        }

        const bool isMep = mode == "MEP_A1" || mode == "MEP_B";
        entry["isMep"]   = isMep;
        Json::Value mepInfo(Json::objectValue);
        mepInfo["mode"] = mode;
        Json::Value ports(Json::arrayValue);

        if (isMep && mapping.isArray()) {
            for (const auto &item : mapping) {
                if (physicalId(item.get("physicalSlotId", "").asString()) != physical)
                    continue;
                const int port    = item.get("portId", -1).asInt();
                const int logical = logicalId(item.get("logicalSlotId", "").asString());
                if (logical < 1 || logical > 2 || !portAllowed(mode, port)
                    || cardMode[logical] == "NONE")
                    continue;

                Json::Value portInfo(Json::objectValue);
                portInfo["state"]  = "ACTIVE";
                portInfo["slotId"] = slotName(logical);
                portInfo["iccId"]  = cardIccid[logical];
                ports.append(portInfo);
            }
        }

        mepInfo["port"]      = ports;
        entry["mepSlotInfo"] = mepInfo;
        synchronized.append(entry);
    }

    rootObj["IMultiSimManager"]["getPhysicalSlotStatus"]["slotStatus"] = synchronized;
    if (JsonParser::writeToJsonFile(rootObj, JSON_PATH1) != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, "%s: failed to write synchronized MultiSim state", __FUNCTION__);
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }
    jsonObjSystemStateSlot_[DEFAULT_SLOT_ID] = rootObj;

    LOG(DEBUG, "Root JSON:\n%s", rootObj.toStyledString().c_str());

    Json::Value slotStatusArray
        = rootObj["IMultiSimManager"]["getPhysicalSlotStatus"]["slotStatus"];

    if (slotStatusArray.isNull() || !slotStatusArray.isArray()) {
        LOG(WARNING, "slotStatus missing or not array. Value:\n%s",
            slotStatusArray.toStyledString().c_str());
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    LOG(DEBUG, "slotStatusArray size = %u", slotStatusArray.size());

    // The state JSON contains the physical/port status, while CardManager
    // JSON is the source of truth for MEP profile ICCIDs.
    const std::string iccidL1 = card1["ICardManager"].get("ICCID", "").asString();
    const std::string iccidL2 = card2["ICardManager"].get("ICCID", "").asString();

    auto iccidForLogical = [&](const std::string &logicalSlot) -> std::string {
        if (logicalSlot == "SLOT_ID_1")
            return iccidL1;
        if (logicalSlot == "SLOT_ID_2")
            return iccidL2;
        return "";
    };

    response->clear_slotstatus();

    for (Json::ArrayIndex i = 0; i < slotStatusArray.size(); ++i) {
        const Json::Value &slotStatus = slotStatusArray[i];

        LOG(DEBUG, "================ SLOT INDEX %u ================", i);
        LOG(DEBUG, "Raw slotStatus JSON:\n%s", slotStatus.toStyledString().c_str());

        auto *slotStatusProto                 = response->add_slotstatus();
        telStub::SimSlotStatus *simSlotStatus = slotStatusProto->mutable_status();

        /* ---------- Physical Slot ---------- */
        std::string physicalSlotStr
            = slotStatus.get("physicalSlotId", "SLOT_ID_INVALID").asString();

        LOG(DEBUG, "physicalSlotId (raw) = '%s'", physicalSlotStr.c_str());

        telStub::PhysicalSlotId physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_INVALID;

        if (physicalSlotStr == "SLOT_ID_1") {
            physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_1;
        } else if (physicalSlotStr == "SLOT_ID_2") {
            physicalSlotId = telStub::PhysicalSlotId::PHYSICAL_SLOT_ID_2;
        }

        slotStatusProto->set_physical_slot(physicalSlotId);
        LOG(DEBUG, "physicalSlotId (mapped enum) = %d", physicalSlotId);

        /* ---------- MEP ---------- */
        const Json::Value &mepInfo = slotStatus["mepSlotInfo"];
        std::string modeStr        = "NONE";

        // CardManager mode is authoritative.  Do not trust a stale
        // isMep/mode value left in the generated MultiSim state JSON.
        const Json::Value &statePorts = mepInfo["port"];
        if (statePorts.isArray()) {
            for (Json::ArrayIndex j = 0; j < statePorts.size(); ++j) {
                const std::string logicalSlot
                    = statePorts[j].get("slotId", "SLOT_ID_INVALID").asString();
                const int logicalSlotId
                    = logicalSlot == "SLOT_ID_1"
                          ? static_cast<int>(telStub::LogicalSlotId::LOGICAL_SLOT_ID_1)
                      : logicalSlot == "SLOT_ID_2"
                          ? static_cast<int>(telStub::LogicalSlotId::LOGICAL_SLOT_ID_2)
                          : static_cast<int>(telStub::LogicalSlotId::LOGICAL_SLOT_ID_INVALID);
                const std::string logicalMode = readCardModeForLogicalSlot(logicalSlotId);
                if (logicalMode != "NONE") {
                    modeStr = logicalMode;
                    break;
                }
            }
        }

        const bool isMep = (modeStr == "MEP_A1" || modeStr == "MEP_B");
        LOG(DEBUG, "isMep (CardManager) = %s", isMep ? "true" : "false");
        simSlotStatus->set_is_mep(isMep);

        /* ---------- ALWAYS create mepSlotInfo ---------- */
        auto *mepSlotInfo = simSlotStatus->mutable_mep_slot_info();

        if (isMep) {

            LOG(DEBUG, "mepSlotInfo JSON:\n%s", mepInfo.toStyledString().c_str());

            LOG(DEBUG, "MEP mode (CardManager) = '%s'", modeStr.c_str());

            if (modeStr == "MEP_A1") {
                mepSlotInfo->set_mode(telStub::MepMode::MEP_MODE_MEP_A1);
            } else if (modeStr == "MEP_B") {
                mepSlotInfo->set_mode(telStub::MepMode::MEP_MODE_MEP_B);
            } else {
                mepSlotInfo->set_mode(telStub::MepMode::MEP_MODE_NONE);
            }

            LOG(DEBUG, "MEP mode (mapped enum) = %d", mepSlotInfo->mode());

            /* ---------- Ports ---------- */
            const Json::Value &ports = mepInfo["port"];
            LOG(DEBUG, "ports isArray=%d size=%u", ports.isArray(),
                ports.isArray() ? ports.size() : 0);

            if (ports.isArray()) {
                for (Json::ArrayIndex j = 0; j < ports.size(); ++j) {
                    const Json::Value &port = ports[j];
                    auto *portInfo          = mepSlotInfo->add_port();

                    LOG(DEBUG, "---- Port index %u ----", j);
                    LOG(DEBUG, "Port JSON:\n%s", port.toStyledString().c_str());

                    std::string stateStr = port.get("state", "UNKNOWN").asString();
                    LOG(DEBUG, "Port state (raw) = '%s'", stateStr.c_str());

                    if (stateStr == "ACTIVE") {
                        portInfo->set_state(telStub::PortState::PORT_STATE_ACTIVE);
                    } else if (stateStr == "INACTIVE") {
                        portInfo->set_state(telStub::PortState::PORT_STATE_INACTIVE);
                    } else {
                        portInfo->set_state(telStub::PortState::PORT_STATE_UNKNOWN);
                    }

                    LOG(DEBUG, "Port state (mapped enum) = %d", portInfo->state());

                    std::string logicalSlotStr = port.get("slotId", "SLOT_ID_INVALID").asString();

                    LOG(DEBUG, "logicalSlotId (raw) = '%s'", logicalSlotStr.c_str());

                    telStub::LogicalSlotId logicalSlotId
                        = telStub::LogicalSlotId::LOGICAL_SLOT_ID_INVALID;

                    if (logicalSlotStr == "SLOT_ID_1") {
                        logicalSlotId = telStub::LogicalSlotId::LOGICAL_SLOT_ID_1;
                    } else if (logicalSlotStr == "SLOT_ID_2") {
                        logicalSlotId = telStub::LogicalSlotId::LOGICAL_SLOT_ID_2;
                    }

                    portInfo->set_slot_id(logicalSlotId);
                    LOG(DEBUG, "logicalSlotId (mapped enum) = %d", logicalSlotId);

                    // A logical slot with mode NONE is an active port but
                    // has no MEP profile ICCID.  Only MEP logical slots get
                    // their ICCID from ICardManagerSlot*.json.
                    const std::string logicalMode = readCardModeForLogicalSlot(logicalSlotId);
                    const std::string iccId
                        = logicalMode != "NONE" ? iccidForLogical(logicalSlotStr) : "";
                    portInfo->set_icc_id(iccId);
                    LOG(DEBUG, "logicalSlotMode = '%s', iccId = '%s'", logicalMode.c_str(),
                        iccId.c_str());
                }
            }
        } else {
            /* Non‑MEP slot: explicit defaults */
            mepSlotInfo->set_mode(telStub::MepMode::MEP_MODE_NONE);
            /* ports intentionally left empty */
        }

        /* ---------- Slot State ---------- */
        std::string slotStateStr = slotStatus.get("slotState", "UNKNOWN").asString();
        LOG(DEBUG, "slotState (raw) = '%s'", slotStateStr.c_str());

        if (slotStateStr == "ACTIVE") {
            simSlotStatus->set_slot_state(telStub::SlotState::SLOT_STATE_ACTIVE);
        } else if (slotStateStr == "INACTIVE") {
            simSlotStatus->set_slot_state(telStub::SlotState::SLOT_STATE_INACTIVE);
        } else {
            simSlotStatus->set_slot_state(telStub::SlotState::SLOT_STATE_UNKNOWN);
        }

        LOG(DEBUG, "slotState (mapped enum) = %d", simSlotStatus->slot_state());

        /* ---------- Card State ---------- */
        std::string cardStateStr = slotStatus.get("cardState", "CARDSTATE_UNKNOWN").asString();
        LOG(DEBUG, "cardState (raw) = '%s'", cardStateStr.c_str());

        if (cardStateStr == "CARDSTATE_ABSENT") {
            simSlotStatus->set_card_state(telStub::CardState::CARDSTATE_ABSENT);
        } else if (cardStateStr == "CARDSTATE_PRESENT") {
            simSlotStatus->set_card_state(telStub::CardState::CARDSTATE_PRESENT);
        } else if (cardStateStr == "CARDSTATE_ERROR") {
            simSlotStatus->set_card_state(telStub::CardState::CARDSTATE_ERROR);
        } else if (cardStateStr == "CARDSTATE_RESTRICTED") {
            simSlotStatus->set_card_state(telStub::CardState::CARDSTATE_RESTRICTED);
        } else {
            simSlotStatus->set_card_state(telStub::CardState::CARDSTATE_UNKNOWN);
        }

        /* ---------- Card Error ---------- */
        std::string cardErrorStr = slotStatus.get("cardError", "CARD_ERROR_NONE").asString();
        LOG(DEBUG, "cardError (raw) = '%s'", cardErrorStr.c_str());

        if (cardErrorStr == "CARD_ERROR_GENERIC") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_GENERIC);
        } else if (cardErrorStr == "CARD_ERROR_NO_SIM") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_NO_SIM);
        } else if (cardErrorStr == "CARD_ERROR_SIM_NOT_READY") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_SIM_NOT_READY);
        } else if (cardErrorStr == "CARD_ERROR_SIM_LOCKED") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_SIM_LOCKED);
        } else if (cardErrorStr == "CARD_ERROR_COMMUNICATION_FAILURE") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_COMMUNICATION_FAILURE);
        } else if (cardErrorStr == "CARD_ERROR_POWER_DOWN") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_POWER_DOWN);
        } else if (cardErrorStr == "CARD_ERROR_POLL_ERROR") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_POLL_ERROR);
        } else if (cardErrorStr == "CARD_ERROR_NO_ATR_RECEIVED") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_NO_ATR_RECEIVED);
        } else if (cardErrorStr == "CARD_ERROR_VOLT_MISMATCH") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_VOLT_MISMATCH);
        } else if (cardErrorStr == "CARD_ERROR_PARITY_ERROR") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_PARITY_ERROR);
        } else if (cardErrorStr == "CARD_ERROR_POSSIBLY_REMOVED") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_POSSIBLY_REMOVED);
        } else if (cardErrorStr == "CARD_ERROR_TECHNICAL_PROBLEMS") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_TECHNICAL_PROBLEMS);
        } else if (cardErrorStr == "CARD_ERROR_NULL_BYTES") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_NULL_BYTES);
        } else if (cardErrorStr == "CARD_ERROR_SAP_CONNECTED") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_SAP_CONNECTED);
        } else if (cardErrorStr == "CARD_ERROR_CMD_TIMEOUT") {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_CMD_TIMEOUT);
        } else {
            simSlotStatus->set_card_error(telStub::CardError::CARD_ERROR_NONE);
        }

        LOG(DEBUG,
            "FINAL SLOT SUMMARY: physicalSlot=%d isMep=%d slotState=%d "
            "cardState=%d cardError=%d",
            physicalSlotId, simSlotStatus->is_mep(), simSlotStatus->slot_state(),
            simSlotStatus->card_state(), simSlotStatus->card_error());
    }

    response->set_error(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    LOG(DEBUG, "%s: Exit SUCCESS", __FUNCTION__);

    return grpc::Status::OK;
}

grpc::Status MultiSimManagerServerImpl::GetServiceStatus(ServerContext *context,
    const commonStub::GetServiceStatusRequest *request,
    commonStub::GetServiceStatusReply *response) {

    LOG(DEBUG, __FUNCTION__);
    int phoneId              = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;

    // Default to SERVICE_AVAILABLE if JSON not found
    telux::common::ServiceStatus status = telux::common::ServiceStatus::SERVICE_AVAILABLE;

    grpc::Status readStatus = readJson();
    if (readStatus.ok()) {
        getJsonForSystemData(phoneId, jsonfilename, rootObj);

        // Check for service status in JSON
        std::string statusStr
            = rootObj["IMultiSimManager"].get("getServiceStatus", "SERVICE_AVAILABLE").asString();

        if (statusStr == "SERVICE_UNAVAILABLE") {
            status = telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
        } else if (statusStr == "SERVICE_FAILED") {
            status = telux::common::ServiceStatus::SERVICE_FAILED;
        }
    }

    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_MULTISIM_FILTER};
        auto &serverEventManager         = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }

    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

void MultiSimManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event) {
    if (event.filter() == telux::tel::TEL_MULTISIM_FILTER) {
        onEventUpdate(event.event());
    }
}

bool MultiSimManagerServerImpl::handleSlotStatusChanged(
    std::string eventParams, ::eventService::EventResponse &notification) {
    LOG(DEBUG, __FUNCTION__, " Handling slot status changed event");

    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    int slotId        = 1;  // Default slot ID

    if (!token.empty()) {
        try {
            slotId = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Slot ID: ", slotId);
        } catch (...) {
            LOG(ERROR, __FUNCTION__, " Invalid slot ID format");
            return false;
        }
    }

    // Parse isMep
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token.empty()) {
        LOG(ERROR, __FUNCTION__, " Missing isMep parameter");
        return false;
    }

    bool isMep = false;
    try {
        isMep = (std::stoi(token) != 0);
    } catch (...) {
        LOG(ERROR, __FUNCTION__, " Invalid isMep format");
        return false;
    }

    // Parse MEP-specific parameters if isMep is true
    telux::tel::Mode mepMode = telux::tel::Mode::NONE;
    std::vector<telux::tel::PortInfo> ports;

    if (isMep) {
        // Parse MEP mode
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        if (token.empty()) {
            LOG(ERROR, __FUNCTION__, " Missing MEP mode parameter");
            return false;
        }

        try {
            mepMode = static_cast<telux::tel::Mode>(std::stoi(token));
        } catch (...) {
            LOG(ERROR, __FUNCTION__, " Invalid MEP mode format");
            return false;
        }

        // Parse port entries. The final three tokens are always
        // slotState, cardState, and cardError. Reserve them so they are not
        // accidentally consumed as a third port.
        auto remainingTokenCount = [](const std::string &input) {
            std::istringstream stream(input);
            return static_cast<size_t>(std::distance(
                std::istream_iterator<std::string>{stream}, std::istream_iterator<std::string>{}));
        };

        while (remainingTokenCount(eventParams) > 3) {
            // Check for port state
            token         = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
            int portState = 0;
            try {
                portState = std::stoi(token);
            } catch (...) {
                LOG(ERROR, __FUNCTION__, " Invalid port state format");
                return false;
            }

            // Parse ICC ID
            std::string iccId = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
            if (iccId.empty()) {
                LOG(ERROR, __FUNCTION__, " Missing ICC ID for port");
                return false;
            }

            // Parse logical slot ID
            token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
            if (token.empty()) {
                LOG(ERROR, __FUNCTION__, " Missing logical slot ID for port");
                return false;
            }

            int logicalSlotId;
            try {
                logicalSlotId = std::stoi(token);
            } catch (...) {
                LOG(ERROR, __FUNCTION__, " Invalid logical slot ID format");
                return false;
            }

            // Add port info
            telux::tel::PortInfo portInfo;
            portInfo.state  = static_cast<telux::tel::PortState>(portState);
            portInfo.iccId  = iccId;
            portInfo.slotId = static_cast<LogicalSlotId>(logicalSlotId);
            ports.push_back(portInfo);
        }
    }

    // Parse non-MEP parameters (slotState, cardState, cardError)
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token.empty()) {
        LOG(ERROR, __FUNCTION__, " Missing slotState parameter");
        return false;
    }

    int slotState;
    try {
        slotState = std::stoi(token);
    } catch (...) {
        LOG(ERROR, __FUNCTION__, " Invalid slotState format");
        return false;
    }

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token.empty()) {
        LOG(ERROR, __FUNCTION__, " Missing cardState parameter");
        return false;
    }

    int cardState;
    try {
        cardState = std::stoi(token);
    } catch (...) {
        LOG(ERROR, __FUNCTION__, " Invalid cardState format");
        return false;
    }

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token.empty()) {
        LOG(ERROR, __FUNCTION__, " Missing cardError parameter");
        return false;
    }

    int cardError;
    try {
        cardError = std::stoi(token);
    } catch (...) {
        LOG(ERROR, __FUNCTION__, " Invalid cardError format");
        return false;
    }

    // Update JSON with new slot status
    std::string jsonfilename = "";
    Json::Value rootObj;
    getJsonForSystemData(slotId, jsonfilename, rootObj);

    // Update slot status in JSON
    Json::Value &slotStatusArray
        = rootObj["IMultiSimManager"]["getPhysicalSlotStatus"]["slotStatus"];
    if (slotStatusArray.isNull() || !slotStatusArray.isArray()) {
        slotStatusArray = Json::arrayValue;
    }

    // For simplicity, assume we're updating the first slot entry or adding a new one
    Json::Value::ArrayIndex index = 0;
    for (; index < slotStatusArray.size(); ++index) {
        Json::Value &entry = slotStatusArray[index];
        if (entry["physicalSlotId"].asString() == "SLOT_ID_" + std::to_string(slotId)) {
            break;
        }
    }

    if (index >= slotStatusArray.size()) {
        slotStatusArray.resize(index + 1);
    }

    Json::Value &slotStatus = slotStatusArray[index];

    // Set physical slot ID
    std::string physicalSlotStr  = "SLOT_ID_" + std::to_string(slotId);
    slotStatus["physicalSlotId"] = physicalSlotStr;

    // Set MEP flag
    slotStatus["isMep"] = isMep;

    if (isMep) {
        Json::Value mepSlotInfo(Json::objectValue);

        // Set MEP mode
        std::string modeStr = "NONE";
        switch (mepMode) {
            case telux::tel::Mode::MEP_A1:
                modeStr = "MEP_A1";
                break;
            case telux::tel::Mode::MEP_B:
                modeStr = "MEP_B";
                break;
            default:
                modeStr = "NONE";
                break;
        }
        mepSlotInfo["mode"] = modeStr;

        // Set port info
        Json::Value portsJson(Json::arrayValue);

        for (const auto &port : ports) {
            Json::Value portJson(Json::objectValue);

            // Port state
            std::string stateStr = "UNKNOWN";
            switch (port.state) {
                case telux::tel::PortState::ACTIVE:
                    stateStr = "ACTIVE";
                    break;
                case telux::tel::PortState::INACTIVE:
                    stateStr = "INACTIVE";
                    break;
                default:
                    stateStr = "UNKNOWN";
                    break;
            }
            portJson["state"] = stateStr;

            // ICC ID
            portJson["iccId"] = port.iccId;

            // Logical slot ID
            std::string slotIdStr = "SLOT_ID_" + std::to_string(static_cast<int>(port.slotId));
            portJson["slotId"]    = slotIdStr;

            // ✅ Correct way to add array element
            portsJson.append(portJson);
        }

        mepSlotInfo["port"]       = portsJson;
        slotStatus["mepSlotInfo"] = mepSlotInfo;
    }
    // Set slot state
    std::string slotStateStr = "UNKNOWN";
    switch (static_cast<telux::tel::SlotState>(slotState)) {
        case telux::tel::SlotState::ACTIVE:
            slotStateStr = "ACTIVE";
            break;
        case telux::tel::SlotState::INACTIVE:
            slotStateStr = "INACTIVE";
            break;
        default:
            slotStateStr = "UNKNOWN";
            break;
    }
    slotStatus["slotState"] = slotStateStr;

    // Set card state
    std::string cardStateStr = "CARDSTATE_UNKNOWN";
    switch (static_cast<telux::tel::CardState>(cardState)) {
        case telux::tel::CardState::CARDSTATE_ABSENT:
            cardStateStr = "CARDSTATE_ABSENT";
            break;
        case telux::tel::CardState::CARDSTATE_PRESENT:
            cardStateStr = "CARDSTATE_PRESENT";
            break;
        case telux::tel::CardState::CARDSTATE_ERROR:
            cardStateStr = "CARDSTATE_ERROR";
            break;
        case telux::tel::CardState::CARDSTATE_RESTRICTED:
            cardStateStr = "CARDSTATE_RESTRICTED";
            break;
        default:
            cardStateStr = "CARDSTATE_UNKNOWN";
            break;
    }
    slotStatus["cardState"] = cardStateStr;

    // Set card error
    std::string cardErrorStr = "CARD_ERROR_UNKNOWN";

    switch (static_cast<telux::tel::CardError>(cardError)) {
        case telux::tel::CardError::UNKNOWN:
            cardErrorStr = "CARD_ERROR_UNKNOWN";
            break;

        case telux::tel::CardError::POWER_DOWN:
            cardErrorStr = "CARD_ERROR_POWER_DOWN";
            break;

        case telux::tel::CardError::POLL_ERROR:
            cardErrorStr = "CARD_ERROR_POLL_ERROR";
            break;

        case telux::tel::CardError::NO_ATR_RECEIVED:
            cardErrorStr = "CARD_ERROR_NO_ATR_RECEIVED";
            break;

        case telux::tel::CardError::VOLT_MISMATCH:
            cardErrorStr = "CARD_ERROR_VOLT_MISMATCH";
            break;

        case telux::tel::CardError::PARITY_ERROR:
            cardErrorStr = "CARD_ERROR_PARITY_ERROR";
            break;

        case telux::tel::CardError::POSSIBLY_REMOVED:
            cardErrorStr = "CARD_ERROR_POSSIBLY_REMOVED";
            break;

        case telux::tel::CardError::TECHNICAL_PROBLEMS:
            cardErrorStr = "CARD_ERROR_TECHNICAL_PROBLEMS";
            break;

        case telux::tel::CardError::NULL_BYTES:
            cardErrorStr = "CARD_ERROR_NULL_BYTES";
            break;

        case telux::tel::CardError::SAP_CONNECTED:
            cardErrorStr = "CARD_ERROR_SAP_CONNECTED";
            break;

        case telux::tel::CardError::CMD_TIMEOUT:
            cardErrorStr = "CARD_ERROR_CMD_TIMEOUT";
            break;

        default:
            cardErrorStr = "CARD_ERROR_UNKNOWN";
            break;
    }
    slotStatus["cardError"] = cardErrorStr;

    // Keep CardManager configuration synchronized with slot-status events.
    // ConfigureLogicalSlotMapping() and GetSimSlotStatus() use these files as
    // the source of truth for MEP mode and profile ICCID.
    auto modeToString = [](telux::tel::Mode mode) -> std::string {
        switch (mode) {
            case telux::tel::Mode::MEP_A1:
                return "MEP_A1";
            case telux::tel::Mode::MEP_B:
                return "MEP_B";
            default:
                return "NONE";
        }
    };

    auto updateCardManager = [&](int logicalSlotId, const std::string &mode,
                                 const std::string &iccId) {
        const std::string cardPath = getCardMgrJsonPathForLogicalSlot(logicalSlotId);
        if (cardPath.empty()) {
            return;
        }

        Json::Value cardRoot;
        if (JsonParser::readFromJsonFile(cardRoot, cardPath) != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to read CardManager JSON: ", cardPath);
            return;
        }

        cardRoot["ICardManager"]["mode"]  = mode;
        cardRoot["ICardManager"]["ICCID"] = mode == "NONE" ? "" : iccId;
        JsonParser::writeToJsonFile(cardRoot, cardPath);

        LOG(DEBUG, __FUNCTION__, " Updated CardManager logicalSlot=", logicalSlotId, " mode=", mode,
            " iccid=", cardRoot["ICardManager"]["ICCID"].asString());
    };

    const std::string eventMode = isMep ? modeToString(mepMode) : "NONE";
    if (isMep) {
        for (const auto &port : ports) {
            updateCardManager(static_cast<int>(port.slotId), eventMode, port.iccId);
        }
    } else {
        // A non-MEP event has no logical-slot fields.  Use the current mapping
        // to reset the logical slots assigned to this physical slot.
        const Json::Value &mapInfo = rootObj["IMultiSimManager"]["logicalSlotMapping"]["mapInfo"];
        if (mapInfo.isArray()) {
            for (const auto &mapping : mapInfo) {
                if (mapping.get("physicalSlotId", "") == physicalSlotStr) {
                    const std::string logicalSlotStr = mapping.get("logicalSlotId", "").asString();
                    const int logicalSlotId          = logicalSlotStr == "SLOT_ID_1"   ? 1
                                                       : logicalSlotStr == "SLOT_ID_2" ? 2
                                                                                       : -1;
                    updateCardManager(logicalSlotId, "NONE", "");
                }
            }
        }
    }

    // Write updated JSON
    JsonParser::writeToJsonFile(rootObj, jsonfilename);
    jsonObjSystemStateSlot_[slotId] = rootObj;

    // Create the event message
    ::telStub::SlotStatusChangedEvent slotStatusChangedEvent;
    slotStatusChangedEvent.set_phone_id(slotId);

    // Add slot status
    auto *slotStatusEntry = slotStatusChangedEvent.add_slotstatus();
    slotStatusEntry->set_physical_slot(static_cast<telStub::PhysicalSlotId>(slotId));

    auto *simSlotStatus = slotStatusEntry->mutable_status();
    simSlotStatus->set_is_mep(isMep);

    if (isMep) {
        auto *mepSlotInfo = simSlotStatus->mutable_mep_slot_info();
        mepSlotInfo->set_mode(static_cast<telStub::MepMode>(mepMode));

        for (size_t i = 0; i < ports.size(); ++i) {
            auto *portInfo = mepSlotInfo->add_port();
            portInfo->set_state(static_cast<telStub::PortState>(ports[i].state));
            portInfo->set_icc_id(ports[i].iccId);
            portInfo->set_slot_id(static_cast<telStub::LogicalSlotId>(ports[i].slotId));
        }
    }

    simSlotStatus->set_slot_state(static_cast<telStub::SlotState>(slotState));
    simSlotStatus->set_card_state(static_cast<telStub::CardState>(cardState));
    simSlotStatus->set_card_error(static_cast<telStub::CardError>(cardError));

    // Pack the event into the notification
    notification.mutable_any()->PackFrom(slotStatusChangedEvent);

    return true;
}

void MultiSimManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, " String is ", event);
    LOG(WARNING, __FUNCTION__,
        " Ignoring injected MultiSim event; slot status events are generated "
        "by ConfigureLogicalSlotMapping");
}
