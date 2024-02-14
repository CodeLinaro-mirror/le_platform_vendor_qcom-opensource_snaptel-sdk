/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <telux/common/DeviceConfig.hpp>

#include "DataSettingsServerImpl.hpp"
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/CommonUtils.hpp"

#define DATA_SETTINGS_API_LOCAL_JSON "api/data/IDataSettingsManagerLocal.json"
#define DATA_SETTINGS_STATE_JSON "system-state/data/IDataSettingsManagerState.json"

#define SLOT_2 2
#define REMOTE 1
#define PERM "PERMANENT"
#define TEMP "TEMPORARY"

DataSettingsServerImpl::DataSettingsServerImpl(
    std::shared_ptr<DataConnectionServerImpl> dcmServerImpl):
    dcmServerImpl_(dcmServerImpl) {
    LOG(DEBUG, __FUNCTION__);
    updateDdsInfo();
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

DataSettingsServerImpl::~DataSettingsServerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

grpc::Status DataSettingsServerImpl::InitService(ServerContext* context,
    const dataStub::InitRequest* request, dataStub::GetServiceStatusReply* response) {

    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = DATA_SETTINGS_API_LOCAL_JSON;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj["IDataSettingsManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["IDataSettingsManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    response->set_service_status(static_cast<dataStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

void DataSettingsServerImpl::updateDdsInfo() {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestDdsSwitch";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return;
    }

    ddsInfo_.type = static_cast<telux::data::DdsType>(
        (data.stateRootObj[subsystem][method]["DdsType"].asString()
        == PERM) ? 0 : 1);
    ddsInfo_.slotId = static_cast<SlotId>(
        data.stateRootObj[subsystem][method]["SlotId"].asInt());
}

grpc::Status DataSettingsServerImpl::SetDdsSwitch(ServerContext* context,
    const dataStub::SetDdsSwitchRequest* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestDdsSwitch";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    } else if (!telux::common::DeviceConfig::isMultiSimSupported()) {
        data.error = telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
    } else if ((ddsInfo_.type == static_cast<telux::data::DdsType>(request->switch_type())) &&
        (ddsInfo_.slotId = static_cast<SlotId>(request->slot_id()))) {
        data.error = telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        ddsInfo_.type = static_cast<telux::data::DdsType>(request->switch_type());
        ddsInfo_.slotId = static_cast<SlotId>(request->slot_id());

        // we are only updating json if it is PERM switch, since TEMP
        //switch is not persistent sccross reboots.
        if (request->switch_type() == 0) {
            data.stateRootObj[subsystem][method]["DdsType"]
                = (request->switch_type() == 0) ? PERM : TEMP;
            data.stateRootObj[subsystem][method]["SlotId"]
                = request->slot_id();
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::RequestCurrentDdsSwitch(ServerContext* context,
    const dataStub::CurrentDdsSwitchRequest* request,
    dataStub::CurrentDdsSwitchResponse* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestDdsSwitch";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        response->set_current_switch(static_cast<int>(ddsInfo_.type));
        response->set_slot_id(ddsInfo_.slotId);
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::setBandInterferenceConfig(ServerContext* context,
    const dataStub::BandInterferenceConfig* request, dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestBandInterferenceConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {

        data.stateRootObj[subsystem][method]["enable"] = request->enable();
        if (request->enable()) {
            data.stateRootObj[subsystem][method]["priority"]
                = (request->priority() == 0) ? "N79" : "WLAN";
            data.stateRootObj[subsystem][method]["wlanWaitTimeInSec"]
                = request->wlan_wait_time_in_sec();
            data.stateRootObj[subsystem][method]["n79WaitTimeInSec"]
                = request->n79_wait_time_in_sec();
            JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
        }
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::requestBandInterferenceConfig(ServerContext* context,
    const dataStub::BandInterferenceRequest* request,
    dataStub::BandInterferenceReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestBandInterferenceConfig";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int isenabled =
            data.stateRootObj[subsystem][method]["enable"].asBool();
        response->mutable_config()->set_enable(isenabled);
        if (isenabled) {
            response->mutable_config()->set_priority(
                data.stateRootObj[subsystem][method]["priority"].asString() == "N79" ? 0 : 1);
            response->mutable_config()->set_wlan_wait_time_in_sec(
                data.stateRootObj[subsystem][method]["wlanWaitTimeInSec"].asInt());
            response->mutable_config()->set_n79_wait_time_in_sec(
                data.stateRootObj[subsystem][method]["n79WaitTimeInSec"].asInt());
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::SetWwanConnectivityConfig(ServerContext* context,
    const dataStub::SetWwanConnectivityConfigRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestWwanConnectivityConfig";
    std::string stateMethod = "requestWwanConnectivityConfig";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int slotId = request->slot_id();
        int slotIdx = (request->slot_id() == SLOT_2) ? 1 : 0;
        bool isAllowed = request->is_wwan_connectivity_allowed();
        data.stateRootObj[subsystem][stateMethod]["isAllowed"][slotIdx] = isAllowed;

        auto f = std::async(std::launch::async,
            [this, slotId, isAllowed, data]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(data.cbDelay + 100));
                //stopping active datacalls on the requested slot_id
                if ((!isAllowed) && (this->dcmServerImpl_)) {
                    this->dcmServerImpl_->stopActiveDataCalls(
                        static_cast<SlotId>(slotId));
                }
            }).share();
        taskQ_->add(f);

        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::RequestWwanConnectivityConfig(ServerContext* context,
    const dataStub::WwanConnectivityConfigRequest* request,
    dataStub::WwanConnectivityConfigReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestWwanConnectivityConfig";
    std::string stateMethod = "requestWwanConnectivityConfig";

    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int slotId = (request->slot_id() == SLOT_2) ? 1 : 0;
        response->set_is_wwan_connectivity_allowed(
            data.stateRootObj[subsystem][stateMethod]["isAllowed"][slotId].asBool());
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::SetMacSecState(ServerContext* context,
    const dataStub::SetMacSecStateRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestMacSecState";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj[subsystem][method]["enabled"] = request->enabled();
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::RequestMacSecState(ServerContext* context,
    const dataStub::MacSecStateRequest* request,
    dataStub::MacSecStateReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestMacSecState";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        response->set_enabled(
            data.stateRootObj[subsystem][method]["enabled"].asBool());
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::setBackhaulPreference(ServerContext* context,
    const dataStub::setBackhaulPreferenceRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestBackhaulPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        Json::Value newPref;
        for (auto pref : request->backhaul_pref()) {
            newPref.append(Json::Value(convertEnumToBackhaulPrefString(
                static_cast<::dataStub::BackhaulPreference>(pref))));
        }
        data.stateRootObj[subsystem][method]["backhaulPref"] = newPref;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::requestBackhaulPreference(ServerContext* context,
    const dataStub::RequestBackhaulPreference* request,
    dataStub::BackhaulPreferenceReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "requestBackhaulPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        int size = data.stateRootObj[subsystem][method]["backhaulPref"].size();
        for (auto idx = 0; idx < size; idx++) {
            response->add_backhaul_pref(convertBackhaulPrefStringToEnum(
                data.stateRootObj[subsystem][method]["backhaulPref"][idx].asString()));
        }
    }

    response->mutable_reply()->set_status(static_cast<commonStub::Status>(data.status));
    response->mutable_reply()->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->mutable_reply()->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

grpc::Status DataSettingsServerImpl::switchBackHaul(ServerContext* context,
    const dataStub::switchBackHaulRequest* request,
    dataStub::DefaultReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = DATA_SETTINGS_API_LOCAL_JSON;
    std::string stateJsonPath = DATA_SETTINGS_STATE_JSON;
    std::string subsystem = "IDataSettingsManager";
    std::string method = "switchBackHaul";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }

    if (request->operation_type() == REMOTE) {
        data.error = telux::common::ErrorCode::INVALID_OPERATION;
    }

    if (data.status == telux::common::Status::SUCCESS &&
        data.error == telux::common::ErrorCode::SUCCESS) {
        data.stateRootObj[subsystem][method]["backhaul"] =
            convertEnumToBackhaulPrefString(
            static_cast<::dataStub::BackhaulPreference>(request->backhaul_type()));
        data.stateRootObj[subsystem][method]["slotId"] = request->slot_id();
        data.stateRootObj[subsystem][method]["profileId"] = request->profile_id();
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }

    response->set_status(static_cast<commonStub::Status>(data.status));
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);

    return grpc::Status::OK;
}

dataStub::BackhaulPreference DataSettingsServerImpl::convertBackhaulPrefStringToEnum(
    std::string pref) {

    if (pref == "ETH") {
        return ::dataStub::BackhaulPreference::PREF_ETH;
    } else if (pref == "USB") {
        return ::dataStub::BackhaulPreference::PREF_USB;
    } else if (pref == "WLAN") {
        return ::dataStub::BackhaulPreference::PREF_WLAN;
    } else if (pref == "WWAN") {
        return ::dataStub::BackhaulPreference::PREF_WWAN;
    } else if (pref == "BLE") {
        return ::dataStub::BackhaulPreference::PREF_BLE;
    }

    return ::dataStub::BackhaulPreference::INVALID;
}

std::string DataSettingsServerImpl::convertEnumToBackhaulPrefString(
    ::dataStub::BackhaulPreference pref) {

    switch (pref) {
        case ::dataStub::BackhaulPreference::PREF_ETH:
            return "ETH";
        case ::dataStub::BackhaulPreference::PREF_USB:
            return "USB";
        case ::dataStub::BackhaulPreference::PREF_WLAN:
            return "WLAN";
        case ::dataStub::BackhaulPreference::PREF_WWAN:
            return "WWAN";
        case ::dataStub::BackhaulPreference::PREF_BLE:
            return "BLE";
        case ::dataStub::BackhaulPreference::INVALID:
        default:
            return "INVALID";
    }

    return "INVALID";
}