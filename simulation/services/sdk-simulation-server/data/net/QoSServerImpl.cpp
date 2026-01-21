/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "QoSServerImpl.hpp"
#include "event/EventService.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/Logger.hpp"

#define QOS_EVENT "qos"
#define QOS_MANAGER_API_LOCAL_JSON "api/data/IQoSManager.json"
#define QOS_MANAGER_STATE_JSON "system-state/data/IQoSManagerState.json"
#define VLAN_MANAGER_API_LOCAL_JSON "api/data/IVlanManagerLocal.json"
#define VLAN_MANAGER_STATE_JSON "system-state/data/IVlanManagerState.json"

QoSServerImpl::QoSServerImpl(std::shared_ptr<DataConnectionServerImpl> dcmServerImpl)
   : dcmServerImpl_(dcmServerImpl) {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

QoSServerImpl::~QoSServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    ServerEventManager::getInstance().deregisterListener(
        shared_from_this(), "qos_filter_status_change_event");
}

grpc::Status QoSServerImpl::InitService(ServerContext *context,
    const dataStub::InitRequest *request, dataStub::GetServiceStatusReply *response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath           = QOS_MANAGER_API_LOCAL_JSON;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ");
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay                         = rootObj["IQoSManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus                = rootObj["IQoSManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        ServerEventManager::getInstance().registerListener(shared_from_this(), QOS_EVENT);
    }

    std::vector<std::string> filters = {DATA_CONNECTION_FILTER};
    telux::common::ClientEventManager::getInstance().registerListener(shared_from_this(), filters);

    return grpc::Status::OK;
}

bool QoSServerImpl::validateCreateTrafficClassRequest(
    const dataStub::CreateTrafficClassRequest *request, const telux::common::JsonData &jsonData,
    dataStub::TcConfigErrorCode &errorCode) {

    auto req_tc_config      = request->tc_config();
    uint32_t trafficClassId = req_tc_config.traffic_class();

    // 1. Validate traffic class ID range
    if (trafficClassId < 0 || trafficClassId > 7) {  // Traffic class 0-7 allowed
        LOG(ERROR, __FUNCTION__, "Invalid traffic class ID: ", trafficClassId,
            ". Must be between 0 and 7.");
        return false;
    }

    // 2. Validate validity mask for mandatory fields
    // (These already ensure dataPath and direction are present)
    if (!(req_tc_config.validity_mask() & (1 << dataStub::TC_TRAFFIC_CLASS_VALID))) {
        errorCode = dataStub::TC_MISSING_TRAFFIC_CLASS;
        return false;
    }
    if (!(req_tc_config.validity_mask() & (1 << dataStub::TC_DATA_PATH_VALID))) {
        errorCode = dataStub::TC_MISSING_DATA_PATH;
        return false;
    }
    if (!(req_tc_config.validity_mask() & (1 << dataStub::TC_DIRECTION_VALID))) {
        errorCode = dataStub::TC_MISSING_DIRECTION;
        return false;
    }

    // 3. Check for duplicate Traffic Class ID
    const Json::Value &subsystemData = jsonData.stateRootObj["IQoSManager"];
    if (subsystemData.isMember("trafficClasses") && subsystemData["trafficClasses"].isArray()) {
        const Json::Value &tc_array = subsystemData["trafficClasses"];
        for (Json::ValueConstIterator it = tc_array.begin(); it != tc_array.end(); ++it) {
            if ((*it).isMember("trafficClass") && (*it)["trafficClass"].isUInt()
                && (*it)["trafficClass"].asUInt()
                       == trafficClassId) {  // Compare with the validated ID
                LOG(ERROR, __FUNCTION__, "Traffic class with ID ", trafficClassId,
                    " already exists.");
                return false;
            }
        }
    }
    return true;
}

bool QoSServerImpl::validateModemPrioritizationFilter(
    const dataStub::ITrafficFilter &trafficFilter, uint32_t trafficClass) {

#ifdef null
    // Note: This configuration is not provided in any TelSDK documentation,
    // as we don’t refer to specific settings that might exist in underlying
    // configuration files. Enable this if needed. (Also IQoSManager.json add this
    // "HighestPriULModemTC": 2)

    std::string apiJsonPath = QOS_MANAGER_API_LOCAL_JSON;
    Json::Value rootObj;  // Use rootObj to read the API file
    telux::common::ErrorCode readError = JsonParser::readFromJsonFile(rootObj, apiJsonPath);

    uint32_t highestPriULModemTC = 0;  // Default or safe value
    if (readError == telux::common::ErrorCode::SUCCESS && rootObj.isMember("IQoSManager")
        && rootObj["IQoSManager"].isMember("HighestPriULModemTC")
        && rootObj["IQoSManager"]["HighestPriULModemTC"].isUInt()) {
        highestPriULModemTC = rootObj["IQoSManager"]["HighestPriULModemTC"].asUInt();
    } else {
        LOG(WARNING, __FUNCTION__,
            "Could not read 'HighestPriULModemTC' from API JSON. Defaulting to 0.");
        // Consider if this should be an error or continue with a default
    }

    // "If TC# < X and datapath towards modem then TC will be rejected."
    if (trafficClass <= highestPriULModemTC) {
        LOG(ERROR, __FUNCTION__, "Traffic Class (", trafficClass,
            ") is less than HighestPriULModemTC (", highestPriULModemTC,
            "). This TC is rejected for modem prioritization as per "
            "configuration.");
        return false;
    }
#endif

    // 1. Check for Source IP
    bool hasSourceIp = (trafficFilter.validity_mask() & dataStub::TF_SOURCE_IPV4_ADDRESS_VALID)
                       || (trafficFilter.validity_mask() & dataStub::TF_SOURCE_IPV6_ADDRESS_VALID);
    if (!hasSourceIp) {
        LOG(ERROR, __FUNCTION__, "Modem prioritization filter for APPS_TO_WAN requires Source IP.");
        return false;
    }

    // 2. Check for IP Protocol
    if (!(trafficFilter.validity_mask() & dataStub::TF_IP_PROTOCOL_VALID)) {
        LOG(ERROR, __FUNCTION__,
            "Modem prioritization filter for APPS_TO_WAN requires IP Protocol.");
        return false;
    }

    // 3. Check for Destination Address or Destination Port
    bool hasDestAddressOrPort
        = (trafficFilter.validity_mask() & dataStub::TF_DESTINATION_IPV4_ADDRESS_VALID)
          || (trafficFilter.validity_mask() & dataStub::TF_DESTINATION_IPV6_ADDRESS_VALID)
          || (trafficFilter.validity_mask() & dataStub::TF_DESTINATION_PORT_VALID)
          || (trafficFilter.validity_mask() & dataStub::TF_DESTINATION_PORT_RANGE_VALID)
          || (trafficFilter.validity_mask() & dataStub::TF_DESTINATION_PORT_CONFIG_VALID);
    if (!hasDestAddressOrPort) {
        LOG(ERROR, __FUNCTION__,
            "Modem prioritization filter for APPS_TO_WAN requires Destination IP "
            "or Port.");
        return false;
    }

    // 4. Direction
    if (trafficFilter.direction() != dataStub::Direction::UPLINK) {
        LOG(ERROR, __FUNCTION__, "Modem prioritization filter for non uplink direction.");
        return false;
    }
    return true;  // Validation passed for modem prioritization requirements
}

