/*
* Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
* SPDX-License-Identifier: BSD-3-Clause-Clear
*/

#include <sstream>
#include <telux/tel/CellInfo.hpp>
#include <telux/tel/ECallDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/SignalStrength.hpp>
#include <telux/tel/VoiceServiceInfo.hpp>
#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "libs/common/Logger.hpp"
#include "PhoneManagerServerImpl.hpp"

#define JSON_PATH1 "api/tel/IPhoneManagerSlot1.json"
#define JSON_PATH2 "api/tel/IPhoneManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/IPhoneManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/IPhoneManagerStateSlot2.json"

#define TEL_PHONE_MANAGER                       "IPhoneManager"
#define PHONE_EVENT_SIGNAL_STRENGTH_CHANGE      "signalStrengthUpdate"
#define PHONE_EVENT_CELL_INFO_CHANGE            "cellInfoListUpdate"
#define PHONE_EVENT_VOICE_SERVICE_STATE_CHANGE  "voiceServiceStateUpdate"
#define PHONE_EVENT_OPERATING_MODE_CHANGE       "operatingModeUpdate"
#define PHONE_EVENT_ECALL_OPERATING_MODE_CHANGE "eCallOperatingModeUpdate"
#define PHONE_EVENT_OPERATOR_INFO_CHANGE        "operatorInfoUpdate"

#define SLOT_1 1
#define SLOT_2 2
#define DEFAULT_SLOT_ID SLOT_1
#define ECALL_MODE_REASON_NORMAL 0

PhoneManagerServerImpl::PhoneManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

PhoneManagerServerImpl::~PhoneManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

telux::common::ServiceStatus PhoneManagerServerImpl::readSubsystemStatus(int slotId) {
    int cbDelay = 0;
    return readSubsystemStatus(slotId, cbDelay);
}

