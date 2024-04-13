/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "ServingManagerServerImpl.hpp"

#include "libs/tel/TelDefinesStub.hpp"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"

#include <telux/common/CommonDefines.hpp>

#define JSON_PATH1 "api/tel/IServingSystemManagerSlot1.json"
#define JSON_PATH2 "api/tel/IServingSystemManagerSlot2.json"
#define JSON_PATH3 "system-state/tel/IServingSystemManagerStateSlot1.json"
#define JSON_PATH4 "system-state/tel/IServingSystemManagerStateSlot2.json"
#define MANAGER "IServingSystemManager"
#define SLOT_1 1
#define SLOT_2 2
#define CALL_BARRING_UPDATE_EVENT "callBarringUpdate"

ServingManagerServerImpl::ServingManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

ServingManagerServerImpl::~ServingManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    if (taskQ_) {
        taskQ_ = nullptr;
    }
}

grpc::Status ServingManagerServerImpl::CleanUpService(ServerContext* context,
    const ::google::protobuf::Empty* request, ::google::protobuf::Empty* response) {
    LOG(DEBUG, __FUNCTION__);
    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::InitService(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }

    int cbDelay = rootObj[MANAGER]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);
    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::vector<std::string> filters = {telux::tel::TEL_SERVING_SYSTEM_FILTER};
        auto &serverEventManager = ServerEventManager::getInstance();
        serverEventManager.registerListener(shared_from_this(), filters);
    }
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetServiceStatus(ServerContext* context,
    const ::commonStub::GetServiceStatusRequest* request,
    commonStub::GetServiceStatusReply* response) {
    Json::Value rootObj;
    std::string filePath = (request->phone_id() == SLOT_1)? JSON_PATH1
        : JSON_PATH2;
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObj, filePath);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::NOT_FOUND, "Json not found");
    }
    std::string srvStatus = rootObj[MANAGER]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<commonStub::ServiceStatus>(status));
    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestRATPreference(ServerContext* context,
    const ::telStub::RequestRATPreferenceRequest* request,
    telStub::RequestRATPreferenceReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestRatPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::vector<int> raTdata;
        std::string value =  data.stateRootObj[MANAGER]["RATPreference"].asString();
        LOG(DEBUG, __FUNCTION__,"String is ", value);
        raTdata = CommonUtils::convertStringToVector(value);
        for(auto &it : raTdata) {
            response->add_rat_pref_types(static_cast<telStub::RatPrefType>(it));
        }
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::SetRATPreference(ServerContext* context,
    const ::telStub::SetRATPreferenceRequest* request,
    telStub::SetRATPreferenceReply* response) {

    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setRatPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        std::vector<uint8_t> ratPrefs;
        for (auto rat : request->rat_pref_types()) {
            ratPrefs.emplace_back(static_cast<uint8_t>(rat));
        }
        std::string value =  CommonUtils::convertVectorToString(ratPrefs, false);
        data.stateRootObj[MANAGER]["RATPreference"] = value;
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestServiceDomainPreference(ServerContext* context,
    const ::telStub::RequestServiceDomainPreferenceRequest* request,
    telStub::RequestServiceDomainPreferenceReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestServiceDomainPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int serviceDomainPreference = data.stateRootObj[MANAGER]["ServiceDomainPreference"].asInt();
        response->set_service_domain_pref
        (static_cast<telStub::ServiceDomainPreference_Pref>(serviceDomainPreference));
    }

    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::SetServiceDomainPreference(ServerContext* context,
    const ::telStub::SetServiceDomainPreferenceRequest* request,
    telStub::SetServiceDomainPreferenceReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "setServiceDomainPreference";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        ::telStub::ServiceDomainPreference_Pref pref = request->service_domain_pref();
        data.stateRootObj[MANAGER]["ServiceDomainPreference"] = static_cast<int>(pref);
        JsonParser::writeToJsonFile(data.stateRootObj, stateJsonPath);
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetDcStatus(ServerContext* context,
    const ::telStub::GetDcStatusRequest* request,
    telStub::GetDcStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getDcStatus";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        telux::tel::DcStatus status;
        status.endcAvailability =
        static_cast<telux::tel::EndcAvailability>(data.stateRootObj[MANAGER]["DcStatus"]\
            ["endcAvailability"].asInt());
        status.dcnrRestriction =
        static_cast<telux::tel::DcnrRestriction>(data.stateRootObj[MANAGER]["DcStatus"]\
            ["dcnrRestriction"].asInt());
        response->set_endc_availability(
        static_cast<telStub::EndcAvailability_Status>(status.endcAvailability));
        response->set_dcnr_restriction
        (static_cast<telStub::DcnrRestriction_Status>(status.dcnrRestriction));
    }
    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetSystemInfo(ServerContext* context,
    const ::telStub::GetSystemInfoRequest* request,
    telStub::GetSystemInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getSystemInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int serviceDomainPreference =  data.stateRootObj[MANAGER]["ServingSystemInfo"]\
            ["domain"].asInt();
        response->set_current_domain
        (static_cast<telStub::ServiceDomainInfo_Domain>(serviceDomainPreference));
        int rat =  data.stateRootObj[MANAGER]["ServingSystemInfo"]["rat"].asInt();
        response->set_current_rat(static_cast<telStub::RadioTechnology>(rat));
    }
    //Create response
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestNetworkTime(ServerContext* context,
    const ::telStub::RequestNetworkTimeRequest* request,
    telStub::RequestNetworkTimeReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestNetworkTime";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        telux::tel::NetworkTimeInfo result;
        result.year = data.stateRootObj[MANAGER]["NetworkTimeInfo"]["year"].asInt();
        result.month = data.stateRootObj[MANAGER]["NetworkTimeInfo"]["month"].asInt();
        result.day =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["day"].asInt();
        result.hour = data.stateRootObj[MANAGER]["NetworkTimeInfo"]["hour"].asInt();
        result.minute = data.stateRootObj[MANAGER]["NetworkTimeInfo"]["minute"].asInt();
        result.second =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["second"].asInt();
        result.dayOfWeek =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["dayOfWeek"].asInt();
        result.timeZone =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["timeZone"].asInt();
        result.dstAdj =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["dstAdj"].asInt();
        result.nitzTime =  data.stateRootObj[MANAGER]["NetworkTimeInfo"]["nitzTime"].asString();
        telStub::NetworkTimeInfo info;
        info.set_year(result.year);
        info.set_month(result.month);
        info.set_day(result.day);
        info.set_minute(result.minute);
        info.set_second(result.second);
        info.set_day_of_week(result.dayOfWeek);
        info.set_time_zone(result.timeZone);
        info.set_dst_adj(result.dstAdj);
        info.set_nitz_time(result.nitzTime);
        *response->mutable_network_time_info() = info;
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::RequestRFBandInfo(ServerContext* context,
    const ::telStub::RequestRFBandInfoRequest* request,
    telStub::RequestRFBandInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "requestRFBandInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        response->set_band(static_cast<telStub::RFBand>(data.stateRootObj[MANAGER]["RFBandInfo"]\
            ["rFBand"].asInt()));
        response->set_band_width(static_cast<telStub::RFBandWidth>(data.stateRootObj[MANAGER]\
            ["RFBandInfo"]["bandwidth"].asInt()));
        response->set_channel(data.stateRootObj[MANAGER]["RFBandInfo"]["channel"].asInt());
    }
    //Create response
    if(data.cbDelay != -1) {
        response->set_is_callback(true);
    } else {
        response->set_is_callback(false);
    }
    response->set_error(static_cast<commonStub::ErrorCode>(data.error));
    response->set_delay(data.cbDelay);
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetNetworkRejectInfo(ServerContext* context,
    const ::telStub::GetNetworkRejectInfoRequest* request,
    telStub::GetNetworkRejectInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getNetworkRejectInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int serviceDomain =  data.stateRootObj[MANAGER]["NetworkRejectInfo"]\
            ["ServingSystemInfo"]["domain"].asInt();
        response->set_reject_domain
        (static_cast<telStub::ServiceDomainInfo_Domain>(serviceDomain));
        int rat =  data.stateRootObj[MANAGER]["NetworkRejectInfo"]["ServingSystemInfo"]\
            ["rat"].asInt();
        response->set_reject_rat(static_cast<telStub::RadioTechnology>(rat));
        int rejectCause = data.stateRootObj[MANAGER]["NetworkRejectInfo"]\
            ["rejectCause"].asInt();
        response->set_reject_cause(rejectCause);
        std::string mcc = data.stateRootObj[MANAGER]["NetworkRejectInfo"]\
            ["mcc"].asString();
        response->set_mcc(mcc);
        std::string mnc = data.stateRootObj[MANAGER]["NetworkRejectInfo"]\
            ["mnc"].asString();
        response->set_mnc(mnc);
    }
    //Create response
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

grpc::Status ServingManagerServerImpl::GetCallBarringInfo(ServerContext* context,
    const ::telStub::GetCallBarringInfoRequest* request,
    telStub::GetCallBarringInfoReply* response) {
    LOG(DEBUG, __FUNCTION__);
    std::string apiJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH1 : JSON_PATH2;
    std::string stateJsonPath = (request->phone_id() == SLOT_1)? JSON_PATH3 : JSON_PATH4;
    std::string subsystem = MANAGER;
    std::string method = "getCallBarringInfo";
    JsonData data;
    telux::common::ErrorCode error =
        CommonUtils::readJsonData(apiJsonPath, stateJsonPath, subsystem, method, data);

    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! " );
        return grpc::Status(grpc::StatusCode::INTERNAL, "Json read failed");
    }
    if(data.status == telux::common::Status::SUCCESS) {
        int count = data.stateRootObj[MANAGER] ["CallBarringInfo"]["infoList"].size();
        for (int i = 0 ; i < count; i++) {
            telStub::CallBarringInfo *result = response->add_barring_infos();
            Json::Value requestedInfo =
                data.stateRootObj[MANAGER] ["CallBarringInfo"]["infoList"][i];
            int rat = requestedInfo["rat"].asInt();
            result->set_rat(static_cast<telStub::RadioTechnology>(rat));
            int serviceDomain = requestedInfo["domain"].asInt();
            result->set_domain
                (static_cast<telStub::ServiceDomainInfo_Domain>(serviceDomain));
            int callType = requestedInfo["callType"].asInt();
            result->set_call_type
                (static_cast<telStub::CallsAllowedInCell_Type>(callType));
        }
    }
    //Create response
    response->set_status(static_cast<commonStub::Status>(data.status));

    return grpc::Status::OK;
}

void ServingManagerServerImpl::handleCallBarringUpdate(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);

    // Split the event string into parameters( for phoneId ,BarringInfo1 ,BarringInfo2 ...)
    // based on delimeter as ","
    std::stringstream ss(eventParams);
    std::vector<string> params;
    while (getline(ss, eventParams, ',')) {
        params.emplace_back(eventParams);
    }
    for(std::string str:params) {
        LOG(DEBUG, __FUNCTION__," Param: ", str);
    }

    ::telStub::CallBarringInfosEvent callBarringInfosEvent;
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

        rootObj[MANAGER] ["CallBarringInfo"]["infoList"].clear();
        callBarringInfosEvent.set_phone_id(phoneId);
        int jsonInfoCount = rootObj[MANAGER] ["CallBarringInfo"]["infoList"].size();
        int newInfoCount = params.size() - 1;
        LOG(DEBUG, " jsonInfoCount ", jsonInfoCount , " newInfoCount ", newInfoCount);

        for (int i = 1; i <= newInfoCount; i++) {
            LOG(DEBUG, " Parsing Params:" , params[i]);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int rat = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Rat is: ", rat);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int domain = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " Domain is: ", domain);
            token = EventParserUtil::getNextToken(params[i], DEFAULT_DELIMITER);
            int callType = std::stoi(token);
            LOG(DEBUG, __FUNCTION__, " CallType is: ", callType);
            rootObj[MANAGER] ["CallBarringInfo"]["infoList"][i-1]["rat"] = rat;
            rootObj[MANAGER] ["CallBarringInfo"]["infoList"][i-1]["domain"] = domain;
            rootObj[MANAGER] ["CallBarringInfo"]["infoList"][i-1]["callType"] = callType;

            telStub::CallBarringInfo *result = callBarringInfosEvent.add_barring_infos();
            result->set_rat(static_cast<telStub::RadioTechnology>(rat));
            result->set_domain
                (static_cast<telStub::ServiceDomainInfo_Domain>(domain));
            result->set_call_type
                (static_cast<telStub::CallsAllowedInCell_Type>(callType));
        }

        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        anyResponse.set_filter(telux::tel::TEL_SERVING_SYSTEM_FILTER);
        anyResponse.mutable_any()->PackFrom(callBarringInfosEvent);

    } catch(exception const & ex) {
        LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
        return;
    }

    auto f = std::async(std::launch::async, [this, anyResponse]() {
            this->triggerChangeEvent(anyResponse);
    }).share();
    taskQ_->add(f);
}

void ServingManagerServerImpl::triggerChangeEvent(::eventService::EventResponse anyResponse) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //posting the event to EventService event queue
    auto& eventImpl = EventService::getInstance();
    eventImpl.updateEventQueue(anyResponse);
}

void ServingManagerServerImpl::onEventUpdate(::eventService::UnsolicitedEvent message) {
    if (message.filter() == telux::tel::TEL_SERVING_SYSTEM_FILTER) {
        onEventUpdate(message.event());
    }
}

void ServingManagerServerImpl::onEventUpdate(std::string event) {
    LOG(DEBUG, __FUNCTION__," Event string is ", event );
    std::string token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__," Token String is ", token );
    if (CALL_BARRING_UPDATE_EVENT == token) {
        handleCallBarringUpdate(event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}
