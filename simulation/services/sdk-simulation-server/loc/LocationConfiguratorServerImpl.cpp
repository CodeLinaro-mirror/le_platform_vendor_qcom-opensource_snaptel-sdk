/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       LocationConfiguratorServerImpl.hpp
 *
 *
 */

#include <telux/loc/LocationDefines.hpp>
#include "LocationConfiguratorServerImpl.hpp"
#include "libs/common/SimulationConfigParser.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "event/EventService.hpp"

#define LOC_CONFIG_API_JSON "api/loc/ILocationConfigurator.json"
#define DEFAULT_DELIMITER " "

std::string LocationConfiguratorServerImpl::getNextEventValue(
    std::string &event, const std::string &fieldName, const std::string &defaultValue) {
    std::string token;
    size_t first = event.find_first_not_of(DEFAULT_DELIMITER);
    if (first != std::string::npos) {
        event.erase(0, first);
    }
    size_t position = event.find(DEFAULT_DELIMITER);
    if (position != std::string::npos) {
        token = event.substr(0, position);
        event.erase(0, position + 1);
    } else {
        token = event;
        event.clear();
    }
    if (token.empty()) {
        LOG(INFO, __FUNCTION__, "The ", fieldName, " is not passed");
        return defaultValue;
    }
    return token;
}

std::string LocationConfiguratorServerImpl::trimString(const std::string &value) {
    size_t first = value.find_first_not_of(DEFAULT_DELIMITER);
    if (first == std::string::npos) {
        return "";
    }
    size_t last = value.find_last_not_of(DEFAULT_DELIMITER);
    return value.substr(first, last - first + 1);
}

uint32_t LocationConfiguratorServerImpl::parseUint32(std::string value, uint32_t defaultValue) {
    try {
        return static_cast<uint32_t>(std::stoul(value, nullptr, 0));
    } catch (std::exception &ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        return defaultValue;
    }
}

uint64_t LocationConfiguratorServerImpl::parseUint64(std::string value, uint64_t defaultValue) {
    try {
        return static_cast<uint64_t>(std::stoull(value, nullptr, 0));
    } catch (std::exception &ex) {
        LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        return defaultValue;
    }
}

bool LocationConfiguratorServerImpl::addGalileoSvToMask(const std::string &token, uint64_t &mask) {
    try {
        size_t parsed = 0;
        uint32_t svId = static_cast<uint32_t>(std::stoul(token, &parsed, 0));
        if (parsed != token.size() || svId < telux::loc::GAL_OSNMA_FIRST_SV_ID
            || svId >= telux::loc::GAL_OSNMA_FIRST_SV_ID + telux::loc::GAL_OSNMA_SV_COUNT) {
            LOG(ERROR, __FUNCTION__, " Invalid Galileo SV ID: ", token);
            return false;
        }
        mask |= (1ULL << (svId - telux::loc::GAL_OSNMA_FIRST_SV_ID));
        return true;
    } catch (std::exception &ex) {
        LOG(ERROR, __FUNCTION__, " Invalid Galileo SV ID: ", token, " ", ex.what());
        return false;
    }
}

bool LocationConfiguratorServerImpl::parseGalileoSvList(const std::string &list, uint64_t &mask) {
    if (list.empty()) {
        return true;
    }

    std::string remaining = list;
    while (!remaining.empty()) {
        size_t position = remaining.find(",");
        std::string token;
        if (position != std::string::npos) {
            token = remaining.substr(0, position);
            remaining.erase(0, position + 1);
        } else {
            token = remaining;
            remaining.clear();
        }
        token = trimString(token);
        if (token.empty() || !addGalileoSvToMask(token, mask)) {
            return false;
        }
    }
    return true;
}

