/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "DualDataServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "common/event-manager/EventParserUtil.hpp"
#include "event/EventService.hpp"

#define DUAL_DATA_MANAGER_API_JSON "api/data/IDualDataManager.json"
#define DUAL_DATA_MANAGER_STATE_JSON "system-state/data/IDualDataManagerState.json"

#define DUAL_DATA_FILTER "dual_data"
#define CAPABILITY_CHANGE_EVENT "capabilityChange"
#define RECOMMENDATION_CHANGE_EVENT "recommendationChange"
#define DEFAULT_DELIMITER " "
#define COMMA_DELIMITER ","
#define DDSSWITCHRECOMMENDATION_EVENT "ddsSwitchRecommendation"
#define REMOTE 1
#define PERM "PERMANENT"
#define TEMP "TEMPORARY"

DualDataServerImpl::DualDataServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

DualDataServerImpl::~DualDataServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status DualDataServerImpl::InitService(ServerContext *context,
    const dataStub::InitRequest *request, dataStub::GetServiceStatusReply *response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath           = DUAL_DATA_MANAGER_API_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay          = rootObj["IDualDataManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus = rootObj["IDualDataManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {DUAL_DATA_FILTER};
        auto &serverEventManager         = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status DualDataServerImpl::GetDualDataCapability(ServerContext *context,
    const ::google::protobuf::Empty *request, dataStub::GetDualDataCapabilityReply *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem     = "IDualDataManager";
    std::string method        = "getDualDataCapability";
    JsonData data;
    telux::common::ErrorCode error
        = CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Json read failed");
        response->set_capability(false);  // Default to false if JSON read fails
        response->set_error(
            static_cast<commonStub::ErrorCode>(telux::common::ErrorCode::INTERNAL_ERROR));
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {

        if (data.stateRootObj[subsystem].isMember("dppdCapability")
            && data.stateRootObj[subsystem]["dppdCapability"].isBool()) {
            bool capability = data.stateRootObj[subsystem]["dppdCapability"].asBool();
            response->set_capability(capability);
            LOG(DEBUG, __FUNCTION__, " Dual data capability: ", capability ? "true" : "false");
        } else {
            response->set_capability(false);
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " Error reading dual data capability, defaulting to false");
        response->set_capability(false);
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status DualDataServerImpl::GetDualDataUsageRecommendation(ServerContext *context,
    const ::google::protobuf::Empty *request,
    dataStub::GetDualDataUsageRecommendationReply *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem     = "IDualDataManager";
    std::string method        = "getDualDataUsageRecommendation";
    JsonData data;
    telux::common::ErrorCode error
        = CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        response->mutable_usage_recommendation()->set_recommendation(
            convertUsageRecommendationStringToEnum(
                data.stateRootObj[subsystem]["dppdUsageRecommendation"].asString()));
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

void DualDataServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == DUAL_DATA_FILTER) {
        onEventUpdate(message.event());
    }
}

void DualDataServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, "String is ", event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "String is ", token);
    if (CAPABILITY_CHANGE_EVENT == token) {
        handleCapabilityChangeRequest(event);
    } else if (RECOMMENDATION_CHANGE_EVENT == token) {
        handleRecommendationChnageRequest(event);
    } else if (DDSSWITCHRECOMMENDATION_EVENT == token) {
        handleDdsSwitchRecommendationChangeRequest(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

::dataStub::UsageRecommendation::Recommendation
    DualDataServerImpl::convertUsageRecommendationStringToEnum(std::string recommendation) {
    LOG(DEBUG, __FUNCTION__);
    if (recommendation == "ALLOWED") {
        return ::dataStub::UsageRecommendation::ALLOWED;
    } else if (recommendation == "NOT_ALLOWED") {
        return ::dataStub::UsageRecommendation::NOT_ALLOWED;
    } else if (recommendation == "NOT_RECOMMENDED") {
        return ::dataStub::UsageRecommendation::NOT_RECOMMENDED;
    }

    return ::dataStub::UsageRecommendation::ALLOWED;
}

void DualDataServerImpl::handleCapabilityChangeRequest(std::string event) {
    LOG(DEBUG, __FUNCTION__);

    bool capability   = true;
    std::string param = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    try {
        capability = (std::stoi(param) == 1) ? true : false;
    } catch (exception const &ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        return;
    }

    std::string apiJsonPath   = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem     = "IDualDataManager";
    std::string method        = "getDualDataCapability";
    JsonData data;
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj[subsystem]["dppdCapability"] = capability;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    ::dataStub::DualDataCapabilityEvent dualDataCapabilityEvent;
    ::eventService::EventResponse anyResponse;

    dualDataCapabilityEvent.set_capability(capability);

    anyResponse.set_filter(DUAL_DATA_FILTER);
    anyResponse.mutable_any()->PackFrom(dualDataCapabilityEvent);
    // posting the event to EventService event queue
    auto &eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void DualDataServerImpl::handleRecommendationChnageRequest(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    std::string recommendation = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);

    std::string apiJsonPath   = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem     = "IDualDataManager";
    std::string method        = "getDualDataUsageRecommendation";
    JsonData data;
    CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj[subsystem]["dppdUsageRecommendation"] = recommendation;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    ::dataStub::DualDataUsageRecommendationEvent dualDataRecommendationEvent;
    ::eventService::EventResponse anyResponse;

    dualDataRecommendationEvent.set_recommendation(recommendation);

    anyResponse.set_filter(DUAL_DATA_FILTER);
    anyResponse.mutable_any()->PackFrom(dualDataRecommendationEvent);
    // posting the event to EventService event queue
    auto &eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void DualDataServerImpl::handleDdsSwitchRecommendationChangeRequest(std::string event) {
    LOG(DEBUG, __FUNCTION__);

    std::string currentEventString = event;
    std::vector<std::string> tokens;

    std::string token;
    while (
        !(token = EventParserUtil::getNextToken(currentEventString, DEFAULT_DELIMITER)).empty()) {
        tokens.push_back(token);
    }

    LOG(DEBUG, __FUNCTION__, "Tokens size: ", tokens.size());
    for (const auto &t : tokens) {
        LOG(DEBUG, __FUNCTION__, "Token: ", t);
    }

    if (tokens.size() < 3 || tokens.size() > 4) {
        LOG(ERROR, __FUNCTION__, "Invalid number of tokens ", tokens.size());
        return;
    }

    JsonData data;
    telux::common::ErrorCode jsonReadError = CommonUtils::readJsonData(DUAL_DATA_MANAGER_API_JSON,
        DUAL_DATA_MANAGER_STATE_JSON, "IDualDataManager", "configureDdsSwitchRecommendation", data);

    if (jsonReadError != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Failed to read DDS switch recommendation config from JSON.");
        return;
    }

    bool enableTemporary
        = data.stateRootObj["IDualDataManager"]["ddsSwitchRecommendationConfig"]["enableTemporaryRe"
                                                                                 "commendations"]
              .asBool();
    bool enablePermanent
        = data.stateRootObj["IDualDataManager"]["ddsSwitchRecommendationConfig"]["enablePermanentRe"
                                                                                 "commendations"]
              .asBool();

    ::dataStub::DdsSwitchRecommendation ddsSwitchRecommendation;

    auto *ddsInfo    = ddsSwitchRecommendation.mutable_recommended_dds_info();
    auto *recDetails = ddsSwitchRecommendation.mutable_recommendation_details();

    tempCause_ = "";
    permCause_ = "";

    int slotId            = std::stoi(tokens[0]);
    std::string ddsType   = tokens[1];
    std::string causeCode = tokens[2];

    ddsInfo->set_slot_id(slotId);
    slot_id_ = slotId;

    if (ddsType == "PERMANENT") {
        if (!enablePermanent) {
            LOG(DEBUG, __FUNCTION__, "Permanent recommendations are disabled. Skipping event.");
            return;
        }
        if (tokens.size() != 3) {
            LOG(ERROR, __FUNCTION__, "Invalid token count ", tokens.size());
            return;
        }
        LOG(DEBUG, __FUNCTION__, "Handling permanent recommendation");
        permCause_ = causeCode;
        ddsInfo->set_dds_type(::dataStub::DdsInfo::PERMANENT);
        recDetails->set_perm_cause(convertPermanentCauseStringToEnum(permCause_));
        ddsTypeStr_ = "PERMANENT";
        tempType_   = "UNKNOWN";  // Not applicable for permanent
        tempCause_  = "";  // Not applicable for permanent

    } else if (ddsType == "TEMPORARY") {
        if (!enableTemporary) {
            LOG(DEBUG, __FUNCTION__, "Temporary recommendations are disabled. Skipping event.");
            return;
        }
        if (tokens.size() != 4) {
            LOG(ERROR, __FUNCTION__, "Invalid token count ", tokens.size());
            return;
        }
        LOG(DEBUG, __FUNCTION__, "Handling temporary recommendation");
        std::string recommendationType = tokens[3];
        tempCause_                     = causeCode;
        ddsInfo->set_dds_type(::dataStub::DdsInfo::TEMPORARY);
        recDetails->set_temp_type(convertTempTypeStringToEnum(recommendationType));
        recDetails->set_temp_cause(convertTemporaryCauseStringToEnum(tempCause_));
        ddsTypeStr_ = "TEMPORARY";
        tempType_   = recommendationType;
        permCause_  = "";

    } else {
        LOG(ERROR, __FUNCTION__, "Unknown ddsType: ", ddsType);
        return;
    }

    ::eventService::EventResponse anyResponse;
    anyResponse.set_filter(DUAL_DATA_FILTER);
    anyResponse.mutable_any()->PackFrom(ddsSwitchRecommendation);
    auto &eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

grpc::Status DualDataServerImpl::RequestCurrentDdsSwitch(ServerContext *context,
    const dataStub::CurrentDdsSwitchRequest *request,
    dataStub::CurrentDdsSwitchResponse *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem     = "IDualDataManager";
    std::string method        = "requestDdsSwitch";
    JsonData data;
    telux::common::ErrorCode error
        = CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS
        && data.error == telux::common::ErrorCode::SUCCESS) {
        response->set_current_switch(static_cast<int>(ddsInfo_.type));
        response->set_slot_id(ddsInfo_.slotId);
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

::dataStub::DdsInfo::DdsType DualDataServerImpl::convertDdsTypeStringToEnum(
    const std::string &ddsTypeStr) {
    if (ddsTypeStr == "PERMANENT")
        return ::dataStub::DdsInfo::PERMANENT;
    if (ddsTypeStr == "TEMPORARY")
        return ::dataStub::DdsInfo::TEMPORARY;
    return ::dataStub::DdsInfo::UNKNOWN;
}

grpc::Status DualDataServerImpl::ConfigureDdsSwitchRecommendation(grpc::ServerContext *context,
    const dataStub::ConfigureDdsSwitchRecommendationRequest *request,
    dataStub::ConfigureDdsSwitchRecommendationReply *response) {

    LOG(DEBUG, __FUNCTION__);

    bool enableTemporary = request->enable_temporary_recommendations();
    bool enablePermanent = request->enable_permanent_recommendations();

    JsonData data;
    CommonUtils::readJsonData(DUAL_DATA_MANAGER_API_JSON, DUAL_DATA_MANAGER_STATE_JSON,
        "IDualDataManager", "configureDdsSwitchRecommendation", data);

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj["IDualDataManager"]["ddsSwitchRecommendationConfig"]["enableTemporaryReco"
                                                                               "mmendations"]
            = enableTemporary;
        data.stateRootObj["IDualDataManager"]["ddsSwitchRecommendationConfig"]["enablePermanentReco"
                                                                               "mmendations"]
            = enablePermanent;
        JsonParser::writeToJsonFile(data.stateRootObj, DUAL_DATA_MANAGER_STATE_JSON);
    } else {
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    response->set_error(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

::dataStub::RecommendationDetails::TemporaryRecommendationType
    DualDataServerImpl::convertTempTypeStringToEnum(const std::string &tempTypeStr) {

    if (tempTypeStr == "REVOKE") {
        return ::dataStub::RecommendationDetails_TemporaryRecommendationType_REVOKE;
    }
    if (tempTypeStr == "LOW") {
        return ::dataStub::RecommendationDetails_TemporaryRecommendationType_LOW;
    }
    if (tempTypeStr == "HIGH") {
        return ::dataStub::RecommendationDetails_TemporaryRecommendationType_HIGH;
    }
    return ::dataStub::RecommendationDetails_TemporaryRecommendationType_UNKNOWN;
}

::dataStub::TemporaryRecommendationCauseCodes DualDataServerImpl::convertTemporaryCauseStringToEnum(
    const std::string &causeStr) {

    LOG(DEBUG, __FUNCTION__, "String is ", causeStr);
    std::string mutableCauseStr = causeStr;
    std::string token;
    ::dataStub::TemporaryRecommendationCauseCodes tempCause
        = ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_UNKNOWN;
    while (!(token = EventParserUtil::getNextToken(mutableCauseStr, COMMA_DELIMITER)).empty()) {
        if (token == "TEMP_CAUSE_CODE_UNKNOWN") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause | ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_UNKNOWN);
        } else if (token == "TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_DSDA_IMPOSSIBLE);
        } else if (token == "TEMP_CAUSE_CODE_DDS_INTERNET_UNAVAIL") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::
                    TEMP_CAUSE_CODE_DDS_INTERNET_UNAVAIL);
        } else if (token == "TEMP_CAUSE_CODE_TX_SHARING") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_TX_SHARING);
        } else if (token == "TEMP_CAUSE_CODE_CALL_STATUS_CHANGED") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_CALL_STATUS_CHANGED);
        } else if (token == "TEMP_CAUSE_CODE_ACTIVE_CALL_ON_DDS") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_ACTIVE_CALL_ON_DDS);
        } else if (token == "TEMP_CAUSE_CODE_TEMP_REC_DISABLED") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_TEMP_REC_DISABLED);
        } else if (token == "TEMP_CAUSE_CODE_NON_DDS_INTERNET_UNAVAIL") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::
                    TEMP_CAUSE_CODE_NON_DDS_INTERNET_UNAVAIL);
        } else if (token == "TEMP_CAUSE_CODE_DATA_OFF") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_DATA_OFF);
        } else if (token == "TEMP_CAUSE_CODE_EMERGENCY_CALL_ON_GOING") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::
                    TEMP_CAUSE_CODE_EMERGENCY_CALL_ON_GOING);
        } else if (token == "TEMP_CAUSE_CODE_DDS_SIM_REMOVED") {
            tempCause = static_cast<::dataStub::TemporaryRecommendationCauseCodes>(
                tempCause
                | ::dataStub::TemporaryRecommendationCauseCodes::TEMP_CAUSE_CODE_DDS_SIM_REMOVED);
        }
    }
    return tempCause;
}