bool QoSServerImpl::validateQoSFilterRequest(const dataStub::AddQoSFilterRequest *request,
    const telux::common::JsonData &jsonData, dataStub::QoSFilterErrorCode &errorCode) {

    // Initial check for direction
    if (request->traffic_filter().direction() == ::dataStub::Direction::DIRECTION_UNSPECIFIED) {
        errorCode = dataStub::QOS_MISSING_DIRECTION;
        return false;
    }

    // Ensure trafficClasses array exists in the state JSON (for matching traffic
    // class ID)
    const Json::Value &subsystemData = jsonData.stateRootObj["IQoSManager"];
    if (!subsystemData.isMember("trafficClasses") || !subsystemData["trafficClasses"].isArray()) {
        LOG(WARNING, __FUNCTION__,
            "No 'trafficClasses' array found in state JSON or it's not an array. "
            "Validation will fail.");
        return false;
    }

    const Json::Value &trafficClasses      = subsystemData["trafficClasses"];
    uint32_t requestedTrafficClass         = request->traffic_class();
    dataStub::Direction requestedDirection = request->traffic_filter().direction();
    dataStub::DataPath requestedDataPath   = request->traffic_filter().data_path();

    bool matchingTrafficClassFound = false;
    for (const auto &tcJson : trafficClasses) {
        // Ensure the JSON values exist and are of the expected type before
        // accessing
        if (tcJson.isMember("trafficClass") && tcJson["trafficClass"].isUInt()
            && tcJson.isMember("direction") && tcJson["direction"].isUInt()
            && tcJson.isMember("dataPath") && tcJson["dataPath"].isUInt()) {

            uint32_t currentTrafficClass = tcJson["trafficClass"].asUInt();
            dataStub::Direction currentDirection
                = static_cast<dataStub::Direction>(tcJson["direction"].asUInt());
            dataStub::DataPath currentDataPath
                = static_cast<dataStub::DataPath>(tcJson["dataPath"].asUInt());

            if (currentTrafficClass == requestedTrafficClass
                && currentDirection == requestedDirection && currentDataPath == requestedDataPath) {
                matchingTrafficClassFound = true;
                break;
            }
        } else {
            LOG(WARNING, __FUNCTION__, "Malformed traffic class entry in state JSON.");
        }
    }

    if (!matchingTrafficClassFound) {
        LOG(ERROR, __FUNCTION__,
            "No matching traffic class found for requested traffic_class: ", requestedTrafficClass,
            ", direction: ", static_cast<int>(requestedDirection),
            ", dataPath: ", static_cast<int>(requestedDataPath));
        return false;
    }

    // --- NEW DUPLICATE QOS FILTER CHECK ---
    if (subsystemData.isMember("qosFilters") && subsystemData["qosFilters"].isArray()) {
        const Json::Value &qos_filters_array = subsystemData["qosFilters"];
        // Convert the incoming traffic filter to JSON for comparison
        Json::Value requestedTrafficFilterJson
            = convertITrafficFilterToJson(request->traffic_filter());

        for (Json::ValueConstIterator it = qos_filters_array.begin(); it != qos_filters_array.end();
             ++it) {
            // Check if the current filter entry is valid before comparing
            if ((*it).isMember("trafficClass") && (*it)["trafficClass"].isUInt()
                && (*it).isMember("trafficFilter")) {
                if ((*it)["trafficClass"].asUInt() == requestedTrafficClass
                    && (*it)["trafficFilter"] == requestedTrafficFilterJson) {
                    LOG(ERROR, __FUNCTION__, "QoS Filter with identical traffic class (",
                        requestedTrafficClass,
                        ") and traffic filter configuration already exists.");
                    return false;
                }
            }
        }
    }

    if (request->traffic_filter().source_vlan_list().size() > 0) {
        std::string apiJsonPath   = VLAN_MANAGER_API_LOCAL_JSON;
        std::string stateJsonPath = VLAN_MANAGER_STATE_JSON;
        std::string vlanSubsystem = "IVlanManager";
        std::string vlanMethod    = "queryVlanInfo";  // Or any method that retrieves VLANs

        telux::common::JsonData vlanData;
        telux::common::ErrorCode vlanError = telux::common::CommonUtils::readJsonData(
            apiJsonPath, stateJsonPath, vlanSubsystem, vlanMethod, vlanData);

        if (vlanError != telux::common::ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Failed to read VLAN Manager state JSON for validation.");
            return false;
        }

        if (vlanData.status == telux::common::Status::SUCCESS
            && vlanData.error == telux::common::ErrorCode::SUCCESS) {

            // Build a set of existing VLAN IDs for quick lookup
            std::set<int> existingVlanIds;
            if (vlanData.stateRootObj[vlanSubsystem].isMember("vlanConfig")
                && vlanData.stateRootObj[vlanSubsystem]["vlanConfig"].isArray()) {
                const Json::Value &vlanConfigs = vlanData.stateRootObj[vlanSubsystem]["vlanConfig"];
                for (Json::ValueConstIterator it = vlanConfigs.begin(); it != vlanConfigs.end();
                     ++it) {
                    if ((*it).isMember("vlanId") && (*it)["vlanId"].isInt()) {
                        existingVlanIds.insert((*it)["vlanId"].asInt());
                    }
                }
            }

            // Check if each requested VLAN exists in the set
            for (int vlan : request->traffic_filter().source_vlan_list()) {
                if (existingVlanIds.find(vlan) == existingVlanIds.end()) {
                    LOG(ERROR, __FUNCTION__, "Requested source VLAN ID ", vlan, " does not exist");
                    return false;
                }
            }
        } else {
            // If VLAN manager read was successful but reported an error status
            LOG(ERROR, __FUNCTION__,
                "VLAN Manager state reported an error. Status: ", static_cast<int>(vlanData.status),
                ", Error: ", static_cast<int>(vlanData.error));
            return false;
        }
    }

    return true;  // Validation passed
}