bool LocationConfiguratorServerImpl::parseGalileoSvListFields(
    const std::string &event, uint64_t &passedMask, uint64_t &failedMask) {
    std::string passedList;
    std::string failedList;
    size_t position = event.find(DEFAULT_DELIMITER);
    if (position != std::string::npos) {
        passedList = event.substr(0, position);
        failedList = event.substr(position + 1);
    } else {
        passedList = event;
    }
    passedList = trimString(passedList);
    failedList = trimString(failedList);

    if (failedList.find(DEFAULT_DELIMITER) != std::string::npos) {
        LOG(ERROR, __FUNCTION__, " Too many SV list fields passed");
        return false;
    }

    return parseGalileoSvList(passedList, passedMask) && parseGalileoSvList(failedList, failedMask);
}

LocationConfiguratorServerImpl::LocationConfiguratorServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

LocationConfiguratorServerImpl::~LocationConfiguratorServerImpl() {
    LOG(DEBUG, __FUNCTION__, " Destructing");
}

grpc::Status LocationConfiguratorServerImpl::InitService(ServerContext *context,
    const google::protobuf::Empty *request, locStub::GetServiceStatusReply *response) {
    LOG(DEBUG, __FUNCTION__);
    int cbDelay                                = 100;
    telux::common::ServiceStatus serviceStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    Json::Value rootNode;
    telux::common::ErrorCode errorCode
        = JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    if (errorCode == ErrorCode::SUCCESS) {
        cbDelay              = rootNode["ILocationConfigurator"]["IsSubsystemReadyDelay"].asInt();
        std::string cbStatus = rootNode["ILocationConfigurator"]["IsSubsystemReady"].asString();
        serviceStatus        = CommonUtils::mapServiceStatus(cbStatus);
    } else {
        LOG(ERROR, "Unable to read LocationConfigurator JSON");
    }
    response->set_service_status(static_cast<::commonStub::ServiceStatus>(serviceStatus));
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {"loc_config"};
        auto &serverEventManager         = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
        taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
    }
    response->set_delay(cbDelay);
    xtraEnabled_ = std::stoi(CommonUtils::readSystemDataValue(
        "loc/ILocationConfigurator", "1", {"ILocationConfigurator", "XtraEnablement", "enable"}));
    xtraConsent_ = std::stoi(CommonUtils::readSystemDataValue(
        "loc/ILocationConfigurator", "1", {"ILocationConfigurator", "XtraEnablement", "consent"}));
    xtraSet_     = std::stoi(CommonUtils::readSystemDataValue(
        "loc/ILocationConfigurator", "1", {"ILocationConfigurator", "XtraEnablement", "set"}));
    return grpc::Status::OK;
}

void LocationConfiguratorServerImpl::onEventUpdate(::eventService::UnsolicitedEvent event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.filter() == "loc_config") {
        onEventUpdate(event.event());
    }
}

void LocationConfiguratorServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__, event);
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    handleEvent(token, event);
}

void LocationConfiguratorServerImpl::handleEvent(std::string token, std::string event) {
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", event);
    if (token == "xtra_status") {
        handleXtraUpdateEvent(event);
    } else if (token == "constellation_update") {
        handleGnssConstellationUpdateEvent(event);
    } else if (token == "nav_msg_auth_status") {
        handleNavMsgAuthStatusEvent(event);
    }
}