::dataStub::PermanentRecommendationCauseCodes DualDataServerImpl::convertPermanentCauseStringToEnum(
    const std::string &causeStr) {

    LOG(DEBUG, __FUNCTION__, "String is ", causeStr);
    std::string mutableCauseStr = causeStr;
    std::string token;
    ::dataStub::PermanentRecommendationCauseCodes permCause
        = ::dataStub::PermanentRecommendationCauseCodes::PERM_CAUSE_CODE_UNKNOWN;
    while (!(token = EventParserUtil::getNextToken(mutableCauseStr, COMMA_DELIMITER)).empty()) {
        if (token == "PERM_CAUSE_CODE_UNKNOWN") {
            permCause = static_cast<::dataStub::PermanentRecommendationCauseCodes>(
                permCause | ::dataStub::PermanentRecommendationCauseCodes::PERM_CAUSE_CODE_UNKNOWN);
        } else if (token == "PERM_CAUSE_CODE_TEMP_CLEAN_UP") {
            permCause = static_cast<::dataStub::PermanentRecommendationCauseCodes>(
                permCause
                | ::dataStub::PermanentRecommendationCauseCodes::PERM_CAUSE_CODE_TEMP_CLEAN_UP);
        } else if (token == "PERM_CAUSE_CODE_DATA_SETTING_OFF") {
            permCause = static_cast<::dataStub::PermanentRecommendationCauseCodes>(
                permCause
                | ::dataStub::PermanentRecommendationCauseCodes::PERM_CAUSE_CODE_DATA_SETTING_OFF);
        } else if (token == "PERM_CAUSE_CODE_PS_INVALID") {
            permCause = static_cast<::dataStub::PermanentRecommendationCauseCodes>(
                permCause
                | ::dataStub::PermanentRecommendationCauseCodes::PERM_CAUSE_CODE_PS_INVALID);
        } else if (token == "PERM_CAUSE_CODE_INTERNET_NOT_AVAIL") {
            permCause = static_cast<::dataStub::PermanentRecommendationCauseCodes>(
                permCause
                | ::dataStub::PermanentRecommendationCauseCodes::PERM_CAUSE_CODE_INTERNET_NOT_AVAIL);
        }
    }
    return permCause;
}