grpc::Status QoSServerImpl::CreateTrafficClass(ServerContext *context,
    const dataStub::CreateTrafficClassRequest *request,
    dataStub::CreateTrafficClassReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "createTrafficClass";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    dataStub::TcConfigErrorCode tcConfigErrorCode = dataStub::TC_SUCCESS;
    if (!validateCreateTrafficClassRequest(request, data, tcConfigErrorCode)) {
        // If validation failed, tcConfigErrorCode is already set by
        // validateCreateTrafficClassRequest
        response->mutable_reply()->set_error(
            static_cast<commonStub::ErrorCode>(telux::common::ErrorCode::INVALID_ARGUMENTS));
        response->set_tc_config_error_code(tcConfigErrorCode);
        return grpc::Status::OK;
    }
    if (data.error == telux::common::ErrorCode::SUCCESS) {
        // If we reach here, it means no duplicate was found and mandatory fields
        // are present. So, we just append it as a new entry.
        if (!data.stateRootObj[subsystem].isMember("trafficClasses")
            || !data.stateRootObj[subsystem]["trafficClasses"].isArray()) {
            data.stateRootObj[subsystem]["trafficClasses"] = Json::arrayValue;
        }
        data.stateRootObj[subsystem]["trafficClasses"].append(
            convertITcConfigToJson(request->tc_config()));
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_tc_config_error_code(tcConfigErrorCode);

    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::GetAllTrafficClasses(ServerContext *context,
    const google::protobuf::Empty *request, dataStub::GetAllTrafficClassesReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "getAllTrafficClasses";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        const Json::Value &tc_array = data.stateRootObj[subsystem]["trafficClasses"];
        for (Json::ValueConstIterator it = tc_array.begin(); it != tc_array.end(); ++it) {
            dataStub::ITcConfig *tc_config = response->add_tc_configs();
            convertJsonToITcConfig(*it, tc_config);  // Use local helper
        }
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::DeleteTrafficClass(ServerContext *context,
    const dataStub::DeleteTrafficClassRequest *request, dataStub::DefaultReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "deleteTrafficClass";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value &tc_array = data.stateRootObj[subsystem]["trafficClasses"];
        Json::Value new_tc_array;
        bool found = false;
        // Find and remove the specified traffic class
        for (Json::ValueIterator it = tc_array.begin(); it != tc_array.end(); ++it) {
            if ((*it)["trafficClass"].asUInt() == request->tc_config().traffic_class()) {
                found = true;
            } else {
                new_tc_array.append(*it);
            }
        }
        if (found) {
            data.stateRootObj[subsystem]["trafficClasses"] = new_tc_array;
            // Also delete associated QoS filters with this traffic class
            Json::Value &qos_filters_array = data.stateRootObj[subsystem]["qosFilters"];
            Json::Value new_qos_filters_array;
            for (Json::ValueIterator it = qos_filters_array.begin(); it != qos_filters_array.end();
                 ++it) {
                if ((*it)["trafficClass"].asUInt() == request->tc_config().traffic_class()) {
                    // This filter is associated with the deleted traffic class, so remove
                    // it
                } else {
                    new_qos_filters_array.append(*it);
                }
            }
            data.stateRootObj[subsystem]["qosFilters"] = new_qos_filters_array;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            // If traffic class not found, return appropriate error
            data.status = telux::common::Status::FAILED;
            data.error  = telux::common::ErrorCode::NO_SUCH_ENTRY;
        }
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::AddQoSFilter(ServerContext *context,
    const dataStub::AddQoSFilterRequest *request, dataStub::AddQoSFilterReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "addQoSFilter";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    dataStub::QoSFilterErrorCode qosFilterErrorCode = dataStub::QoSFilterErrorCode::QOS_SUCCESS;
    if (!validateQoSFilterRequest(request, data, qosFilterErrorCode)) {
        // If validation failed, qosFilterErrorCode is already set by
        // validateQoSFilterRequest
        response->mutable_reply()->set_error(
            static_cast<commonStub::ErrorCode>(telux::common::ErrorCode::INVALID_ARGUMENTS));
        response->set_qos_filter_error_code(qosFilterErrorCode);
        return grpc::Status::OK;
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        // Generate a new handle for the QoS filter
        uint32_t handle = data.stateRootObj[subsystem]["nextQoSFilterHandle"].asUInt();
        data.stateRootObj[subsystem]["nextQoSFilterHandle"] = handle + 1;

        Json::Value newFilter;
        newFilter["handle"]        = handle;
        newFilter["trafficClass"]  = request->traffic_class();
        newFilter["trafficFilter"] = convertITrafficFilterToJson(request->traffic_filter());

        // Simulate installation status
        Json::Value status{};
        status["ethStatus"]   = static_cast<uint32_t>(dataStub::FilterInstallationStatus::SUCCESS);
        status["modemStatus"] = static_cast<uint32_t>(dataStub::FilterInstallationStatus::SUCCESS);
        status["ipaStatus"]   = static_cast<uint32_t>(dataStub::FilterInstallationStatus::SUCCESS);

        if (request->traffic_filter().data_path() == ::dataStub::DataPath::APPS_TO_WAN) {
            status["ethStatus"]
                = static_cast<uint32_t>(dataStub::FilterInstallationStatus::NOT_APPLICABLE);
            status["ipaStatus"]
                = static_cast<uint32_t>(dataStub::FilterInstallationStatus::NOT_APPLICABLE);
        }

        if (request->traffic_filter().data_path() == ::dataStub::DataPath::TETHERED_TO_APPS_SW) {
            status["ipaStatus"]
                = static_cast<uint32_t>(dataStub::FilterInstallationStatus::NOT_APPLICABLE);
            status["modemStatus"]
                = static_cast<uint32_t>(dataStub::FilterInstallationStatus::NOT_APPLICABLE);
        }

        if ((request->traffic_filter().data_path() == ::dataStub::DataPath::APPS_TO_WAN)
            || (request->traffic_filter().data_path()
                == ::dataStub::DataPath::TETHERED_TO_WAN_HW)) {

            if (!validateModemPrioritizationFilter(
                    request->traffic_filter(), request->traffic_class())) {
                status["modemStatus"]
                    = static_cast<uint32_t>(dataStub::FilterInstallationStatus::FAILED);
            } else {
                if (!(dcmServerImpl_->isAnyDataCallActive(SLOT_ID_1)
                        || dcmServerImpl_->isAnyDataCallActive(SLOT_ID_2))) {
                    LOG(DEBUG, __FUNCTION__, " as data call not active modem status is PENDING");
                    status["modemStatus"]
                        = static_cast<uint32_t>(dataStub::FilterInstallationStatus::PENDING);
                }
            }
        }
        newFilter["status"] = status;
        data.stateRootObj[subsystem]["qosFilters"].append(newFilter);
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);

        ::eventService::EventResponse anyResponse;
        ::dataStub::QoSFilterStatusChangeEvent qosFilterStatusChangeEvent;
        qosFilterStatusChangeEvent.set_handle(handle);
        convertJsonToQoSFilterStatus(status, qosFilterStatusChangeEvent.mutable_status());
        anyResponse.set_filter(DATA_QOS_FILTER);
        anyResponse.mutable_any()->PackFrom(qosFilterStatusChangeEvent);
        // posting the event to EventService event queue
        auto &eventImpl = EventService::getInstance();
        eventImpl.updateEventQueue(anyResponse);
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_qos_filter_error_code(qosFilterErrorCode);
    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::GetQosFilter(ServerContext *context,
    const dataStub::GetQosFilterRequest *request, dataStub::GetQosFilterReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "getQosFilter";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        const Json::Value &qos_filters_array = data.stateRootObj[subsystem]["qosFilters"];
        bool found                           = false;
        for (Json::ValueConstIterator it = qos_filters_array.begin(); it != qos_filters_array.end();
             ++it) {
            if ((*it)["handle"].asUInt() == request->filter_handle()) {
                convertJsonToIQoSFilter(*it, response->mutable_qos_filter());  // Use local helper
                found = true;
                break;
            }
        }
        if (!found) {
            data.status = telux::common::Status::FAILED;
            data.error  = telux::common::ErrorCode::NO_SUCH_ENTRY;
        }
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::GetQosFilters(ServerContext *context,
    const google::protobuf::Empty *request, dataStub::GetQosFiltersReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "getQosFilters";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        const Json::Value &qos_filters_array = data.stateRootObj[subsystem]["qosFilters"];
        for (Json::ValueConstIterator it = qos_filters_array.begin(); it != qos_filters_array.end();
             ++it) {
            dataStub::IQoSFilter *qos_filter = response->add_qos_filters();
            convertJsonToIQoSFilter(*it, qos_filter);  // Use local helper
        }
    }

    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::DeleteQosFilter(ServerContext *context,
    const dataStub::DeleteQosFilterRequest *request, dataStub::DefaultReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "deleteQosFilter";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value &qos_filters_array = data.stateRootObj[subsystem]["qosFilters"];
        Json::Value new_qos_filters_array;
        bool found = false;
        for (Json::ValueIterator it = qos_filters_array.begin(); it != qos_filters_array.end();
             ++it) {
            if ((*it)["handle"].asUInt() == request->filter_handle()) {
                found = true;
            } else {
                new_qos_filters_array.append(*it);
            }
        }
        if (found) {
            data.stateRootObj[subsystem]["qosFilters"] = new_qos_filters_array;
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        } else {
            data.status = telux::common::Status::FAILED;
            data.error  = telux::common::ErrorCode::NO_SUCH_ENTRY;
        }
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::DeleteAllQosConfigs(ServerContext *context,
    const google::protobuf::Empty *request, dataStub::DefaultReply *response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "deleteAllQosConfigs";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        // Clear all filters and traffic classes and reset next handle
        data.stateRootObj[subsystem]["qosFilters"].clear();
        data.stateRootObj[subsystem]["trafficClasses"].clear();
        data.stateRootObj[subsystem]["nextQoSFilterHandle"] = 1;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_error(static_cast<commonStub::ErrorCode>(data.error));

    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::RegisterOnQoSFilterStatusChange(ServerContext *context,
    const google::protobuf::Empty *request,
    grpc::ServerWriter<dataStub::RegisterOnQoSFilterStatusChangeReply> *writer) {
    LOG(DEBUG, __FUNCTION__);
    std::unique_lock<std::mutex> lock(qosFilterStatusMtx_);
    qosFilterStatusWriters_.push_back(
        std::shared_ptr<grpc::ServerWriter<dataStub::RegisterOnQoSFilterStatusChangeReply>>(
            writer));

    qosFilterStatusCv_.wait(lock, [context] { return context->IsCancelled(); });

    qosFilterStatusWriters_.erase(
        std::remove_if(qosFilterStatusWriters_.begin(), qosFilterStatusWriters_.end(),
            [writer](const std::shared_ptr<
                grpc::ServerWriter<dataStub::RegisterOnQoSFilterStatusChangeReply>> &w) {
                return w.get() == writer;
            }),
        qosFilterStatusWriters_.end());
    LOG(DEBUG, __FUNCTION__, " Writer removed. Active writers: ", qosFilterStatusWriters_.size());

    return grpc::Status::OK;
}

grpc::Status QoSServerImpl::DeRegisterOnQoSFilterStatusChange(ServerContext *context,
    const dataStub::DeRegisterNotificationRequest *request, google::protobuf::Empty *response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

void QoSServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    LOG(DEBUG, __FUNCTION__);
#ifdef null  // add if event injection needed in future
    if (message.filter() == QOS_EVENT) {
        std::string event = message.event();
        std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
        LOG(DEBUG, __FUNCTION__, " Token String is ", token);
        if (token == QOS_FILTER_STATUS_UPDATE_TOKEN) {
            handleThrottleApnEvent(event);
        } else {
            LOG(ERROR, __FUNCTION__, " Unknown Token! ");
        }
    }
#endif
}

// --- Helper Functions ---
// Helper for converting BandwidthConfig
Json::Value QoSServerImpl::convertBandwidthConfigToJson(
    const dataStub::BandwidthConfig &protoConfig) {
    Json::Value jsonConfig;
    jsonConfig["dlBandwidthConfigType"]
        = static_cast<uint32_t>(protoConfig.dl_bandwidth_config_type());
    Json::Value bandwidthValue;
    bandwidthValue["minBandwidth"] = protoConfig.dl_bandwidth_value().min_bandwidth();
    bandwidthValue["maxBandwidth"] = protoConfig.dl_bandwidth_value().max_bandwidth();
    jsonConfig["dlBandwidthValue"] = bandwidthValue;
    return jsonConfig;
}

void QoSServerImpl::convertJsonToBandwidthConfig(
    const Json::Value &jsonConfig, dataStub::BandwidthConfig *protoConfig) {
    protoConfig->set_dl_bandwidth_config_type(
        static_cast<dataStub::BandwidthConfigType>(jsonConfig["dlBandwidthConfigType"].asUInt()));
    protoConfig->mutable_dl_bandwidth_value()->set_min_bandwidth(
        jsonConfig["dlBandwidthValue"]["minBandwidth"].asUInt());
    protoConfig->mutable_dl_bandwidth_value()->set_max_bandwidth(
        jsonConfig["dlBandwidthValue"]["maxBandwidth"].asUInt());
}

// Conversions for ITcConfig
Json::Value QoSServerImpl::convertITcConfigToJson(const dataStub::ITcConfig &protoTcConfig) {
    Json::Value jsonTcConfig;
    jsonTcConfig["validityMask"] = protoTcConfig.validity_mask();
    jsonTcConfig["trafficClass"] = protoTcConfig.traffic_class();
    jsonTcConfig["direction"]    = static_cast<uint32_t>(protoTcConfig.direction());
    jsonTcConfig["dataPath"]     = static_cast<uint32_t>(protoTcConfig.data_path());
    if (protoTcConfig.validity_mask() & (1 << dataStub::TC_BANDWIDTH_CONFIG_VALID)) {
        jsonTcConfig["bandwidthConfig"]
            = convertBandwidthConfigToJson(protoTcConfig.bandwidth_config());
    }
    return jsonTcConfig;
}

void QoSServerImpl::convertJsonToITcConfig(
    const Json::Value &jsonTcConfig, dataStub::ITcConfig *protoTcConfig) {
    protoTcConfig->set_validity_mask(jsonTcConfig["validityMask"].asUInt());
    protoTcConfig->set_traffic_class(jsonTcConfig["trafficClass"].asUInt());
    protoTcConfig->set_direction(
        static_cast<dataStub::Direction>(jsonTcConfig["direction"].asUInt()));
    protoTcConfig->set_data_path(
        static_cast<dataStub::DataPath>(jsonTcConfig["dataPath"].asUInt()));
    if (jsonTcConfig.isMember("bandwidthConfig")) {
        convertJsonToBandwidthConfig(
            jsonTcConfig["bandwidthConfig"], protoTcConfig->mutable_bandwidth_config());
    }
}

// Helper for converting QoSFilterStatus
Json::Value QoSServerImpl::convertQoSFilterStatusToJson(
    const dataStub::QoSFilterStatus &protoStatus) {
    Json::Value jsonStatus;
    jsonStatus["ethStatus"]   = static_cast<uint32_t>(protoStatus.eth_status());
    jsonStatus["modemStatus"] = static_cast<uint32_t>(protoStatus.modem_status());
    jsonStatus["ipaStatus"]   = static_cast<uint32_t>(protoStatus.ipa_status());
    return jsonStatus;
}

void QoSServerImpl::convertJsonToQoSFilterStatus(
    const Json::Value &jsonStatus, dataStub::QoSFilterStatus *protoStatus) {
    protoStatus->set_eth_status(
        static_cast<dataStub::FilterInstallationStatus>(jsonStatus["ethStatus"].asUInt()));
    protoStatus->set_modem_status(
        static_cast<dataStub::FilterInstallationStatus>(jsonStatus["modemStatus"].asUInt()));
    protoStatus->set_ipa_status(
        static_cast<dataStub::FilterInstallationStatus>(jsonStatus["ipaStatus"].asUInt()));
}

// Conversions for ITrafficFilter
Json::Value QoSServerImpl::convertITrafficFilterToJson(
    const dataStub::ITrafficFilter &protoFilter) {
    Json::Value jsonFilter;
    // Store validity mask directly
    jsonFilter["validityMask"] = protoFilter.validity_mask();

    // Set common fields if valid
    if (protoFilter.validity_mask() & dataStub::TF_DIRECTION_VALID) {
        jsonFilter["direction"] = static_cast<uint32_t>(protoFilter.direction());
    }
    if (protoFilter.validity_mask() & dataStub::TF_IP_PROTOCOL_VALID) {
        jsonFilter["ipProtocol"] = protoFilter.ip_protocol();
    }
    if (protoFilter.validity_mask() & dataStub::TF_PCP_VALID) {
        jsonFilter["pcp"] = protoFilter.pcp();
    }
    if (protoFilter.validity_mask() & dataStub::TF_DATA_PATH_VALID) {
        jsonFilter["dataPath"] = static_cast<uint32_t>(protoFilter.data_path());
    }

    // Source Fields
    if (protoFilter.validity_mask() & dataStub::TF_SOURCE_IPV4_ADDRESS_VALID) {
        jsonFilter["sourceIpv4Address"] = protoFilter.source_ipv4_address();
    }
    if (protoFilter.validity_mask() & dataStub::TF_SOURCE_IPV6_ADDRESS_VALID) {
        jsonFilter["sourceIpv6Address"] = protoFilter.source_ipv6_address();
    }
    if (protoFilter.validity_mask() & dataStub::TF_SOURCE_PORT_VALID) {
        jsonFilter["sourcePort"] = protoFilter.source_port();
    }
    if (protoFilter.validity_mask() & dataStub::TF_SOURCE_PORT_RANGE_VALID) {
        jsonFilter["sourceStartPort"] = protoFilter.source_start_port();
        jsonFilter["sourcePortRange"] = protoFilter.source_port_range();
    }
    if (protoFilter.validity_mask() & dataStub::TF_SOURCE_PORT_CONFIG_VALID) {
        Json::Value portConfig;
        portConfig["port"]  = protoFilter.source_port_config().port();
        portConfig["range"] = protoFilter.source_port_config().range();
        portConfig["maxActiveConnections"]
            = protoFilter.source_port_config().max_active_connections();
        jsonFilter["sourcePortConfig"] = portConfig;
    }
    if (protoFilter.validity_mask() & dataStub::TF_SOURCE_VLAN_LIST_VALID) {
        Json::Value vlanList(Json::arrayValue);
        for (int vlan : protoFilter.source_vlan_list()) {
            vlanList.append(vlan);
        }
        jsonFilter["sourceVlanList"] = vlanList;
    }

    // Destination Fields
    if (protoFilter.validity_mask() & dataStub::TF_DESTINATION_IPV4_ADDRESS_VALID) {
        jsonFilter["destIpv4Address"] = protoFilter.dest_ipv4_address();
    }
    if (protoFilter.validity_mask() & dataStub::TF_DESTINATION_IPV6_ADDRESS_VALID) {
        jsonFilter["destIpv6Address"] = protoFilter.dest_ipv6_address();
    }
    if (protoFilter.validity_mask() & dataStub::TF_DESTINATION_PORT_VALID) {
        jsonFilter["destPort"] = protoFilter.dest_port();
    }
    if (protoFilter.validity_mask() & dataStub::TF_DESTINATION_PORT_RANGE_VALID) {
        jsonFilter["destStartPort"] = protoFilter.dest_start_port();
        jsonFilter["destPortRange"] = protoFilter.dest_port_range();
    }
    if (protoFilter.validity_mask() & dataStub::TF_DESTINATION_PORT_CONFIG_VALID) {
        Json::Value portConfig;
        portConfig["port"]  = protoFilter.dest_port_config().port();
        portConfig["range"] = protoFilter.dest_port_config().range();
        portConfig["maxActiveConnections"]
            = protoFilter.dest_port_config().max_active_connections();
        jsonFilter["destPortConfig"] = portConfig;
    }
    if (protoFilter.validity_mask() & dataStub::TF_DESTINATION_VLAN_LIST_VALID) {
        Json::Value vlanList(Json::arrayValue);
        for (int vlan : protoFilter.dest_vlan_list()) {
            vlanList.append(vlan);
        }
        jsonFilter["destVlanList"] = vlanList;
    }

    return jsonFilter;
}

void QoSServerImpl::convertJsonToITrafficFilter(
    const Json::Value &jsonFilter, dataStub::ITrafficFilter *protoFilter) {
    // Read validity mask
    uint32_t validityMask = 0;
    if (jsonFilter.isMember("validityMask")) {
        validityMask = jsonFilter["validityMask"].asUInt();
        protoFilter->set_validity_mask(validityMask);
    }

    // Set common fields if valid
    if ((validityMask & dataStub::TF_DIRECTION_VALID) && jsonFilter.isMember("direction")) {
        protoFilter->set_direction(
            static_cast<dataStub::Direction>(jsonFilter["direction"].asUInt()));
    }
    if ((validityMask & dataStub::TF_IP_PROTOCOL_VALID) && jsonFilter.isMember("ipProtocol")) {
        protoFilter->set_ip_protocol(jsonFilter["ipProtocol"].asUInt());
    }
    if ((validityMask & dataStub::TF_PCP_VALID) && jsonFilter.isMember("pcp")) {
        protoFilter->set_pcp(jsonFilter["pcp"].asInt());
    }
    if ((validityMask & dataStub::TF_DATA_PATH_VALID) && jsonFilter.isMember("dataPath")) {
        protoFilter->set_data_path(
            static_cast<dataStub::DataPath>(jsonFilter["dataPath"].asUInt()));
    }

    // Source Fields
    if ((validityMask & dataStub::TF_SOURCE_IPV4_ADDRESS_VALID)
        && jsonFilter.isMember("sourceIpv4Address")) {
        protoFilter->set_source_ipv4_address(jsonFilter["sourceIpv4Address"].asString());
    }
    if ((validityMask & dataStub::TF_SOURCE_IPV6_ADDRESS_VALID)
        && jsonFilter.isMember("sourceIpv6Address")) {
        protoFilter->set_source_ipv6_address(jsonFilter["sourceIpv6Address"].asString());
    }
    if ((validityMask & dataStub::TF_SOURCE_PORT_VALID) && jsonFilter.isMember("sourcePort")) {
        protoFilter->set_source_port(jsonFilter["sourcePort"].asUInt());
    }
    if ((validityMask & dataStub::TF_SOURCE_PORT_RANGE_VALID)
        && jsonFilter.isMember("sourceStartPort") && jsonFilter.isMember("sourcePortRange")) {
        protoFilter->set_source_start_port(jsonFilter["sourceStartPort"].asUInt());
        protoFilter->set_source_port_range(jsonFilter["sourcePortRange"].asUInt());
    }
    if ((validityMask & dataStub::TF_SOURCE_PORT_CONFIG_VALID)
        && jsonFilter.isMember("sourcePortConfig")) {
        const Json::Value &portConfig = jsonFilter["sourcePortConfig"];
        protoFilter->mutable_source_port_config()->set_port(portConfig["port"].asUInt());
        protoFilter->mutable_source_port_config()->set_range(portConfig["range"].asUInt());
        protoFilter->mutable_source_port_config()->set_max_active_connections(
            portConfig["maxActiveConnections"].asUInt());
    }
    if ((validityMask & dataStub::TF_SOURCE_VLAN_LIST_VALID)
        && jsonFilter.isMember("sourceVlanList")) {
        for (const Json::Value &vlan : jsonFilter["sourceVlanList"]) {
            protoFilter->add_source_vlan_list(vlan.asInt());
        }
    }

    // Destination Fields
    if ((validityMask & dataStub::TF_DESTINATION_IPV4_ADDRESS_VALID)
        && jsonFilter.isMember("destIpv4Address")) {
        protoFilter->set_dest_ipv4_address(jsonFilter["destIpv4Address"].asString());
    }
    if ((validityMask & dataStub::TF_DESTINATION_IPV6_ADDRESS_VALID)
        && jsonFilter.isMember("destIpv6Address")) {
        protoFilter->set_dest_ipv6_address(jsonFilter["destIpv6Address"].asString());
    }
    if ((validityMask & dataStub::TF_DESTINATION_PORT_VALID) && jsonFilter.isMember("destPort")) {
        protoFilter->set_dest_port(jsonFilter["destPort"].asUInt());
    }
    if ((validityMask & dataStub::TF_DESTINATION_PORT_RANGE_VALID)
        && jsonFilter.isMember("destStartPort") && jsonFilter.isMember("destPortRange")) {
        protoFilter->set_dest_start_port(jsonFilter["destStartPort"].asUInt());
        protoFilter->set_dest_port_range(jsonFilter["destPortRange"].asUInt());
    }
    if ((validityMask & dataStub::TF_DESTINATION_PORT_CONFIG_VALID)
        && jsonFilter.isMember("destPortConfig")) {
        const Json::Value &portConfig = jsonFilter["destPortConfig"];
        protoFilter->mutable_dest_port_config()->set_port(portConfig["port"].asUInt());
        protoFilter->mutable_dest_port_config()->set_range(portConfig["range"].asUInt());
        protoFilter->mutable_dest_port_config()->set_max_active_connections(
            portConfig["maxActiveConnections"].asUInt());
    }
    if ((validityMask & dataStub::TF_DESTINATION_VLAN_LIST_VALID)
        && jsonFilter.isMember("destVlanList")) {
        for (const Json::Value &vlan : jsonFilter["destVlanList"]) {
            protoFilter->add_dest_vlan_list(vlan.asInt());
        }
    }
}

// Conversions for IQoSFilter
Json::Value QoSServerImpl::convertIQoSFilterToJson(const dataStub::IQoSFilter &protoQoSFilter) {
    Json::Value jsonQoSFilter;
    jsonQoSFilter["handle"]        = protoQoSFilter.handle();
    jsonQoSFilter["trafficClass"]  = protoQoSFilter.traffic_class();
    jsonQoSFilter["trafficFilter"] = convertITrafficFilterToJson(protoQoSFilter.traffic_filter());
    jsonQoSFilter["status"]        = convertQoSFilterStatusToJson(protoQoSFilter.status());
    return jsonQoSFilter;
}

void QoSServerImpl::syncQoSFilterStatus(dataStub::DataPath dataPath, Json::Value &jsonStatus) {
    if ((dataPath == dataStub::DataPath::TETHERED_TO_WAN_HW)
        || (dataPath == dataStub::DataPath::APPS_TO_WAN)) {
        if ((dcmServerImpl_->isAnyDataCallActive(SLOT_ID_1)
                || dcmServerImpl_->isAnyDataCallActive(SLOT_ID_2))) {
            LOG(DEBUG, __FUNCTION__, " as data call active modem status is SUCCESS");
            jsonStatus["modemStatus"]
                = static_cast<uint32_t>(dataStub::FilterInstallationStatus::SUCCESS);
        } else {
            LOG(DEBUG, __FUNCTION__, " as data call not active modem status is PENDING");
            jsonStatus["modemStatus"]
                = static_cast<uint32_t>(dataStub::FilterInstallationStatus::PENDING);
        }
    }
}

void QoSServerImpl::convertJsonToIQoSFilter(
    const Json::Value &jsonQoSFilter, dataStub::IQoSFilter *protoQoSFilter) {
    protoQoSFilter->set_handle(jsonQoSFilter["handle"].asUInt());
    protoQoSFilter->set_traffic_class(jsonQoSFilter["trafficClass"].asUInt());
    convertJsonToITrafficFilter(
        jsonQoSFilter["trafficFilter"], protoQoSFilter->mutable_traffic_filter());
    dataStub::DataPath dataPath
        = static_cast<dataStub::DataPath>(jsonQoSFilter["trafficFilter"]["dataPath"].asUInt());
    // Update current modem status as per active data call
    Json::Value currentStatusJson = jsonQoSFilter["status"];
    syncQoSFilterStatus(dataPath, currentStatusJson);
    convertJsonToQoSFilterStatus(currentStatusJson, protoQoSFilter->mutable_status());
}

void QoSServerImpl::onEventUpdate(google::protobuf::Any event) {
    LOG(DEBUG, __FUNCTION__);
    if (event.Is<::dataStub::StartDataCallEvent>()) {
        ::dataStub::StartDataCallEvent startEvent;
        event.UnpackTo(&startEvent);
        LOG(DEBUG, __FUNCTION__, " QoSServerImpl : StartDataCallEvent ");
        handleStartDataCallEvent(startEvent);
    } else if (event.Is<::dataStub::StopDataCallEvent>()) {
        ::dataStub::StopDataCallEvent stopEvent;
        event.UnpackTo(&stopEvent);
        LOG(DEBUG, __FUNCTION__, " QoSServerImpl : StopDataCallEvent ");
        handleStopDataCallEvent(stopEvent);
    }
}

void QoSServerImpl::handleStartDataCallEvent(::dataStub::StartDataCallEvent startEvent) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "getAllTrafficClasses";
    telux::common::JsonData data;

    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to read QoS Manager state JSON for events.");
        return;
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value &qos_filters_array = data.stateRootObj[subsystem]["qosFilters"];
        bool stateChanged              = false;

        for (Json::ValueIterator it = qos_filters_array.begin(); it != qos_filters_array.end();
             ++it) {
            Json::Value &filterJson = *it;  // Get mutable reference to current filter
            if (filterJson.isMember("trafficFilter")
                && filterJson["trafficFilter"].isMember("dataPath") && filterJson.isMember("status")
                && filterJson["status"].isMember("modemStatus")) {

                dataStub::DataPath dataPath = static_cast<dataStub::DataPath>(
                    filterJson["trafficFilter"]["dataPath"].asUInt());
                uint32_t currentModemStatus = filterJson["status"]["modemStatus"].asUInt();

                if ((dataPath == dataStub::DataPath::APPS_TO_WAN
                        || dataPath == dataStub::DataPath::TETHERED_TO_WAN_HW)
                    && currentModemStatus
                           == static_cast<uint32_t>(dataStub::FilterInstallationStatus::PENDING)) {

                    LOG(DEBUG, __FUNCTION__, " Updating modem status for filter handle ",
                        filterJson["handle"].asUInt(), " to SUCCESS due to StartDataCallEvent.");
                    filterJson["status"]["modemStatus"]
                        = static_cast<uint32_t>(dataStub::FilterInstallationStatus::SUCCESS);
                    stateChanged = true;

                    // Publish an unsolicited event for this filter's status change
                    ::eventService::EventResponse anyResponse;
                    ::dataStub::QoSFilterStatusChangeEvent qosFilterStatusChangeEvent;
                    qosFilterStatusChangeEvent.set_handle(filterJson["handle"].asUInt());
                    convertJsonToQoSFilterStatus(
                        filterJson["status"], qosFilterStatusChangeEvent.mutable_status());
                    anyResponse.set_filter(DATA_QOS_FILTER);  // Assuming DATA_QOS_FILTER
                                                              // is defined and correct
                    anyResponse.mutable_any()->PackFrom(qosFilterStatusChangeEvent);
                    EventService::getInstance().updateEventQueue(anyResponse);
                }
            } else {
                LOG(WARNING, __FUNCTION__,
                    "Malformed QoS filter entry found during StartDataCallEvent "
                    "processing.");
            }
        }
        if (stateChanged) {
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }
}

void QoSServerImpl::handleStopDataCallEvent(::dataStub::StopDataCallEvent stopEvent) {
    LOG(DEBUG, __FUNCTION__);

    std::string apiJsonPath   = QOS_MANAGER_API_LOCAL_JSON;
    std::string stateJsonPath = QOS_MANAGER_STATE_JSON;
    std::string subsystem     = "IQoSManager";
    std::string method        = "getAllTrafficClasses";
    telux::common::JsonData data;
    telux::common::ErrorCode error = telux::common::CommonUtils::readJsonData(
        apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Failed to read QoS Manager state JSON for events.");
        return;
    }

    if (data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value &qos_filters_array = data.stateRootObj[subsystem]["qosFilters"];
        bool stateChanged              = false;

        // Check if any data call is still active. If so, don't set modem status to
        // PENDING. We only want to set to PENDING if ALL data calls are down.
        bool anyDataCallStillActive = dcmServerImpl_->isAnyDataCallActive(SLOT_ID_1)
                                      || dcmServerImpl_->isAnyDataCallActive(SLOT_ID_2);

        for (Json::ValueIterator it = qos_filters_array.begin(); it != qos_filters_array.end();
             ++it) {
            Json::Value &filterJson = *it;  // Get mutable reference to current filter
            if (filterJson.isMember("trafficFilter")
                && filterJson["trafficFilter"].isMember("dataPath") && filterJson.isMember("status")
                && filterJson["status"].isMember("modemStatus")) {

                dataStub::DataPath dataPath = static_cast<dataStub::DataPath>(
                    filterJson["trafficFilter"]["dataPath"].asUInt());
                uint32_t currentModemStatus = filterJson["status"]["modemStatus"].asUInt();

                if ((dataPath == dataStub::DataPath::APPS_TO_WAN
                        || dataPath == dataStub::DataPath::TETHERED_TO_WAN_HW)
                    && currentModemStatus
                           == static_cast<uint32_t>(dataStub::FilterInstallationStatus::SUCCESS)
                    && !anyDataCallStillActive) {  // Only set to PENDING if no data calls
                                                   // are active

                    LOG(DEBUG, __FUNCTION__, " Updating modem status for filter handle ",
                        filterJson["handle"].asUInt(),
                        " to PENDING due to StopDataCallEvent (no active data calls).");
                    filterJson["status"]["modemStatus"]
                        = static_cast<uint32_t>(dataStub::FilterInstallationStatus::PENDING);
                    stateChanged = true;

                    // Publish an unsolicited event for this filter's status change
                    ::eventService::EventResponse anyResponse;
                    ::dataStub::QoSFilterStatusChangeEvent qosFilterStatusChangeEvent;
                    qosFilterStatusChangeEvent.set_handle(filterJson["handle"].asUInt());
                    convertJsonToQoSFilterStatus(
                        filterJson["status"], qosFilterStatusChangeEvent.mutable_status());
                    anyResponse.set_filter(DATA_QOS_FILTER);  // Assuming DATA_QOS_FILTER
                                                              // is defined and correct
                    anyResponse.mutable_any()->PackFrom(qosFilterStatusChangeEvent);
                    EventService::getInstance().updateEventQueue(anyResponse);
                }
            } else {
                LOG(WARNING, __FUNCTION__,
                    "Malformed QoS filter entry found during StopDataCallEvent "
                    "processing.");
            }
        }
        if (stateChanged) {
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }
}