telux::common::ServiceStatus PhoneManagerServerImpl::readSubsystemStatus(int slotId, int &cbDelay) {
    Json::Value rootObj;
    std::string filePath = (slotId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return telux::common::ServiceStatus::SERVICE_FAILED;
    }

    cbDelay = rootObj[TEL_PHONE_MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus = rootObj[TEL_PHONE_MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus, " slotId::", slotId);
    return status;
}

telux::common::ErrorCode PhoneManagerServerImpl::readJsonData(int slotId, std::string method,
    JsonData &data) {
    std::string apiJsonPath = (slotId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string subsystem = TEL_PHONE_MANAGER;
    std::string stateJsonPath = (slotId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    return CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
}

telux::common::ErrorCode  PhoneManagerServerImpl::readJsonData(int slotId, std::string method,
    JsonData &data, std::string &stateJsonPath) {
    std::string apiJsonPath = (slotId == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string subsystem = TEL_PHONE_MANAGER;
    stateJsonPath = (slotId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    return CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);
}

grpc::Status PhoneManagerServerImpl::InitService(ServerContext* context,
    const ::google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    int cbDelay = 0;
    telux::common::ServiceStatus status = readSubsystemStatus(DEFAULT_SLOT_ID, cbDelay);
    if (status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_PHONE_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    } else {
        LOG(ERROR, __FUNCTION__, " Json not found or service not available or failed");
        return grpc::Status(grpc::StatusCode::INTERNAL,
            " Json not found or service not available or failed");
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::google::protobuf::Empty* request, commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ServiceStatus status = readSubsystemStatus(DEFAULT_SLOT_ID);
    if (status != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
        return grpc::Status::OK;
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::IsSubsystemReady(ServerContext* context,
    const google::protobuf::Empty *request, commonStub::IsSubsystemReadyReply* response) {
    LOG(DEBUG, __FUNCTION__);
    bool status = false;
    telux::common::ServiceStatus servstatus = readSubsystemStatus(DEFAULT_SLOT_ID);
    if (servstatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        response->set_is_ready(status);
        return grpc::Status::OK;
    } else {
        status = true;
    }
    response->set_is_ready(status);
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetPhoneIds(ServerContext *context,
    const google::protobuf::Empty *request, telStub::GetPhoneIdsReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID,
        "getPhoneIds", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetPhoneId(ServerContext *context,
    const google::protobuf::Empty *request, telStub::GetPhoneIdReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID,
        "getPhoneId", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetCellularCapabilities(ServerContext* context,
    const google::protobuf::Empty *request, telStub::CellularCapabilityInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID,
        "requestCellularCapabilityInfo", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        int size = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
            ["voiceTech"].size();
        for (auto index = 0; index < size; index++) {
            response->add_voice_service_techs(convertVoiceTechStringToEnum(
                data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]["voiceTech"]\
                [index].asString()));
        }
        response->set_sim_count(data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
            ["simCount"].asInt());
        response->set_max_active_sims(data.stateRootObj[TEL_PHONE_MANAGER]\
            ["cellularCapabilityInfo"]["maxActiveSims"].asInt());
        int simRATCapSize = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
            ["SimRATCapabilities"].size();
        for (auto i = 0; i < simRATCapSize; i++) {
            telStub::SimRatCapability *simCaps = response->add_sim_rat_capabilities();
            simCaps->set_slot_id(data.stateRootObj[TEL_PHONE_MANAGER]\
                ["cellularCapabilityInfo"]["SimRATCapabilities"][i]["slotId"].asInt());
            int ratCapSize = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
                ["SimRATCapabilities"][i]["capabilities"].size();
            for (auto j = 0; j < ratCapSize; j++) {
                simCaps->add_capabilities(
                convertRATCapStringToEnum(data.stateRootObj[TEL_PHONE_MANAGER]\
                    ["cellularCapabilityInfo"]["SimRATCapabilities"][i]["capabilities"]\
                    [j].asString()));
            }
        }
        int deviceRATCapSize = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
            ["DeviceRATCapabilities"].size();
        for (auto i = 0; i < deviceRATCapSize; i++) {
            telStub::SimRatCapability *deviceCaps = response->add_device_rat_capability();
            deviceCaps->set_slot_id(data.stateRootObj[TEL_PHONE_MANAGER]\
                ["cellularCapabilityInfo"]["DeviceRATCapabilities"][i]["slotId"].asInt());
            int sizeDeviceCap = data.stateRootObj[TEL_PHONE_MANAGER]["cellularCapabilityInfo"]\
                ["DeviceRATCapabilities"][i]["capabilities"].size();
            for (auto j = 0; j < sizeDeviceCap; j++) {
                deviceCaps->add_capabilities(
                convertRATCapStringToEnum(data.stateRootObj[TEL_PHONE_MANAGER]\
                    ["cellularCapabilityInfo"]\
                    ["DeviceRATCapabilities"][i]["capabilities"][j].asString()));
            }
        }
    }
    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::SetOperatingMode(ServerContext* context,
    const ::telStub::SetOperatingModeRequest* request, telStub::SetOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    std::string stateJsonPath = "";

    if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID, "setOperatingMode", data,
        stateJsonPath)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    ::telStub::OperatingMode mode = request->operating_mode();
    int operatingMode = static_cast<int>(mode);
    if (data.status == telux::common::Status::SUCCESS) {
        if (operatingMode < (static_cast<int>(telux::tel::OperatingMode::ONLINE)) ||
            operatingMode > (static_cast<int>(telux::tel::OperatingMode::PERSISTENT_LOW_POWER))) {
            LOG(ERROR, __FUNCTION__, " Invalid operating mode");
            response->set_error(static_cast<commonStub::ErrorCode>(
                commonStub::ErrorCode::INVALID_ARGUMENTS));
        } else {
            notifyAndUpdateOperatingMode(operatingMode);
            response->set_error(static_cast<commonStub::ErrorCode>(data.error));
        }
    }

    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }

    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetOperatingMode(ServerContext* context,
    const google::protobuf::Empty* request, telStub::GetOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID, "requestOperatingMode", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        int opMode = data.stateRootObj[TEL_PHONE_MANAGER]["operatingModeInfo"]\
            ["operatingMode"].asInt();
        telux::tel::OperatingMode operatingMode = static_cast<telux::tel::OperatingMode>(opMode);
        switch(operatingMode) {
            case telux::tel::OperatingMode::ONLINE:
                response->set_operating_mode(telStub::OperatingMode::ONLINE);
                break;
            case telux::tel::OperatingMode::AIRPLANE:
                response->set_operating_mode(telStub::OperatingMode::AIRPLANE);
                break;
            case telux::tel::OperatingMode::FACTORY_TEST:
                response->set_operating_mode(telStub::OperatingMode::FACTORY_TEST);
                break;
            case telux::tel::OperatingMode::OFFLINE:
                response->set_operating_mode(telStub::OperatingMode::OFFLINE);
                break;
            case telux::tel::OperatingMode::RESETTING:
                response->set_operating_mode(telStub::OperatingMode::RESETTING);
                break;
            case telux::tel::OperatingMode::SHUTTING_DOWN:
                response->set_operating_mode(telStub::OperatingMode::SHUTTING_DOWN);
                break;
            case telux::tel::OperatingMode::PERSISTENT_LOW_POWER:
                response->set_operating_mode(telStub::OperatingMode::PERSISTENT_LOW_POWER);
                break;
            default:
                response->set_operating_mode(telStub::OperatingMode::ONLINE);
                break;
        }
    }
    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::ResetWwan(ServerContext* context,
    const google::protobuf::Empty* request, telStub::ResetWwanReply* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(DEFAULT_SLOT_ID, "resetWwan", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " Data Status is Success");
    }
    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::RequestVoiceServiceState(ServerContext* context,
    const telStub::RequestVoiceServiceStateRequest* request,
    telStub::RequestVoiceServiceStateReply* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(request->phone_id(), "requestVoiceServiceState",
        data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        telStub::VoiceServiceState voiceServiceState = static_cast<telStub::VoiceServiceState>(
            data.stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
            ["voiceServiceState"].asInt());
        LOG(DEBUG, __FUNCTION__," VoiceServiceState is :", static_cast<int>(voiceServiceState));
        response->set_voice_service_state(voiceServiceState);
        telStub::VoiceServiceDenialCause voiceServiceDenialCause =
            static_cast<telStub::VoiceServiceDenialCause>(data.stateRootObj[TEL_PHONE_MANAGER]\
            ["voiceServiceStateInfo"]["voiceServiceDenialCause"].asInt());
        LOG(DEBUG, __FUNCTION__," VoiceServiceDenialCause is :",
                static_cast<int>(voiceServiceDenialCause));
        response->set_voice_service_denial_cause(voiceServiceDenialCause);
        telStub::RadioTechnology radioTech = static_cast<telStub::RadioTechnology>(
            data.stateRootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["radioTech"].asInt());
        LOG(DEBUG, __FUNCTION__," RadioTech is :", static_cast<int>(radioTech));
        response->set_radio_technology(radioTech);
    }

    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::SetRadioPower(ServerContext* context,
    const telStub::SetRadioPowerRequest* request, telStub::SetRadioPowerReply* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    std::string stateJsonPath = "";
    if (ErrorCode::SUCCESS != readJsonData(request->phone_id(), "setRadioPower", data,
        stateJsonPath)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        data.stateRootObj[TEL_PHONE_MANAGER]["radioPowerState"]["enable"] = request->enable();
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    // TODO resetting voice service state, signal strength values
    int phoneId = request->phone_id();
    bool enable = request->enable();
    if (!enable) {
        updateJsonForDefaultValues(phoneId, true);
    }

    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::RequestCellInfoList(ServerContext* context,
    const telStub::RequestCellInfoListRequest* request,
    telStub::RequestCellInfoListResponse* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(request->phone_id(), "requestCellInfo", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        // Create response
        int newCellCount = data.stateRootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"].size();
        LOG(DEBUG, __FUNCTION__, " newCellCount: ", newCellCount);
        for (int i = 0 ; i < newCellCount; i++) {
            telStub::CellInfoList *cellInfo = response->add_cell_info_list();
            Json::Value requestedCell =
                data.stateRootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"][i];
            int cell = requestedCell["cellType"].asInt();
            telux::tel::CellType cellType = static_cast<telux::tel::CellType>(cell);
            cellInfo->mutable_cell_type()->set_cell_type(
                static_cast<telStub::CellInfo_CellType>(cell));
            // whether any cell registered
            cellInfo->mutable_cell_type()->set_registered(requestedCell["registered"].asInt());
            switch(cellType) {
                case telux::tel::CellType::GSM: {
                    // cell identity
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_mcc(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_mnc(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_lac(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["lac"].asInt());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_cid(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["cid"].asInt());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_arfcn(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["arfcn"].asInt());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_bsic(
                        requestedCell["gsmCellInfo"]["gsmCellIdentity"]["bsic"].asInt());
                    // signal strength
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                        set_gsm_signal_strength(requestedCell["gsmCellInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt());
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                        set_gsm_bit_error_rate(requestedCell["gsmCellInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmBitErrorRate"].asInt());
                    break;
                }
                case telux::tel::CellType::LTE: {
                    // cell identity
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_mcc(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_mnc(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_ci(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["ci"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_pci(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["pci"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_tac(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["tac"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_earfcn(
                        requestedCell["lteCellInfo"]["lteCellIdentity"]["earfcn"].asInt());
                    // signal strength
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_signal_strength(requestedCell["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteSignalStrength"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rsrp(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["lteRsrp"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rsrq(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["lteRsrq"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rssnr(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["lteRssnr"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_cqi(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["lteCqi"].asInt());
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_timing_advance(requestedCell["lteCellInfo"]["lteSignalStrengthInfo"]\
                        ["timingAdvance"].asInt());
                    break;
                }
                case telux::tel::CellType::WCDMA: {
                    // cell identity
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_mcc(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_mnc(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_lac(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["lac"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_cid(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["cid"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_psc(
                        requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["psc"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->set_uarfcn
                        (requestedCell["wcdmaCellInfo"]["wcdmaCellIdentity"]["uarfcn"].asInt());
                    // signal strength
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_signal_strength(requestedCell["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["signalStrength"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_bit_error_rate(requestedCell["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["bitErrorRate"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_bit_error_rate(requestedCell["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["ecio"].asInt());
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_bit_error_rate(requestedCell["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["rscp"].asInt());
                    break;
                }
                case telux::tel::CellType::NR5G: {
                    // cell identity
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_mcc(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["mcc"].asString());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_mnc(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["mnc"].asString());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_ci(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["ci"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_pci(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["pci"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_tac(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["tac"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_arfcn(
                        requestedCell["nr5gCellInfo"]["nr5gCellIdentity"]["arfcn"].asInt());
                    // signal strength
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rsrp(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                        ["rsrp"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rsrq(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                        ["rsrq"].asInt());
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rssnr(requestedCell["nr5gCellInfo"]["nr5gSignalStrengthInfo"]\
                        ["rssnr"].asInt());
                    break;
                }
                case telux::tel::CellType::CDMA:
                case telux::tel::CellType::TDSCDMA:
                default:
                    LOG(DEBUG, " Deprecated or Invalid type");
                    break;
            } // end switch
        } // end for loop
    }

    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::SetCellInfoListRate(ServerContext* context,
    const telStub::SetCellInfoListRateRequest* request,
    telStub::SetCellInfoListRateReply* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    std::string stateJsonPath = "";
    int time_internal = request->cell_info_rate();
    if (ErrorCode::SUCCESS != readJsonData(request->phone_id(), "setCellInfoListRate", data,
        stateJsonPath)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        data.stateRootObj[TEL_PHONE_MANAGER]["cellInfoListRate"]["timeInterval"] = time_internal;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetSignalStrength(ServerContext* context,
    const telStub::GetSignalStrengthRequest* request,
    telStub::GetSignalStrengthResponse* response) {

    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(request->phone_id(), "requestSignalStrength", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        // Create response
        // gsm signal strength
	    response->mutable_gsm_signal_strength_info()->set_gsm_signal_strength(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]
                ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt());
	    response->mutable_gsm_signal_strength_info()->set_gsm_bit_error_rate(
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]\
                ["gsmBitErrorRate"].asInt());
        LOG(DEBUG, __FUNCTION__, " gsmSignalStrength:",
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]
                ["gsmSignalStrengthInfo"]["gsmSignalStrength"].asInt(), " gsmBitErrorRate",
                data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]\
                ["gsmBitErrorRate"].asInt());
        // lte signal strength
        response->mutable_lte_signal_strength_info()->set_lte_signal_strength(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteSignalStrength"].asInt());
        response->mutable_lte_signal_strength_info()->set_lte_rsrp(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRsrp"].asInt());
        response->mutable_lte_signal_strength_info()->set_lte_rsrq(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRsrq"].asInt());
        response->mutable_lte_signal_strength_info()->set_lte_rssnr(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRssnr"].asInt());
        response->mutable_lte_signal_strength_info()->set_lte_cqi(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteCqi"].asInt());
        response->mutable_lte_signal_strength_info()->set_timing_advance(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["timingAdvance"].asInt());
        LOG(DEBUG, __FUNCTION__, " lteSignalStrength:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteSignalStrength"].asInt(), " lteRsrp:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRsrp"].asInt(), " lteRsrq:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRssnr"].asInt(), " lteCqi:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["lteRssnr"].asInt(), " timingAdvance:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
            ["timingAdvance"].asInt());
        // wcdma signal strength
        response->mutable_wcdma_signal_strength_info()->set_signal_strength(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
            ["signalStrength"].asInt());
        response->mutable_wcdma_signal_strength_info()->set_bit_error_rate(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
            ["bitErrorRate"].asInt());
        response->mutable_wcdma_signal_strength_info()->set_signal_strength(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
            ["ecio"].asInt());
        response->mutable_wcdma_signal_strength_info()->set_bit_error_rate(
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
            ["rscp"].asInt());
        LOG(DEBUG, __FUNCTION__, " wcdmaSignalStrength:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
            ["signalStrength"].asInt(), " bitErrorRate:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
            ["bitErrorRate"].asInt(), "ecio:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
            ["ecio"].asInt(), "rscp:",
            data.stateRootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
            ["rscp"].asInt());
        // nr5g signal strength
        response->mutable_nr5g_signal_strength_info()->set_rsrp(
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"].asInt());
        response->mutable_nr5g_signal_strength_info()->set_rsrq(
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"].asInt());
        response->mutable_nr5g_signal_strength_info()->set_rssnr(
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rssnr"].asInt());
        LOG(DEBUG, __FUNCTION__, " nr5grsrp:",
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"].asInt(), " nr5grsrq",
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"].asInt(), " nr5grssnr",
            data.stateRootObj[TEL_PHONE_MANAGER]\
            ["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rssnr"].asInt());
    }

    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::SetECallOperatingMode(ServerContext* context,
    const telStub::SetECallOperatingModeRequest* request,
    telStub::SetECallOperatingModeReply* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    std::string stateJsonPath = "";
    int phoneId = request->phone_id();
    if (ErrorCode::SUCCESS != readJsonData(phoneId, "setECallOperatingMode",
        data, stateJsonPath)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    int ecallMode = static_cast<int>(request->ecall_mode());
    if (data.status == telux::common::Status::SUCCESS) {
        if (ecallMode < (static_cast<int>(telux::tel::ECallMode::NORMAL)) ||
            ecallMode > (static_cast<int>(telux::tel::ECallMode::ECALL_ONLY))) {
            LOG(ERROR, __FUNCTION__, " Invalid eCall operating mode");
            response->set_error(static_cast<commonStub::ErrorCode>(
                commonStub::ErrorCode::NOT_SUPPORTED));
        } else {
            int ecallModeReason = ECALL_MODE_REASON_NORMAL;
            notifyAndUpdateEcallMode(phoneId, ecallMode, ecallModeReason);
            response->set_error(static_cast<commonStub::ErrorCode>(data.error));
        }
    }
    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::GetECallOperatingMode(ServerContext* context,
    const telStub::GetECallOperatingModeRequest* request,
    telStub::GetECallOperatingModeReply* response) {

    LOG(DEBUG, __FUNCTION__);
    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(request->phone_id(), "requestECallOperatingMode",
        data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed");
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        int ecallMode = data.stateRootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]\
        ["ecallMode"].asInt();
        response->set_ecall_mode(static_cast<telStub::ECallMode>(ecallMode));
    }
    //Update response
    if (data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::RequestOperatorInfo(ServerContext* context,
    const telStub::RequestOperatorInfoRequest* request,
    telStub::RequestOperatorInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    if (ErrorCode::SUCCESS != readJsonData(request->phone_id(), "requestOperatorInfo", data)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    if (data.status == telux::common::Status::SUCCESS) {
        std::string operatorLongName = data.stateRootObj[TEL_PHONE_MANAGER]\
            ["operatorNameInfo"]["longName"].asString();
        std::string operatorShortName = data.stateRootObj[TEL_PHONE_MANAGER]\
            ["operatorNameInfo"]["shortName"].asString();
        std::string plmn = data.stateRootObj[TEL_PHONE_MANAGER]\
            ["operatorNameInfo"]["plmn"].asString();
        bool home = data.stateRootObj[TEL_PHONE_MANAGER]\
            ["operatorNameInfo"]["home"].asBool();
        response->mutable_plmn_info()->set_long_name(operatorLongName);
        response->mutable_plmn_info()->set_short_name(operatorShortName);
        response->mutable_plmn_info()->set_plmn(plmn);
        response->mutable_plmn_info()->set_ishome(home);
    }

    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

grpc::Status PhoneManagerServerImpl::ConfigureSignalStrength(ServerContext* context,
    const telStub::ConfigureSignalStrengthRequest* request,
    telStub::ConfigureSignalStrengthReply* response) {
    LOG(DEBUG, __FUNCTION__);

    JsonData data;
    std::string stateJsonPath = "";
    if (ErrorCode::SUCCESS != readJsonData(request->slot_id(), "configureSignalStrength",
        data, stateJsonPath)) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return grpc::Status(grpc::StatusCode::INTERNAL, " Json read failed");
    }

    std::vector<telStub::ConfigureSignalStrength> signalStrengthConfig = {};
    for (auto config : request->config()) {
        signalStrengthConfig.emplace_back(config);
    }
    if (data.status == telux::common::Status::SUCCESS) {
        int currentCount = 0;
        for (unsigned int i = 0; i < signalStrengthConfig.size(); i++) {
            Json::Value newconfig;
            /* TODO check the RAT stored and user requested RAT and update it. currently
                without check it is written to json */
            currentCount = data.stateRootObj[TEL_PHONE_MANAGER]\
                ["configureSignalStrengthInfo"].size();
            LOG(DEBUG, __FUNCTION__," current configcount is : ", currentCount);
            newconfig["radioSignalType"] = signalStrengthConfig[i].rat_sig_type();
            newconfig["configType"] = signalStrengthConfig[i].config_type();
            switch(signalStrengthConfig[i].config_type()) {
                case telStub::SignalStrengthConfigType::DELTA:
                        newconfig["delta"] = signalStrengthConfig[i].delta();
                    break;
                case telStub::SignalStrengthConfigType::THRESHOLD:
                    newconfig["lowerThreshold"] = signalStrengthConfig[i].mutable_threshold()->
                        lower_range_threshold();
                    newconfig["upperThreshold"] = signalStrengthConfig[i].mutable_threshold()->
                        upper_range_threshold();
                    break;
                default:
                    break;
            }

            bool ratFound = false;
            for (int j = 0; j < currentCount; j++) {
                if ((data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthInfo"][j]\
                    ["radioSignalType"]) == newconfig["radioSignalType"]) {
                        LOG(DEBUG, __FUNCTION__, " Matched RAT");
                        data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthInfo"]\
                            [j] = newconfig;
                    ratFound = true;
                    break;
                }
            }

            if (ratFound) {
                LOG(DEBUG, __FUNCTION__, " Matching RAT found");
                continue;
            }
            data.stateRootObj[TEL_PHONE_MANAGER]["configureSignalStrengthInfo"]\
                [currentCount] = newconfig;
        }
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }
    //Update response
    if(data.cbDelay != -1) {
        response->set_iscallback(true);
    } else {
        response->set_iscallback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));
    return grpc::Status::OK;
}

void PhoneManagerServerImpl::handleSignalStrengthChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    // Split the event string into parameters( for phoneId, SignalStrength1, SignalStrength2 ...)
    // based on delimeter as ","
    std::stringstream ss(eventParams);
    std::vector<string> params;
    while (getline(ss, eventParams, ',')) {
        params.emplace_back(eventParams);
    }

    ::telStub::SignalStrengthChangeEvent signalStrengthChangeEvent;
    ::eventService::EventResponse anyResponse;
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(params[0], DEFAULT_DELIMITER);
        int phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }

        Json::Value rootObj;
        std::string jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return;
        }

        rootObj[TEL_PHONE_MANAGER] ["signalStrengthInfo"].clear();

        signalStrengthChangeEvent.set_phone_id(phoneId);
        int signalStrengthInfoCount = params.size() - 1;
        for (int index = 1; index <= signalStrengthInfoCount; index++) {
            std::string rat = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
            LOG(DEBUG, __FUNCTION__, " RAT Type is:", rat);
            if (rat == "GSM") {
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int signalStrength = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int bitErrorRate = std::stoi(token);
                LOG(DEBUG, __FUNCTION__," signalStrength:", signalStrength," bitErrorRate:",
                    bitErrorRate);

                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]\
                    ["gsmSignalStrength"] = signalStrength;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]\
                    ["gsmBitErrorRate"] = bitErrorRate;

                signalStrengthChangeEvent.mutable_gsm_signal_strength_info()->
                    set_gsm_signal_strength(signalStrength);
                signalStrengthChangeEvent.mutable_gsm_signal_strength_info()->
                    set_gsm_bit_error_rate(bitErrorRate);
            } else if(rat == "WCDMA") {
                token = EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int signalStrength = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int bitErrorRate = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int ecio = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index],
                    DEFAULT_DELIMITER);
                int rscp = std::stoi(token);
                LOG(DEBUG, __FUNCTION__," signalStrength:", signalStrength," bitErrorRate:",
                    bitErrorRate, " ecio:", ecio, " rscp:", rscp);

                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
                    ["signalStrength"] = signalStrength;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
                    ["bitErrorRate"] = bitErrorRate;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
                    ["ecio"] = ecio;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
                    ["rscp"] = rscp;
                signalStrengthChangeEvent.mutable_wcdma_signal_strength_info()->
                    set_signal_strength(signalStrength);
                signalStrengthChangeEvent.mutable_wcdma_signal_strength_info()->
                    set_bit_error_rate(bitErrorRate);
                signalStrengthChangeEvent.mutable_wcdma_signal_strength_info()->
                    set_ecio(ecio);
                signalStrengthChangeEvent.mutable_wcdma_signal_strength_info()->
                    set_rscp(rscp);
            } else if(rat == "LTE") {
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int signalStrength = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int rsrp = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int rsrq = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int rssnr = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int cqi = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int timingAdvance = std::stoi(token);
                LOG(DEBUG, __FUNCTION__," signalStrength:", signalStrength," rsrp:",
                    rsrp, " rsrq:", rsrq," rssnr:", rssnr, " cqi:", cqi," timingAdvance:",
                    timingAdvance);

                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteSignalStrength"] = signalStrength;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteRsrp"] = rsrp;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteRsrq"] = rsrq;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteRssnr"] = rssnr;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["lteCqi"] = cqi;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
                    ["timingAdvance"] = timingAdvance;

                signalStrengthChangeEvent.mutable_lte_signal_strength_info()->
                    set_lte_signal_strength(signalStrength);
                signalStrengthChangeEvent.mutable_lte_signal_strength_info()->
                    set_lte_rsrp(rsrp);
                signalStrengthChangeEvent.mutable_lte_signal_strength_info()->
                    set_lte_rsrq(rsrq);
                signalStrengthChangeEvent.mutable_lte_signal_strength_info()->
                    set_lte_rssnr(rssnr);
                signalStrengthChangeEvent.mutable_lte_signal_strength_info()->
                    set_lte_cqi(cqi);
                signalStrengthChangeEvent.mutable_lte_signal_strength_info()->
                    set_timing_advance(timingAdvance);
            } else if(rat == "NR5G") {
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int rsrp = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int rsrq = std::stoi(token);
                token = EventParserUtil::getNextToken(params[index], DEFAULT_DELIMITER);
                int rssnr = std::stoi(token);
                LOG(DEBUG, __FUNCTION__, " rsrp:", rsrp, " rsrq:", rsrq, " rssnr:", rssnr);

                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]\
                    ["rsrp"] = rsrp;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]\
                    ["rsrq"] = rsrq;
                rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]\
                    ["rssnr"] = rssnr;

                signalStrengthChangeEvent.mutable_nr5g_signal_strength_info()->set_rsrp(rsrp);
                signalStrengthChangeEvent.mutable_nr5g_signal_strength_info()->set_rsrq(rsrq);
                signalStrengthChangeEvent.mutable_nr5g_signal_strength_info()->set_rssnr(rssnr);
            } else {
                LOG(ERROR, " Invalid or deprecated RAT");
                    return;
            }
        }
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
        anyResponse.mutable_any()->PackFrom(signalStrengthChangeEvent);
    }  catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void PhoneManagerServerImpl::triggerChangeEvent(::eventService::EventResponse anyResponse) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void PhoneManagerServerImpl::handleCellInfoChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);

    // Split the event string into parameters( for phoneId, CellInfo1, CellInfo2 ...) based on
    // delimeter as ","
    std::stringstream ss(eventParams);
    std::vector<string> params;
    while (getline(ss, eventParams, ',')) {
        params.emplace_back(eventParams);
    }

    for(std::string str:params) {
        LOG(DEBUG, __FUNCTION__," Param: ", str);
    }

    ::telStub::CellInfoListEvent cellInfoListEvent;
    ::eventService::EventResponse anyResponse;
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(params[0], DEFAULT_DELIMITER);
        int phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " PhoneId : ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for phone id");
            return;
        }

        Json::Value rootObj;
        std::string jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return;
        }

        rootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"].clear();
        cellInfoListEvent.set_phone_id(phoneId);
        int jsonCellCount = rootObj[TEL_PHONE_MANAGER] ["cellInfo"]["cellList"].size();
        int newCellCount = params.size() - 1;
        LOG(DEBUG, " jsonCellCount ", jsonCellCount , " newCellCount", newCellCount);

        for (int i = 1; i <= newCellCount; i++) {
            LOG(DEBUG, " Parsing Params:" , params[i]);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int cell = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Cell Type is: ", cell);
            telux::tel::CellType cellType = static_cast<telux::tel::CellType>(cell);
            if (cell < (static_cast<int>(telux::tel::CellType::GSM)) ||
                cell > (static_cast<int>(telux::tel::CellType::NR5G))) {
                LOG(ERROR, __FUNCTION__, " Invalid input for cell type");
                return;
            }

            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int registered = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Is registered cell: ", registered);

            rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["cellType"] = cell;
            rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["registered"] = registered;

            telStub::CellInfoList *cellInfo = cellInfoListEvent.add_cell_info_list();
            cellInfo->mutable_cell_type()->set_cell_type(
                static_cast<telStub::CellInfo_CellType>(cell));
            cellInfo->mutable_cell_type()->set_registered(registered);

            switch(cellType) {
                case telux::tel::CellType::GSM:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int cid = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int arfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int bsic = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int signalStrength = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int bitErrorRate = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " lac:", lac, " cid:",
                        cid, " arfcn:", arfcn, " bsic:", bsic, " signalStrength:", signalStrength
                        , " bitErrorRate:", bitErrorRate);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["lac"] = lac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["cid"] = cid;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["arfcn"] = arfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmCellIdentity"]["bsic"] = bsic;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmSignalStrength"] = signalStrength;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["gsmCellInfo"]\
                        ["gsmSignalStrengthInfo"]["gsmBitErrorRate"] = bitErrorRate;

                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_mcc(mcc);
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_mnc(mnc);
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_lac(lac);
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_cid(cid);
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_arfcn(
                        arfcn);
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_cell_identity()->set_bsic(bsic);
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                        set_gsm_signal_strength(signalStrength);
                    cellInfo->mutable_gsm_cell_info()->mutable_gsm_signal_strength_info()->
                        set_gsm_bit_error_rate(bitErrorRate);
                    break;
                }
                case telux::tel::CellType::WCDMA:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int cid = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int psc = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int uarfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int signalStrength = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int bitErrorRate = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int ecio = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rscp = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " lac:", lac, " cid:",
                        cid, " psc:", psc, " uarfcn:", uarfcn, " signalStrength:", signalStrength
                        , " bitErrorRate:", bitErrorRate, " ecio:", ecio, " rscp:", rscp);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["lac"] = lac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["cid"] = cid;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["psc"] = psc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaCellIdentity"]["uarfcn"] = uarfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["signalStrength"] = signalStrength;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["bitErrorRate"] = bitErrorRate;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["ecio"] = ecio;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["wcdmaCellInfo"]\
                        ["wcdmaSignalStrengthInfo"]["rscp"] = rscp;

                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->
                        set_mcc(mcc);
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->
                        set_mnc(mnc);
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->
                        set_lac(lac);
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->
                        set_cid(cid);
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->
                        set_psc(psc);
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_cell_identity()->
                        set_uarfcn(uarfcn);
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_signal_strength(signalStrength);
                    cellInfo->mutable_wcdma_cell_info()->mutable_wcdma_signal_strength_info()->
                        set_bit_error_rate(bitErrorRate);
                    break;
                }
                case telux::tel::CellType::LTE:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int ci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int pci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int tac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int earfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int signalStrength = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lteRsrp = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lteRsrq = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lteRssnr = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int lteCqi = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int timingAdvance = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " ci:", ci, " pci:",
                        pci, " tac:", tac, " earfcn:", earfcn, " signalStrength:", signalStrength
                        , "lteRsrp:", lteRsrp, "lteRsrq:", lteRsrq, "lteRssnr:", lteRssnr
                        , "lteCqi:", lteCqi, "timingAdvance:", timingAdvance);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["ci"] = ci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["pci"] = pci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["tac"] = tac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteCellIdentity"]["earfcn"] = earfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteSignalStrength"] = signalStrength;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteRsrp"] = lteRsrp;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteRsrq"] = lteRsrq;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteRssnr"] = lteRssnr;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["lteCqi"] = lteCqi;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]["lteCellInfo"]\
                        ["lteSignalStrengthInfo"]["timingAdvance"] = timingAdvance;

                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_mcc(mcc);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_mnc(mnc);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_tac(tac);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_ci(ci);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->set_pci(pci);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_cell_identity()->
                        set_earfcn(earfcn);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_signal_strength(signalStrength);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rsrp(lteRsrp);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rsrq(lteRsrq);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_rssnr(lteRssnr);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_lte_cqi(lteCqi);
                    cellInfo->mutable_lte_cell_info()->mutable_lte_signal_strength_info()->
                        set_timing_advance(timingAdvance);
                    break;
                }
                case telux::tel::CellType::NR5G:
                {
                    std::string mcc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    std::string mnc = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int ci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int pci = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int tac = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int arfcn = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rsrp = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rsrq = std::stoi(token);
                    token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
                    int rssnr = std::stoi(token);

                    LOG(DEBUG, __FUNCTION__," mcc:", mcc," mnc:", mnc, " ci:", ci, " pci:",
                        pci, " tac:", tac, " arfcn:", arfcn, " rsrp:", rsrp, "rsrq:", rsrq, "rssnr:"
                        , rssnr);

                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["mcc"] = mcc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["mnc"] = mnc;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["ci"] = ci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["pci"] = pci;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["tac"] = tac;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gCellIdentity"]["arfcn"] = arfcn;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gSignalStrengthInfo"]["rsrp"] = rsrp;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gSignalStrengthInfo"]["rsrq"] = rsrq;
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["nr5gCellInfo"]["nr5gSignalStrengthInfo"]["rssnr"] = rssnr;

                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_mcc(mcc);
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_mnc(mnc);
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_tac(tac);
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_ci(ci);
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->set_pci(pci);
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_cell_identity()->
                        set_arfcn(arfcn);
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rsrp(rsrp);
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rsrq(rsrq);
                    cellInfo->mutable_nr5g_cell_info()->mutable_nr5g_signal_strength_info()->
                        set_rssnr(rssnr);
                    break;
                }
                case telux::tel::CellType::CDMA:
                case telux::tel::CellType::TDSCDMA:
                default:
                {
                    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][i-1]\
                        ["registered"] = 0; // none of the cell is registered
                    cellInfo->mutable_cell_type()->set_registered(0);
                    LOG(ERROR, " Invalid or deprecated cell type");
                    break;
                }
            }
        }

        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
        anyResponse.mutable_any()->PackFrom(cellInfoListEvent);

    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void PhoneManagerServerImpl::handleVoiceServiceStateChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }

        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int voiceServiceState = std::stoi(token);
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int voiceServiceDenialCause = std::stoi(token);
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int radioTech = std::stoi(token);
        notifyAndUpdateVoiceServiceState(phoneId, voiceServiceState, voiceServiceDenialCause,
            radioTech);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }
}

void PhoneManagerServerImpl::notifyAndUpdateVoiceServiceState(int phoneId, int voiceServiceState,
    int voiceServiceDenialCause, int radioTech) {

    Json::Value rootObj;
    std::string jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return;
    }

    rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
        ["voiceServiceState"] = voiceServiceState;
    rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]\
        ["voiceServiceDenialCause"] = voiceServiceDenialCause;
    rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["radioTech"] = radioTech;

    LOG(DEBUG, __FUNCTION__, " VoiceServiceState:", voiceServiceState,
        " VoiceServiceDenialCause:", voiceServiceDenialCause, " RadioTech:",
        radioTech );

    telStub::VoiceServiceStateEvent voiceServiceStateChangeEvent;
    ::eventService::EventResponse anyResponse;
    voiceServiceStateChangeEvent.set_phone_id(phoneId);
    voiceServiceStateChangeEvent.set_voice_service_state
        (static_cast<telStub::VoiceServiceState>(voiceServiceState));
    voiceServiceStateChangeEvent.set_voice_service_denial_cause
        (static_cast<telStub::VoiceServiceDenialCause>(voiceServiceDenialCause));
    voiceServiceStateChangeEvent.set_radio_technology
        (static_cast<telStub::RadioTechnology>(radioTech));

    JsonParser::writeToJsonFile(rootObj, jsonfilename);
    anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
    anyResponse.mutable_any()->PackFrom(voiceServiceStateChangeEvent);

    auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void PhoneManagerServerImpl::handleOperatingModeChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    try {
        int operatingMode = std::stoi(token);
        notifyAndUpdateOperatingMode(operatingMode);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }
}

void PhoneManagerServerImpl::notifyAndUpdateOperatingMode(int operatingMode) {

    Json::Value rootObj;
    std::string jsonfilename = JSON_PATH3;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return;
    }
    rootObj[TEL_PHONE_MANAGER]["operatingModeInfo"]["operatingMode"] = operatingMode;
    LOG(DEBUG, __FUNCTION__, " OperatingMode:", operatingMode);
    JsonParser::writeToJsonFile(rootObj, jsonfilename);
    /* TODO When operating mode changes to other than online, need to set default
        values for signal strength(UNAVAILABLE), cellinfo(cellidentity changes?),
        voice servicestate(voice service state to NOT_REG_AND_NOT_SEARCHING,
        radio tech is unknown), operator name(unknown).
        Handling of triggering internal notifications will be handled later . */
    if (operatingMode != 0) {
        updateJsonForDefaultValues(DEFAULT_SLOT_ID, true);
    }

    //Operating mode is per device not per slot
    telStub::OperatingModeEvent operatingModeChangeEvent;
    ::eventService::EventResponse anyResponse;
    operatingModeChangeEvent.set_operating_mode(static_cast<telStub::OperatingMode>(operatingMode));
    anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
    anyResponse.mutable_any()->PackFrom(operatingModeChangeEvent);

    auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void PhoneManagerServerImpl::handleECallOperatingModeChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }

        // Read string to get eCall mode
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int ecallMode = std::stoi(token);
        if (ecallMode < (static_cast<int>(telux::tel::ECallMode::NORMAL)) ||
            ecallMode > (static_cast<int>(telux::tel::ECallMode::NONE))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for eCall mode");
            return;
        }
        // Read string to get eCall mode reason
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int ecallModeReason = std::stoi(token);
        if (ecallModeReason < (static_cast<int>(telux::tel::ECallModeReason::NORMAL)) ||
            ecallModeReason > (static_cast<int>(telux::tel::ECallModeReason::ERA_GLONASS))) {
            LOG(ERROR, __FUNCTION__, " Invalid input for eCall mode reason");
            return;
        }
        notifyAndUpdateEcallMode(phoneId, ecallMode, ecallModeReason);
    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }
}

void PhoneManagerServerImpl::notifyAndUpdateEcallMode(int phoneId, int ecallMode,
    int ecallModeReason) {

    Json::Value rootObj;
    std::string jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return;
    }

    rootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]["ecallMode"] = ecallMode;
    rootObj[TEL_PHONE_MANAGER]["eCallOperatingMode"]["ecallModeReason"] = ecallModeReason;
    LOG(DEBUG, __FUNCTION__, " ecallMode: ", ecallMode, " ecallModeReason: ", ecallModeReason);

    ::telStub::ECallModeInfoChangeEvent ecallModeInfoChangeEvent;
    ::eventService::EventResponse anyResponse;

    ecallModeInfoChangeEvent.set_phone_id(phoneId);
    ecallModeInfoChangeEvent.set_ecall_mode(static_cast<telStub::ECallMode>(ecallMode));
    ecallModeInfoChangeEvent.set_ecall_mode_reason
        (static_cast<telStub::ECallModeReason::Reason>(ecallModeReason));

    JsonParser::writeToJsonFile(rootObj, jsonfilename);
    anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
    anyResponse.mutable_any()->PackFrom(ecallModeInfoChangeEvent);

    auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void PhoneManagerServerImpl::handleOperatorInfoChanged(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);
    try {
        // Read string to get slotId
        std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int phoneId = std::stoi(token);
        LOG(DEBUG, __FUNCTION__, " Slot id is: ", phoneId);
        if (phoneId < SLOT_1 || phoneId > SLOT_2) {
            LOG(ERROR, " Invalid input for slot id");
            return;
        }

        Json::Value rootObj;
        std::string jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
        telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
        if (error != ErrorCode::SUCCESS) {
            LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
            return;
        }

        ::telStub::PlmnInfo info;
        // Read string to get long name
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        string longName = token;
        info.set_long_name(token);

        // Read string to get short name
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        string shortName = token;
        info.set_short_name(token);

        // Read string to get plmn name
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        string plmn = token;
        info.set_plmn(token);

        // Read string to get home
        token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
        int home = std::stoi(token);
        info.set_ishome(false);
        if (home == 1 || home == 0) {
           if (home == 1) {
                info.set_ishome(true);
           }
        } else {
            LOG(ERROR, " Invalid input for home");
            return;
        }
        rootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["longName"] = longName;
        rootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["shortName"] = shortName;
        rootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["plmn"] = plmn;
        rootObj[TEL_PHONE_MANAGER]["operatorNameInfo"]["home"] = static_cast<bool>(home);
        LOG(DEBUG, __FUNCTION__, " longName:", longName, " shortName:", shortName,
            " plmn:", plmn, " ishome:", home);

        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        triggerOperatorModeChange(phoneId, info);

    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }
}

void PhoneManagerServerImpl::triggerOperatorModeChange(int phoneId, ::telStub::PlmnInfo info) {

    ::telStub::OperatorInfoEvent operatorInfoEvent;
    ::eventService::EventResponse anyResponse;
    operatorInfoEvent.set_phone_id(phoneId);
    operatorInfoEvent.mutable_plmn_info()->set_long_name(info.long_name());
    operatorInfoEvent.mutable_plmn_info()->set_short_name(info.short_name());
    operatorInfoEvent.mutable_plmn_info()->set_plmn(info.plmn());
    operatorInfoEvent.mutable_plmn_info()->set_ishome(info.ishome());
    anyResponse.set_filter(telux::tel::TEL_PHONE_FILTER);
    anyResponse.mutable_any()->PackFrom(operatorInfoEvent);
    auto f = std::async(std::launch::async, [this, anyResponse]() {
        this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void PhoneManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__," Event: ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__," Token: ", token );
    if (PHONE_EVENT_SIGNAL_STRENGTH_CHANGE == token) {
        handleSignalStrengthChanged(event);
    } else if (PHONE_EVENT_CELL_INFO_CHANGE == token) {
        handleCellInfoChanged(event);
    } else if (PHONE_EVENT_VOICE_SERVICE_STATE_CHANGE == token) {
        handleVoiceServiceStateChanged(event);
    } else if (PHONE_EVENT_OPERATING_MODE_CHANGE == token) {
       handleOperatingModeChanged(event);
    } else if (PHONE_EVENT_ECALL_OPERATING_MODE_CHANGE == token) {
       handleECallOperatingModeChanged(event);
    } else if (PHONE_EVENT_OPERATOR_INFO_CHANGE == token) {
       handleOperatorInfoChanged(event);
    } else {
        LOG(ERROR, __FUNCTION__, " Event not supported");
    }
}

void PhoneManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == telux::tel::TEL_PHONE_FILTER) {
        std::string event = message.event();
        onEventUpdate(event);
    }
}

telStub::VoiceServiceTechnology PhoneManagerServerImpl::convertVoiceTechStringToEnum(
    std::string voiceTech) {
    LOG(DEBUG, __FUNCTION__, " VoiceTech : ", voiceTech);
    if (voiceTech == "GW_CSFB") {
        return ::telStub::VoiceServiceTechnology::VOICE_TECH_GW_CSFB;
    } else if (voiceTech == "1x_CSFB") {
        return ::telStub::VoiceServiceTechnology::VOICE_TECH_1x_CSFB;
    } else if (voiceTech == "VOLTE") {
        return ::telStub::VoiceServiceTechnology::VOICE_TECH_VOLTE;
    } else {
        LOG(ERROR, " Invalid VoiceTech");
    }
    return ::telStub::VoiceServiceTechnology::VOICE_TECH_INVALID;
}

telStub::RATCapability PhoneManagerServerImpl::convertRATCapStringToEnum(std::string radioCap) {
    LOG(DEBUG, __FUNCTION__, " RadioCap : ", radioCap);
    if (radioCap == "AMPS") {
        return telStub::RATCapability::AMPS;
    } else if (radioCap == "CDMA") {
        return telStub::RATCapability::CDMA;
    } else if (radioCap == "HDR") {
        return telStub::RATCapability::HDR;
    } else if (radioCap == "GSM") {
        return telStub::RATCapability::GSM;
    } else if (radioCap == "WCDMA") {
        return telStub::RATCapability::WCDMA;
    } else if (radioCap == "LTE") {
        return telStub::RATCapability::LTE;
    } else if (radioCap == "NR5G") {
        return telStub::RATCapability::NR5G;
    } else if (radioCap == "NR5GSA") {
        return telStub::RATCapability::NR5GSA;
    } else {
        LOG(ERROR, " Invalid radio capability");
    }
    return telStub::RATCapability::RAT_CAP_INVALID;
}

void PhoneManagerServerImpl::updateJsonForDefaultValues(int phoneId, bool resetVoiceState) {
    LOG(DEBUG, __FUNCTION__, " phoneId : ", phoneId);
    Json::Value rootObj;
    std::string jsonfilename = (phoneId == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    telux::common::ErrorCode error = JsonParser::readFromJsonFile(rootObj, jsonfilename);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed" );
        return;
    }
    if (resetVoiceState) {
        rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["voiceServiceState"] = 0;
        rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["voiceServiceDenialCause"] = -1;
        rootObj[TEL_PHONE_MANAGER]["voiceServiceStateInfo"]["radioTech"] = 0;
    }
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]\
        ["gsmSignalStrength"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["gsmSignalStrengthInfo"]\
        ["gsmBitErrorRate"] = INVALID_SIGNAL_STRENGTH_VALUE;
    // update signal strength for cellInfo list
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["gsmCellInfo"]\
        ["gsmSignalStrengthInfo"]["gsmSignalStrength"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["gsmCellInfo"]\
        ["gsmSignalStrengthInfo"]["gsmBitErrorRate"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
        ["signalStrength"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
        ["bitErrorRate"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
        ["signalStrength"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
        ["bitErrorRate"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
        ["ecio"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["wcdmaSignalStrengthInfo"]\
        ["rscp"] = INVALID_SIGNAL_STRENGTH_VALUE;
    // update signal strength for cellInfo list
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["wcdmaCellInfo"]\
        ["wcdmaSignalStrengthInfo"]["signalStrength"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["wcdmaCellInfo"]\
        ["wcdmaSignalStrengthInfo"]["bitErrorRate"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
        ["lteSignalStrength"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
        ["lteRsrp"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
        ["lteRsrq"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
        ["lteRssnr"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
        ["lteCqi"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["lteSignalStrengthInfo"]\
        ["timingAdvance"] = INVALID_SIGNAL_STRENGTH_VALUE;
    // update signal strength for cellInfo list
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["lteCellInfo"]\
        ["lteSignalStrengthInfo"]["lteSignalStrength"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["lteCellInfo"]\
        ["lteSignalStrengthInfo"]["lteRsrp"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["lteCellInfo"]\
        ["lteSignalStrengthInfo"]["lteRsrq"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["lteCellInfo"]\
        ["lteSignalStrengthInfo"]["lteRssnr"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["lteCellInfo"]\
        ["lteSignalStrengthInfo"]["lteCqi"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["lteCellInfo"]\
        ["lteSignalStrengthInfo"]["timingAdvance"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrp"]
        = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]["rsrq"]
        = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["signalStrengthInfo"]["nr5gSignalStrengthInfo"]\
        ["rssnr"] = INVALID_SIGNAL_STRENGTH_VALUE;
    // update signal strength for cellInfo list
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["nr5gCellInfo"]\
        ["nr5gSignalStrengthInfo"]["rsrp"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["nr5gCellInfo"]\
        ["nr5gSignalStrengthInfo"]["rsrq"] = INVALID_SIGNAL_STRENGTH_VALUE;
    rootObj[TEL_PHONE_MANAGER]["cellInfo"]["cellList"][0]["nr5gCellInfo"]\
        ["nr5gSignalStrengthInfo"]["rssnr"] = INVALID_SIGNAL_STRENGTH_VALUE;
    // TODO Cell Identity need to reset?
    JsonParser::writeToJsonFile(rootObj, jsonfilename);
}