grpc::Status DualDataServerImpl::GetDdsSwitchRecommendation(grpc::ServerContext *context,
    const ::google::protobuf::Empty *request, dataStub::GetDdsSwitchRecommendationReply *response) {

    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    telux::common::ErrorCode jsonReadError = CommonUtils::readJsonData(DUAL_DATA_MANAGER_API_JSON,
        DUAL_DATA_MANAGER_STATE_JSON, "IDualDataManager", "configureDdsSwitchRecommendation", data);

    if (jsonReadError != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, "Failed to read DDS switch recommendation config from JSON.");
        response->set_error(commonStub::ErrorCode::GENERIC_FAILURE);
        return grpc::Status::OK;
    }

    bool enableTemporary
        = data.stateRootObj["IDualDataManager"]["ddsSwitchRecommendationConfig"]["enableTemporaryRe"
                                                                                 "commendations"]
              .asBool();
    bool enablePermanent
        = data.stateRootObj["IDualDataManager"]["ddsSwitchRecommendationConfig"]["enablePermanentRe"
                                                                                 "commendations"]
              .asBool();

    if (!enableTemporary && !enablePermanent) {
        LOG(DEBUG, __FUNCTION__, "Both temporary and permanent recommendations are disabled");

        response->mutable_dds_switch_recommendation()->mutable_recommended_dds_info()->set_dds_type(
            ::dataStub::DdsInfo::UNKNOWN);
        response->set_error(commonStub::ErrorCode::OPERATION_NOT_ALLOWED);
        return grpc::Status::OK;
    }
    dataStub::DdsSwitchRecommendation recommendation;
    ::dataStub::DdsInfo::DdsType ddsType = convertDdsTypeStringToEnum(ddsTypeStr_);
    auto *info                           = recommendation.mutable_recommended_dds_info();
    info->set_slot_id(slot_id_);
    info->set_dds_type(ddsType);
    auto *details = recommendation.mutable_recommendation_details();

    if (ddsType == ::dataStub::DdsInfo::TEMPORARY) {
        auto tempType = convertTempTypeStringToEnum(tempType_);
        details->set_temp_type(tempType);
        auto tempCause = convertTemporaryCauseStringToEnum(tempCause_);
        LOG(DEBUG, __FUNCTION__, "Temp cause string: ", tempCause_);
        details->set_temp_cause(tempCause);
    }
    if (ddsType == ::dataStub::DdsInfo::PERMANENT) {
        auto permCause = convertPermanentCauseStringToEnum(permCause_);
        LOG(DEBUG, __FUNCTION__, "Perm cause string: ", permCause_);
        details->set_perm_cause(permCause);
    }

    response->mutable_dds_switch_recommendation()->CopyFrom(recommendation);
    response->set_error(commonStub::ErrorCode::ERROR_CODE_SUCCESS);
    return grpc::Status::OK;
}