void LocationConfiguratorServerImpl::handleXtraUpdateEvent(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    int validity      = 0;
    int dataStatus    = 0;
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, __FUNCTION__, "The validity is not passed");
    } else {
        try {
            validity = std::stoi(token);
        } catch (std::exception &ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, __FUNCTION__, "The dataStatus is not passed");
    } else {
        try {
            dataStatus = std::stoi(token);
        } catch (std::exception &ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", std::to_string(validity),
        {"ILocationConfigurator", "XtraParams", "xtraValidForHours"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
        std::to_string(dataStatus), {"ILocationConfigurator", "XtraParams", "xtraDataStatus"});
    auto f = std::async(std::launch::async, [this]() { this->triggerXtraStatusEvent(); }).share();
    taskQ_->add(f);
}

void LocationConfiguratorServerImpl::handleGnssConstellationUpdateEvent(std::string event) {
    std::string enabledMask = " ";
    std::string token       = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, __FUNCTION__, "The Mask is not passed");
        enabledMask = "0X1FFFFF";
    } else {
        try {
            enabledMask = token;
        } catch (std::exception &ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    CommonUtils::writeSystemDataValue<string>(
        "loc/ILocationConfigurator", enabledMask, {"ILocationConfigurator", "GnssSignalType"});
    auto f = std::async(std::launch::async, [this]() {
        this->triggerGnssConstellationUpdateEvent();
    }).share();
    taskQ_->add(f);
}

void LocationConfiguratorServerImpl::handleNavMsgAuthStatusEvent(std::string event) {
    LOG(DEBUG, __FUNCTION__);
    uint32_t validityMask   = parseUint32(getNextEventValue(event, "validityMask", "0"), 0);
    uint32_t featureEnabled = parseUint32(getNextEventValue(event, "featureEnabled", "0"), 0);
    uint32_t merkleTreeProvisioned
        = parseUint32(getNextEventValue(event, "merkleTreeProvisioned", "0"), 0);
    uint32_t publicKeyStatusMask
        = parseUint32(getNextEventValue(event, "publicKeyStatusMask", "0"), 0);
    uint32_t galWeek               = parseUint32(getNextEventValue(event, "galWeek", "0"), 0);
    uint32_t galSecOfWeek          = parseUint32(getNextEventValue(event, "galSecOfWeek", "0"), 0);
    uint64_t navDataAuthPassedMask = 0;
    uint64_t navDataAuthFailedMask = 0;
    if (!parseGalileoSvListFields(event, navDataAuthPassedMask, navDataAuthFailedMask)) {
        LOG(ERROR, __FUNCTION__, " Nav message auth status not updated");
        return;
    }

    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", "2",
        {"ILocationConfigurator", "NavMsgAuthStatus", "constellation"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
        std::to_string(validityMask),
        {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "validityMask"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
        std::to_string(featureEnabled),
        {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "featureEnabled"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
        std::to_string(merkleTreeProvisioned),
        {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "merkleTreeProvisioned"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
        std::to_string(publicKeyStatusMask),
        {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "publicKeyStatusMask"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", std::to_string(galWeek),
        {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "galWeek"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
        std::to_string(galSecOfWeek),
        {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "galSecOfWeek"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
        std::to_string(navDataAuthPassedMask),
        {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "navDataAuthPassedMask"});
    CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
        std::to_string(navDataAuthFailedMask),
        {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "navDataAuthFailedMask"});
}

void LocationConfiguratorServerImpl::triggerXtraStatusEvent() {
    LOG(DEBUG, __FUNCTION__);
    uint32_t dataStatus;
    if (xtraConsent_) {
        dataStatus = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "XtraParams", "xtraDataStatus"}));
    } else {
        // If Xtra consent is false, Data status is Unknown.
        dataStatus = 0;
    }
    uint32_t validHours = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator",
        "0", {"ILocationConfigurator", "XtraParams", "xtraValidForHours"}));
    LOG(DEBUG, __FUNCTION__, xtraEnabled_, dataStatus, validHours);
    std::lock_guard<std::mutex> lck(mtx_);
    ::locStub::XtraStatusEvent xtraEvent;
    ::eventService::EventResponse anyResponse;
    xtraEvent.set_enable(xtraEnabled_);
    xtraEvent.set_validity(validHours);
    xtraEvent.set_datastatus(dataStatus);
    xtraEvent.set_consent(xtraConsent_);
    anyResponse.set_filter("loc_config");
    anyResponse.mutable_any()->PackFrom(xtraEvent);
    // posting the event to EventService event queue
    auto &eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void LocationConfiguratorServerImpl::triggerGnssConstellationUpdateEvent() {
    LOG(DEBUG, __FUNCTION__);
    std::string enabledMask = CommonUtils::readSystemDataValue(
        "loc/ILocationConfigurator", "0x1FFFFFF", {"ILocationConfigurator", "GnssSignalType"});
    std::lock_guard<std::mutex> lck(mtx_);
    ::locStub::GnssUpdateEvent GnssEvent;
    ::eventService::EventResponse anyResponse;
    GnssEvent.set_enabledmask(std::stoul(enabledMask, nullptr, 16));
    anyResponse.set_filter("loc_config");
    anyResponse.mutable_any()->PackFrom(GnssEvent);
    // posting the event to EventService event queue
    auto &eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

grpc::Status LocationConfiguratorServerImpl::RegisterListener(ServerContext *context,
    const locStub::RegisterListenerRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    if (request->xtra_indication() == true) {
        auto f
            = std::async(std::launch::async, [this]() { this->triggerXtraStatusEvent(); }).share();
        taskQ_->add(f);
    }
    if (request->gnss_indication() == true) {
        auto f = std::async(std::launch::async, [this]() {
            this->triggerGnssConstellationUpdateEvent();
        }).share();
        taskQ_->add(f);
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureCTUNC(ServerContext *context,
    const locStub::ConfigureCTUNCRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    bool enable           = request->enable();
    float timeUncertainty = request->time_uncertainty();
    uint32_t energyBudget = request->energy_budget();
    apiJsonReader("configureCTunc", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(enable), {"ILocationConfigurator", "CTunc", "enable"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(timeUncertainty), {"ILocationConfigurator", "CTunc", "timeUncertainty"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(energyBudget), {"ILocationConfigurator", "CTunc", "energyBudget"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigurePACE(ServerContext *context,
    const locStub::ConfigurePACERequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    bool enable = request->enable();
    apiJsonReader("configurePACE", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(enable), {"ILocationConfigurator", "PACE", "enable"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::DeleteAllAidingData(ServerContext *context,
    const google::protobuf::Empty *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("deleteAllAidingData", response);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureLeverArm(ServerContext *context,
    const locStub::ConfigureLeverArmRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureLeverArm", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        for (auto itr : request->lever_arm_config_info()) {
            if (itr.first == 1) {
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.forward_offset()),
                    {"ILocationConfigurator", "LeverArm", "GNSSTOVRPforwardOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.sideways_offset()),
                    {"ILocationConfigurator", "LeverArm", "GNSSTOVRPsidewaysOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.up_offset()),
                    {"ILocationConfigurator", "LeverArm", "GNSSTOVRPupOffset"});
            }
            if (itr.first == 2) {
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.forward_offset()),
                    {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSforwardOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.sideways_offset()),
                    {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSsidewaysOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.up_offset()),
                    {"ILocationConfigurator", "LeverArm", "DRIMUTOGNSSupOffset"});
            }
            if (itr.first == 3) {
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.forward_offset()),
                    {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSforwardOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.sideways_offset()),
                    {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSsidewaysOffset"});
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(itr.second.up_offset()),
                    {"ILocationConfigurator", "LeverArm", "VEPPIMUTOGNSSupOffset"});
            }
        }
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureConstellations(ServerContext *context,
    const locStub::ConfigureConstellationsRequest *request,
    locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureConstellations", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        std::string blacklist = "";
        for (int ind = 0; ind < request->sv_black_list_info_size(); ind++) {
            blacklist += std::to_string(
                static_cast<int>(request->sv_black_list_info(ind).constellation()));
            blacklist += " : ";
            blacklist += std::to_string(request->sv_black_list_info(ind).sv_id());
            blacklist += ", ";
        }
        if (!blacklist.empty()) {
            blacklist.pop_back();
            blacklist.pop_back();
        }
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", blacklist,
            {"ILocationConfigurator", "configureConstellations", "Blacklist"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureMinGpsWeek(ServerContext *context,
    const locStub::ConfigureMinGpsWeekRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t minGpsWeek = request->min_gps_week();
    apiJsonReader("configureMinGpsWeek", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(minGpsWeek), {"ILocationConfigurator", "MinGpsWeek", "mingpsweek"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestMinGpsWeek(ServerContext *context,
    const google::protobuf::Empty *request, locStub::RequestMinGpsWeekReply *response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(
        rootNode, "ILocationConfigurator", "requestMinGpsWeek", status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    uint16_t minGpsWeek = 0;
    if (errorCode == ErrorCode::SUCCESS) {
        minGpsWeek = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "MinGpsWeek", "mingpsweek"}));
    }
    response->set_min_gps_week(minGpsWeek);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureMinSVElevation(ServerContext *context,
    const locStub::ConfigureMinSVElevationRequest *request,
    locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t minSVElevation = request->min_sv_elevation();
    apiJsonReader("configureMinSVElevation", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(minSVElevation),
            {"ILocationConfigurator", "MinSvElevation", "minSVElevation"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestMinSVElevation(ServerContext *context,
    const google::protobuf::Empty *request, locStub::RequestMinSVElevationReply *response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(
        rootNode, "ILocationConfigurator", "requestMinSVElevation", status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    uint16_t minSVElevation = 0;
    if (errorCode == ErrorCode::SUCCESS) {
        minSVElevation = std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator",
            "0", {"ILocationConfigurator", "MinSvElevation", "minSVElevation"}));
    }
    response->set_min_sv_elevation(minSVElevation);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureRobustLocation(ServerContext *context,
    const locStub::ConfigureRobustLocationRequest *request,
    locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    bool enable        = request->enable();
    bool enableForE911 = request->enable_for_e911();
    apiJsonReader("configureRobustLocation", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(enable), {"ILocationConfigurator", "RobustLocation", "enable"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(enableForE911),
            {"ILocationConfigurator", "RobustLocation", "enableForE911"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestRobustLocation(ServerContext *context,
    const google::protobuf::Empty *request, locStub::RequestRobustLocationReply *response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(
        rootNode, "ILocationConfigurator", "requestRobustLocation", status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    response->mutable_robust_location_configuration()->set_enabled(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "RobustLocation", "enable"})));
    response->mutable_robust_location_configuration()->set_enabled_for_e911(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "RobustLocation", "enableForE911"})));
    response->mutable_robust_location_configuration()->set_valid_mask(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "7",
            {"ILocationConfigurator", "RobustLocation", "validity"})));
    response->mutable_robust_location_configuration()->mutable_version()->set_major_version(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "1",
            {"ILocationConfigurator", "RobustLocation", "majorversion"})));
    response->mutable_robust_location_configuration()->mutable_version()->set_minor_version(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "1",
            {"ILocationConfigurator", "RobustLocation", "minorversion"})));

    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureSecondaryBand(ServerContext *context,
    const locStub::ConfigureSecondaryBandRequest *request,
    locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureSecondaryBand", response);
    std::bitset<25> disabledSignal;
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        std::string secBandSet = "";
        for (int ind = 0; ind < request->constellation_set_size(); ind++) {
            int id = static_cast<int>(request->constellation_set(ind));
            secBandSet += std::to_string(id);
            secBandSet += ", ";
            switch (request->constellation_set(ind)) {
                case ::locStub::GnssConstellationType::GPS:
                    disabledSignal |= telux::loc::GPS_L2;
                    disabledSignal |= telux::loc::GPS_L5;
                    break;
                case ::locStub::GnssConstellationType::GALILEO:
                    disabledSignal |= telux::loc::GALILEO_E5A;
                    disabledSignal |= telux::loc::GALILIEO_E5B;
                    break;
                case ::locStub::GnssConstellationType::GLONASS:
                    disabledSignal |= telux::loc::GLONASS_G2;
                    break;
                case ::locStub::GnssConstellationType::BDS:
                    disabledSignal |= telux::loc::BEIDOU_B2I;
                    disabledSignal |= telux::loc::BEIDOU_B2AI;
                    disabledSignal |= telux::loc::BEIDOU_B2AQ;
                    disabledSignal |= telux::loc::BEIDOU_B2BQ;
                    disabledSignal |= telux::loc::BEIDOU_B2BI;
                    break;
                case ::locStub::GnssConstellationType::QZSS:
                    disabledSignal |= telux::loc::QZSS_L2;
                    disabledSignal |= telux::loc::QZSS_L5;
                    break;
                case ::locStub::GnssConstellationType::NAVIC:
                    disabledSignal |= telux::loc::NAVIC_L5;
                    break;
                case ::locStub::GnssConstellationType::SBAS:
                case ::locStub::GnssConstellationType::COMPASS:
                default:
                    break;
            }
        }
        if (!secBandSet.empty()) {
            secBandSet.pop_back();
            secBandSet.pop_back();
        }
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", secBandSet,
            {"ILocationConfigurator", "SecondaryBand", "Set"});
        unsigned long val = disabledSignal.flip().to_ulong();
        std::stringstream ss;
        ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << val;
        std::string enabledMask        = "0X" + ss.str();
        std::string currentEnabledMask = CommonUtils::readSystemDataValue(
            "loc/ILocationConfigurator", "0x1FFFFFF", {"ILocationConfigurator", "GnssSignalType"});
        if (enabledMask != currentEnabledMask) {
            CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", enabledMask,
                {"ILocationConfigurator", "GnssSignalType"});
            auto f = std::async(std::launch::async, [this]() {
                this->triggerGnssConstellationUpdateEvent();
            }).share();
            taskQ_->add(f);
        }
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestSecondaryBandConfig(ServerContext *context,
    const google::protobuf::Empty *request, locStub::RequestSecondaryBandConfigReply *response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationConfigurator", "requestSecondaryBandConfig", status,
        errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    std::string str = CommonUtils::readSystemDataValue(
        "loc/ILocationConfigurator", "8", {"ILocationConfigurator", "SecondaryBand", "Set"});
    for (size_t itr = 0; itr < str.size(); itr++) {
        if (str[itr] >= '0' && str[itr] <= '8') {
            int constel = (str[itr] - 48);
            response->add_constellation_set(static_cast<::locStub::GnssConstellationType>(constel));
        }
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::DeleteAidingData(ServerContext *context,
    const locStub::DeleteAidingDataRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t aidingDataMask = request->aiding_data_mask();
    apiJsonReader("deleteAidingData", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(aidingDataMask),
            {"ILocationConfigurator", "DeleteAidingData", "aidingDataMask"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureDR(ServerContext *context,
    const locStub::ConfigureDRRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureDR", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().speed_factor())),
            {"ILocationConfigurator", "configureDR", "speedFactor"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().speed_factor_unc())),
            {"ILocationConfigurator", "configureDR", "speedFactorUnc"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().gyro_factor())),
            {"ILocationConfigurator", "configureDR", "gyroFactor"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().gyro_factor_unc())),
            {"ILocationConfigurator", "configureDR", "gyroFactorUnc"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().mount_param().roll_offset())),
            {"ILocationConfigurator", "configureDR", "rollOffset"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().mount_param().yaw_offset())),
            {"ILocationConfigurator", "configureDR", "yawOffset"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().mount_param().pitch_offset())),
            {"ILocationConfigurator", "configureDR", "pitchOffset"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<float>(request->config().mount_param().offset_unc())),
            {"ILocationConfigurator", "configureDR", "offsetUnc"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->config().valid_mask())),
            {"ILocationConfigurator", "configureDR", "validity"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureEngineState(ServerContext *context,
    const locStub::ConfigureEngineStateRequest *request,
    locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t engineType  = request->engine_type();
    uint16_t engineState = request->engine_state();
    apiJsonReader("configureEngineState", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(engineType)),
            {"ILocationConfigurator", "EngineState", "engineType"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(engineState)),
            {"ILocationConfigurator", "EngineState", "engineState"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ProvideConsentForTerrestrialPositioning(
    ServerContext *context, const locStub::ProvideConsentForTerrestrialPositioningRequest *request,
    locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    bool consent = request->user_consent();
    apiJsonReader("provideConsentForTerrestrialPositioning", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(consent),
            {"ILocationConfigurator", "ConsentForTerrestrialPositioning", "Consent"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureNmeaTypes(ServerContext *context,
    const locStub::ConfigureNmeaTypesRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t nmeaType = request->nmea_type();
    apiJsonReader("configureNmeaTypes", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(nmeaType),
            {"ILocationConfigurator", "configureNmeaTypes", "sentenceConfig"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureNmea(ServerContext *context,
    const locStub::ConfigureNmeaRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t nmeaType   = request->nmea_type();
    uint16_t engineType = request->engine_type();
    uint16_t datumType;
    if (request->datum_type() == ::locStub::DatumType::WGS_84) {
        datumType = 0;
    } else if (request->datum_type() == ::locStub::DatumType::PZ_90) {
        datumType = 1;
    }
    apiJsonReader("configureNmea", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(nmeaType), {"ILocationConfigurator", "configureNmea", "sentenceConfig"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(datumType), {"ILocationConfigurator", "configureNmea", "datumType"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(engineType), {"ILocationConfigurator", "configureNmea", "engineType"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureEngineIntegrityRisk(ServerContext *context,
    const locStub::ConfigureEngineIntegrityRiskRequest *request,
    locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    uint16_t integRisk  = request->integrity_risk();
    uint16_t engineType = request->engine_type();
    apiJsonReader("configureEngineIntegrityRisk", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(engineType),
            {"ILocationConfigurator", "configureEngineIntegrityRisk", "engineType"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(integRisk),
            {"ILocationConfigurator", "configureEngineIntegrityRisk", "integrityRisk"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureXtraParams(ServerContext *context,
    const locStub::ConfigureXtraParamsRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureXtraParams", response);
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        if (xtraConsent_ == false) {
            CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                std::to_string(static_cast<int>(false)),
                {"ILocationConfigurator", "XtraEnablement", "enable"});
            xtraEnabled_ = false;
        } else {
            CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                std::to_string(static_cast<int>(request->enable())),
                {"ILocationConfigurator", "XtraEnablement", "enable"});
            xtraEnabled_ = request->enable();
        }
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->download_interval_minute())),
            {"ILocationConfigurator", "XtraParams", "downloadIntervalMinute"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->download_timeout_sec())),
            {"ILocationConfigurator", "XtraParams", "downloadTimeoutSec"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->download_retry_interval_minute())),
            {"ILocationConfigurator", "XtraParams", "downloadRetryIntervalMinute"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->download_retry_attempts())),
            {"ILocationConfigurator", "XtraParams", "downloadRetryAttempts"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator", request->ca_path(),
            {"ILocationConfigurator", "XtraParams", "caPath"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->integrity_download_enabled())),
            {"ILocationConfigurator", "XtraParams", "isIntegrityDownloadEnabled"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->integrity_download_interval_minute())),
            {"ILocationConfigurator", "XtraParams", "integrityDownloadIntervalMinute"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->daemon_debug_log_level())),
            {"ILocationConfigurator", "XtraParams", "daemonDebugLogLevel"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            request->server_urls(), {"ILocationConfigurator", "XtraParams", "serverURLs"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            request->ntp_server_urls(), {"ILocationConfigurator", "XtraParams", "ntpServerURLs"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            request->nts_server_url(), {"ILocationConfigurator", "XtraParams", "ntsServerURL"});
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(request->diag_logging_enabled())),
            {"ILocationConfigurator", "XtraParams", "diagLoggingEnabled"});
    }
    if (xtraSet_ != request->enable()) {
        auto f
            = std::async(std::launch::async, [this]() { this->triggerXtraStatusEvent(); }).share();
        taskQ_->add(f);
        xtraSet_ = request->enable();
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(xtraSet_)),
            {"ILocationConfigurator", "XtraEnablement", "set"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestXtraStatus(ServerContext *context,
    const google::protobuf::Empty *request, locStub::RequestXtraStatusReply *response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(
        rootNode, "ILocationConfigurator", "requestXtraStatus", status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
    response->mutable_xtra_status()->set_feature_enabled(xtraEnabled_);
    response->mutable_xtra_status()->set_xtra_valid_for_hours(
        std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
            {"ILocationConfigurator", "XtraParams", "xtraValidForHours"})));
    if (xtraConsent_) {
        response->mutable_xtra_status()->set_xtra_data_status(
            std::stoi(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "XtraParams", "xtraDataStatus"})));
    } else {
        response->mutable_xtra_status()->set_xtra_data_status(0);
    }
    response->mutable_xtra_status()->set_consent(xtraConsent_);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::RequestNavMsgAuthStatus(ServerContext *context,
    const locStub::RequestNavMsgAuthStatusRequest *request,
    locStub::RequestNavMsgAuthStatusReply *response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(
        rootNode, "ILocationConfigurator", "requestNavMsgAuthStatus", status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);

    auto navMsgAuthStatus = response->mutable_nav_msg_auth_status();
    navMsgAuthStatus->set_constellation(request->constellation());

    auto galStatus = navMsgAuthStatus->mutable_gal_status();
    if (request->constellation() == ::locStub::GnssConstellationType::GALILEO) {
        galStatus->set_validity_mask(parseUint32(
            CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "validityMask"}),
            0));
        galStatus->set_feature_enabled(parseUint32(
            CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "featureEnabled"}),
            0));
        galStatus->set_merkle_tree_provisioned(
            parseUint32(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                            {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus",
                                "merkleTreeProvisioned"}),
                0));
        galStatus->set_public_key_status_mask(
            parseUint32(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                            {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus",
                                "publicKeyStatusMask"}),
                0));
        galStatus->set_gal_week(parseUint32(
            CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "galWeek"}),
            0));
        galStatus->set_gal_sec_of_week(parseUint32(
            CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus", "galSecOfWeek"}),
            0));
        galStatus->set_nav_data_auth_passed_mask(
            parseUint64(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                            {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus",
                                "navDataAuthPassedMask"}),
                0));
        galStatus->set_nav_data_auth_failed_mask(
            parseUint64(CommonUtils::readSystemDataValue("loc/ILocationConfigurator", "0",
                            {"ILocationConfigurator", "NavMsgAuthStatus", "GalOsnmaStatus",
                                "navDataAuthFailedMask"}),
                0));
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::InjectMerkleTree(ServerContext *context,
    const google::protobuf::Empty *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("injectMerkleTreeInformation", response);
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ConfigureOsnma(ServerContext *context,
    const locStub::ConfigureOsnmaRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("configureOsnma", response);
    bool enable = request->enable();
    if (response->error() == ::commonStub::ErrorCode::ERROR_CODE_SUCCESS) {
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(enable), {"ILocationConfigurator", "configureOsnma", "enable"});
    }
    return grpc::Status::OK;
}

grpc::Status LocationConfiguratorServerImpl::ProvideXtraConsent(ServerContext *context,
    const locStub::XtraConsentRequest *request, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    apiJsonReader("provideConsentForXtra", response);
    bool consent = request->consent();
    if (consent != xtraConsent_) {
        if (consent == false) {
            xtraEnabled_ = false;
            CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                std::to_string(static_cast<int>(false)),
                {"ILocationConfigurator", "XtraEnablement", "enable"});
        } else {
            if (xtraSet_) {
                xtraEnabled_ = true;
                CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
                    std::to_string(static_cast<int>(true)),
                    {"ILocationConfigurator", "XtraEnablement", "enable"});
            }
        }
        xtraConsent_ = consent;
        CommonUtils::writeSystemDataValue<string>("loc/ILocationConfigurator",
            std::to_string(static_cast<int>(consent)),
            {"ILocationConfigurator", "XtraEnablement", "consent"});
        auto f
            = std::async(std::launch::async, [this]() { this->triggerXtraStatusEvent(); }).share();
        taskQ_->add(f);
    }
    return grpc::Status::OK;
}

void LocationConfiguratorServerImpl::apiJsonReader(
    std::string apiName, locStub::LocManagerCommandReply *response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootNode;
    JsonParser::readFromJsonFile(rootNode, LOC_CONFIG_API_JSON);
    telux::common::Status status;
    telux::common::ErrorCode errorCode;
    int cbDelay;
    CommonUtils::getValues(rootNode, "ILocationConfigurator", apiName, status, errorCode, cbDelay);
    response->set_status(static_cast<::commonStub::Status>(status));
    response->set_error(static_cast<::commonStub::ErrorCode>(errorCode));
    response->set_delay(cbDelay);
}