grpc::Status DualDataServerImpl::SetDdsSwitch(ServerContext *context,
    const dataStub::SetDdsSwitchRequest *request, dataStub::DefaultReply *response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = DUAL_DATA_MANAGER_API_JSON;
    std::string stateJsonPath = DUAL_DATA_MANAGER_STATE_JSON;
    std::string subsystem     = "IDualDataManager";
    std::string method        = "requestDdsSwitch";
    JsonData data;
    telux::common::ErrorCode error
        = CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    } else if (!telux::common::DeviceConfig::isMultiSimSupported()) {
        data.error = telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
    } else if ((ddsInfo_.slotId == static_cast<SlotId>(request->slot_id()))
               && ((ddsInfo_.type == static_cast<telux::data::DdsType>(request->switch_type()))
                   || ((ddsInfo_.type == telux::data::DdsType::PERMANENT)
                       && (static_cast<telux::data::DdsType>(request->switch_type())
                           == telux::data::DdsType::TEMPORARY)))) {
        // If for a slot_id, the requested switch_type is same as existing switch_type or
        // switch_type is from PERMANENT to TEMPORARY, it is not allowed.
        data.error = telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
    }

    if (data.status == telux::common::Status::SUCCESS
        && data.error == telux::common::ErrorCode::SUCCESS) {

        ddsInfo_.type   = static_cast<telux::data::DdsType>(request->switch_type());
        ddsInfo_.slotId = static_cast<SlotId>(request->slot_id());

        // we are only updating json if it is PERM switch, since TEMP
        // switch is not persistent across reboots.
        if (request->switch_type() == 0) {
            data.stateRootObj[subsystem][method]["DdsType"]
                = (request->switch_type() == 0) ? PERM : TEMP;
            data.stateRootObj[subsystem][method]["SlotId"] = request->slot_id();
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}