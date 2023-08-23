/*
* Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted (subject to the limitations in the
* disclaimer below) provided that the following conditions are met:
*
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*
*     * Redistributions in binary form must reproduce the above
*       copyright notice, this list of conditions and the following
*       disclaimer in the documentation and/or other materials provided
*       with the distribution.
*
*     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
*       contributors may be used to endorse or promote products derived
*       from this software without specific prior written permission.
*
* NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
* GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
* HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
* WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
* IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
* ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
* GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
* IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
* OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
* IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "CardManagerServerImpl.hpp"
#include "../../../libs/tel/TelDefinesStub.hpp"

#define JSON_PATH1 "system-state/tel/ICardManagerStateSlot1.json"
#define JSON_PATH2 "system-state/tel/ICardManagerStateSlot2.json"
#define JSON_PATH3 "api/tel/ICardManagerSlot1.json"
#define JSON_PATH4 "api/tel/ICardManagerSlot2.json"

#define SLOT_1 1
#define SLOT_2 2

CardManagerServerImpl::CardManagerServerImpl() {
    LOG(DEBUG, __FUNCTION__);
    readJson();
    jsonObjSystemStateSlot_[SLOT_1] = rootObjSystemStateSlot1_;
    jsonObjSystemStateSlot_[SLOT_2] = rootObjSystemStateSlot2_;
    jsonObjSystemStateFileName_[SLOT_1] = JSON_PATH1;
    jsonObjSystemStateFileName_[SLOT_2] = JSON_PATH2;
    //Api response
    jsonObjApiResponseSlot_[SLOT_1] = rootObjApiResponseSlot1_;
    jsonObjApiResponseSlot_[SLOT_2] = rootObjApiResponseSlot2_;
    jsonObjApiResponseFileName_[SLOT_1] = JSON_PATH3;
    jsonObjApiResponseFileName_[SLOT_2] = JSON_PATH4;

}

void CardManagerServerImpl::readJson() {
    LOG(DEBUG, __FUNCTION__);
    telux::common::ErrorCode error =
        JsonParser::readFromJsonFile(rootObjSystemStateSlot1_, JSON_PATH1);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH1 );
        exit(0);
    }
    error = JsonParser::readFromJsonFile(rootObjSystemStateSlot2_, JSON_PATH2);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH2 );
        exit(0);
    }
    error = JsonParser::readFromJsonFile(rootObjApiResponseSlot1_, JSON_PATH3);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH3 );
        exit(0);
    }
    error = JsonParser::readFromJsonFile(rootObjApiResponseSlot2_, JSON_PATH4);
    if (error != ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed! ", JSON_PATH4 );
        exit(0);
    }
}

void CardManagerServerImpl::getJsonForSystemData(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjSystemStateFileName_[phoneId];
    rootObj = jsonObjSystemStateSlot_[phoneId];
}

void CardManagerServerImpl::getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
    Json::Value& rootObj ) {
    jsonfilename = jsonObjApiResponseFileName_[phoneId];
    rootObj = jsonObjApiResponseSlot_[phoneId];
}

bool CardManagerServerImpl::isCallbackNeeded(Json::Value rootObj, std::string apiname) {
    int value = rootObj["ICardManager"][apiname]["callbackDelay"].asInt();
    bool isCallback = true;
    if (value == -1) {
        isCallback = false;
    }
    return isCallback;
}

bool CardManagerServerImpl::findAppId(Json::Value rootObj, const char* appid, int& index) {
    int sizeofADF = rootObj["ICardManager"]["EFs"]["ADF"].size();
    LOG(DEBUG, __FUNCTION__,"Size of ADF is", sizeofADF);
    bool foundAppId = false;
    for (index =0 ; index < sizeofADF ; index++) {
        const char* tmp = (rootObj["ICardManager"]["EFs"]["ADF"]\
            [index]["AppId"].asString()).c_str();
        std::string val = rootObj["ICardManager"]["EFs"]["ADF"]\
            [index]["AppId"].asString();
        LOG(DEBUG, __FUNCTION__,"appid is orignal from json  ", val);
        if(strcmp(tmp, appid) == 0) {
            foundAppId = true;
            std::string t = rootObj["ICardManager"]["EFs"]["ADF"][index]["AppId"].asString();
            LOG(DEBUG, __FUNCTION__,"appid is ", t);
            break;
        }
    }
    return foundAppId;
}

grpc::Status CardManagerServerImpl::InitService(ServerContext* context,
    const google::protobuf::Empty* request,
    tel::GetServiceStatusReply* response) {

    Json::Value rootObj;
    rootObj = jsonObjApiResponseSlot_[SLOT_1];
    int cbDelay = rootObj["ICardManager"]["IsSubsystemReadyDelay"].asInt();
    std::string cbStatus =
        rootObj["ICardManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(cbStatus);
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", cbStatus);

    if(status == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        auto &eventManager = telux::common::EventManager::getInstance();
        eventManager.registerListener(shared_from_this(), "tel_card");
    }
    response->set_service_status(static_cast<tel::ServiceState>(status));
    response->set_delay(cbDelay);
    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::GetServiceStatus(ServerContext* context,
    const google::protobuf::Empty* request,
    tel::GetServiceStatusReply* response) {

    Json::Value rootObj;
    rootObj = jsonObjApiResponseSlot_[SLOT_1];
    std::string srvStatus = rootObj["ICardManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus status = CommonUtils::mapServiceStatus(srvStatus);
    response->set_service_status(static_cast<tel::ServiceState>(status));
    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::IsSubsystemReady(ServerContext* context,
    const google::protobuf::Empty* request,
    tel::IsSubsystemReadyReply* response) {

    bool status = false;
    Json::Value rootObj;
    rootObj = jsonObjApiResponseSlot_[SLOT_1];
    std::string IsSubsystemReady = rootObj["ICardManager"]["IsSubsystemReady"].asString();
    telux::common::ServiceStatus servstatus = CommonUtils::mapServiceStatus(IsSubsystemReady);
    if(servstatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        status = true;
    }
    response->set_is_ready(status);
    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::GetCardState(ServerContext* context,
    const ::tel::GetCardStateRequest* request,
    tel::GetCardStateReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    telux::tel::CardState cardState;
    std::string jsonfilename = "";
    Json::Value rootObj;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    int state = rootObj["ICardManager"]["GetState"]["cardState"].asInt();
    cardState = static_cast<telux::tel::CardState>(state);
    // Create response
    switch(cardState) {
    case telux::tel::CardState::CARDSTATE_UNKNOWN:
        response->set_card_state(tel::CardState::CARDSTATE_UNKNOWN);
        break;
    case telux::tel::CardState::CARDSTATE_ABSENT:
        response->set_card_state(tel::CardState::CARDSTATE_ABSENT);
        break;
    case telux::tel::CardState::CARDSTATE_PRESENT:
        response->set_card_state(tel::CardState::CARDSTATE_PRESENT);
        break;
    case telux::tel::CardState::CARDSTATE_ERROR:
        response->set_card_state(tel::CardState::CARDSTATE_ERROR);
        break;
    default:
        response->set_card_state(tel::CardState::CARDSTATE_ERROR);
        break;
    }
    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::ReadEFLinearFixed(ServerContext* context,
    const ::tel::ReadEFLinearFixedRequest* request,
    tel::ReadEFLinearFixedReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    getJsonForSystemData(slotId, jsonfilename, rootObj);
    //Api response
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);

    std::string filepath = request->file_path();
    uint16_t fileId  = request->file_id();
    int recordNum = request->record_number();
    std::string aid = request->aid();
    const char* appid = aid.c_str();
    telux::tel::IccResult result;
    telux::common::ErrorCode error;
    telux::common::Status status;
    tel::ErrorCode tmp;
    uint32_t cbDelay;
     CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "ReadEFLinearFixed", status,
        error, cbDelay );
    int index;
    int i =0;
    if(status == telux::common::Status::SUCCESS) {
        bool foundAppId = findAppId(rootObj, appid, index);
        if(foundAppId) {
            int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"].size();
            LOG(DEBUG, __FUNCTION__,"LinearFixedEFfiles size ", size);
            tmp = findmatchingrecordADF<tel::ReadEFLinearFixedReply*>(rootObj, response, size,
                index, recordNum, fileId, i);
            if(tmp == tel::ErrorCode::ERROR_CODE_SUCCESS) {
                result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                            [i+recordNum]["sw1"].asInt();
                result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                            [i+recordNum]["sw2"].asInt();
                result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+recordNum]["payload"].asString();
                std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+recordNum]["data"].asString();
                result.data = Helper::convertStringToVector(input);
            } else {
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        } else {
            int size = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"].size();
            tmp = findmatchingrecordDF(rootObj, size, recordNum , fileId, i);
            if (tmp == tel::ErrorCode::ERROR_CODE_SUCCESS) {
                result.sw1 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                            [i+recordNum]["sw1"].asInt();
                result.sw2 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                            [i+recordNum]["sw2"].asInt();
                result.payload = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                            [i+recordNum]["payload"].asString();
                std::string input = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                            [i+recordNum]["data"].asString();
                result.data = Helper::convertStringToVector(input);
            } else {
                LOG(DEBUG, __FUNCTION__, "Valid AppId not found");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
    }

    // Create response
    tel::IccResult requestedRecord;
    std::string apiname = "ReadEFLinearFixed";
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_delay(cbDelay);
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_status(static_cast<tel::Status>(status));
    if(error == telux::common::ErrorCode::SUCCESS) {
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
    } else {
        std::string s = "";
        requestedRecord.set_sw1(0);
        requestedRecord.set_sw2(0);
        requestedRecord.set_pay_load(s);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
    }
    *response->mutable_result() = requestedRecord;
    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::WriteEFLinearFixed(ServerContext* context,
    const ::tel::WriteEFLinearFixedRequest* request,
    tel::WriteEFLinearFixedReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int slotId = request->slot_id();
    std::string jsonfilename = "";
    std::string jsonObjApiResponseFileName = "";
    Json::Value rootObj;
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(slotId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::string filepath = request->file_path();
    uint16_t fileId  = request->file_id();
    std::string aid = request->aid();
    const char* appid = aid.c_str();
    telux::tel::IccResult result;
    int recordsize = request->record_number();
    std::vector<uint8_t> data;
    telux::common::Status status;
    uint32_t cbDelay;
    tel::ErrorCode tmp;
    telux::common::ErrorCode error;
    for(int d : request->data()) {
        data.emplace_back(d);
    }
    int s = data.size();
    for (int i = 0; i < s; i++) {
        LOG(DEBUG, __FUNCTION__,"data recieved from request", static_cast<int>(data.at(i)));
    }
    std::string str1 = "";
    int i = 0;
    int index =0;
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "WriteEFLinearFixed", status,
        error, cbDelay );
    if(status == telux::common::Status::SUCCESS) {
        bool foundAppId = findAppId(rootObj, appid, index);
        if(foundAppId) {
            int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"].size();
            LOG(DEBUG, __FUNCTION__,"LinearFixedEFfiles size ", size);
            tmp = findmatchingrecordADF<tel::WriteEFLinearFixedReply*>(rootObj,
                response, size, index, recordsize, fileId, i);
            if(tmp == tel::ErrorCode::ERROR_CODE_SUCCESS) {
                str1 = Helper::convertVectorToString(data, false);
                LOG(DEBUG, __FUNCTION__,"String value is", str1);
                rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i+recordsize]\
                    ["data"] = str1;
                LOG(DEBUG, __FUNCTION__,"String is data  ", str1);
                str1 = Helper::convertVectorToString(data, true);
                LOG(DEBUG, __FUNCTION__,"String is payload ", str1);
                rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i+recordsize]\
                    ["payload"] = str1;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[slotId] = rootObj;
                result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                    [i+recordsize]["sw1"].asInt();
                LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                    [i+recordsize]["sw2"].asInt();
                LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                    ["LinearFixedEFFiles"][i+recordsize]["payload"].asString();
                std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                    ["LinearFixedEFFiles"][i+recordsize]["data"].asString();
                result.data = Helper::convertStringToVector(input);
            } else {
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        } else {
            int i = 0;
            int size = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"].size();
            tmp = findmatchingrecordDF(rootObj, size, recordsize , fileId, i);
            if (tmp == tel::ErrorCode::ERROR_CODE_SUCCESS) {
                str1 = Helper::convertVectorToString(data, false);
                rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i+recordsize]\
                    ["data"] = str1;
                LOG(DEBUG, __FUNCTION__,"String is  ", str1);
                str1 = Helper::convertVectorToString(data, true);
                rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i+recordsize]\
                    ["payload"] = str1;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[slotId] = rootObj;
                result.sw1 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                    [i+recordsize]["sw1"].asInt();
                result.sw2 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                    [i+recordsize]["sw2"].asInt();
                result.payload = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                    [i+recordsize]["payload"].asString();
                std::string input = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                    [i+recordsize]["data"].asString();
                result.data = Helper::convertStringToVector(input);
            } else {
                LOG(DEBUG, __FUNCTION__, "Valid AppId not found");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
    }
    //Create response
    std::string apiname = "WriteEFLinearFixed";
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_error(static_cast<tel::ErrorCode>(error));
    LOG(DEBUG, __FUNCTION__, "STatus is", static_cast<int>(status));
    response->set_status(static_cast<tel::Status>(status));

    tel::IccResult requestedRecord;

    if(error == telux::common::ErrorCode::SUCCESS) {
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
    } else {
        std::string s = "";
        requestedRecord.set_sw1(0);
        requestedRecord.set_sw2(0);
        requestedRecord.set_pay_load(s);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
    }
    *response->mutable_result() = requestedRecord;
    return grpc::Status::OK;
    }

    grpc::Status CardManagerServerImpl::ReadEFLinearFixedAll(ServerContext* context,
    const ::tel::ReadEFLinearFixedAllRequest* request,
    tel::ReadEFLinearFixedAllReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    std::string apiname = "ReadEFLinearFixedAll";
    Json::Value rootObj;
    Json::Value jsonObjApiResponse;
    std::string jsonObjApiResponseFileName = "";
    getJsonForSystemData(slotId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::string filepath = request->file_path();
    uint16_t fileId  = request->file_id();
    std::string aid = request->aid();
    const char* appid = aid.c_str();
    std::vector<telux::tel::IccResult> records;
    tel::ErrorCode tmp;
    uint32_t cbDelay;
    telux::common::Status status;
    telux::common::ErrorCode error;
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );

    int index;
    if(status == telux::common::Status::SUCCESS) {
        bool foundAppId = findAppId(rootObj, appid, index);
        if(foundAppId) {
            int i = 0;
            int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"].size();
            LOG(DEBUG, __FUNCTION__,"LinearFixedEFfiles size ", size);
            while (i < size ) {
                uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                    ["LinearFixedEFFiles"][i]["fileId"].asInt();
                if (tmpfileId == fileId) {
                    int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                        [i]["numberOfRecords"].asInt();
                    LOG(DEBUG, __FUNCTION__,"NumberOfRecords ", num);
                    for(int j = 1; j <= num ; j++) {
                        telux::tel::IccResult result;
                        result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+j]["sw1"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                        result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+j]["sw2"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                        result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+j]["payload"].asString();
                        LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                        std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["LinearFixedEFFiles"][i+j]["data"].asString();
                        result.data = Helper::convertStringToVector(input);
                        records.emplace_back(result);
                    }
                    break;
                } else {
                    LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
                    int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                        [i]["numberOfRecords"].asInt();
                    i = i + num + 1;
                    LOG(DEBUG, __FUNCTION__,"Incremented value is ", i );
                }
            }
            if(i == size) {
                LOG(DEBUG, __FUNCTION__,"Valid record not found ", i );
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        } else {
            int size = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"].size();
            int i =0;
            int recordnum = 0;
            tmp = findmatchingrecordDF(rootObj, size, recordnum , fileId, i);
            if (tmp == tel::ErrorCode::ERROR_CODE_SUCCESS) {
                int num = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i]\
                    ["numberOfRecords"].asInt();
                LOG(DEBUG, __FUNCTION__,"NumberOfRecords ", num);
                for(int j = 1; j <= num ; j++) {
                    telux::tel::IccResult result;
                    result.sw1 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                        [j]["sw1"].asInt();
                    result.sw2 = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][j]\
                        ["sw2"].asInt();
                    result.payload = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                        [j]["payload"].asString();
                    std::string input = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
                        [j]["data"].asString();
                    result.data = Helper::convertStringToVector(input);
                    records.emplace_back(result);
                }
            } else {
                LOG(DEBUG, __FUNCTION__,"Valid fileId not found ");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
    }

    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_iscallback(iscallback);
    response->set_delay(cbDelay);
    response->set_status(static_cast<tel::Status>(status));

    for(auto &it : records) {
        tel::IccResult *result = response->add_records();
        if(error == telux::common::ErrorCode::SUCCESS) {
            result->set_sw1(it.sw1);
            result->set_sw2(it.sw2);
            result->set_pay_load(it.payload);
            for (auto &r : it.data) {
                result->add_data(r);
            }
        } else {
            std::string s = "";
            result->set_sw1(0);
            result->set_sw2(0);
            result->set_pay_load(s);
            for (auto &r : it.data) {
                result->add_data(r);
            }
        }
    }
    return grpc::Status::OK;
}
grpc::Status CardManagerServerImpl::ReadEFTransparent(ServerContext* context,
    const ::tel::ReadEFTransparentRequest* request,
    tel::ReadEFTransparentReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    Json::Value rootObj;
    getJsonForSystemData(slotId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::string filepath = request->file_path();
    uint16_t fileId  = request->file_id();
    int recordsize = request->size();
    std::string aid = request->aid();
    const char* appid = aid.c_str();
    telux::tel::IccResult result;
    telux::common::ErrorCode error;
    telux::common::Status status;
    uint32_t cbDelay;
    std::string apiname = "ReadEFTransparent";
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );

    int sizeofADF = rootObj["ICardManager"]["EFs"]["ADF"].size();
    LOG(DEBUG, __FUNCTION__,"Size of ADF is", sizeofADF);
    int index;
    if(status == telux::common::Status::SUCCESS) {
        bool foundAppId = findAppId(rootObj, appid, index);
        if(foundAppId) {
            int i = 0;
            int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"].size();
            LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size);
            while (i < size ) {
                uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                    ["TransparentEFFiles"][i]["fileId"].asInt();
                if (tmpfileId == fileId) {
                    if (recordsize >= 0)  {
                        result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["TransparentEFFiles"][i]["sw1"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                        result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["TransparentEFFiles"][i]["sw2"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                        result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["TransparentEFFiles"][i]["payload"].asString();
                        LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                        std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                            ["TransparentEFFiles"][i]["data"].asString();
                        result.data = Helper::convertStringToVector(input);
                        break;
                    } else {
                        LOG(DEBUG, __FUNCTION__,"Request failed ");
                        error = telux::common::ErrorCode::GENERIC_FAILURE;
                    }
                }
                i++;
            }
            if (i == size) {
                LOG(DEBUG, __FUNCTION__,"FileId not found ");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        } else {
            int i = 0;
            int size = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"].size();
            LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size);
            while (i < size ) {
                uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                    [i]["fileId"].asInt();
                if (tmpfileId == fileId) {
                    if (recordsize >= 0)  {
                        result.sw1 = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                            [i]["sw1"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                        result.sw2 = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                            [i]["sw2"].asInt();
                        LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                        result.payload = rootObj["ICardManager"]["EFs"]\
                            ["DFTransparentEFRecords"][i]["payload"].asString();
                        LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                        std::string input = rootObj["ICardManager"]["EFs"]\
                            ["DFTransparentEFRecords"][i]["data"].asString();
                        result.data = Helper::convertStringToVector(input);
                        break;
                    } else {
                        LOG(DEBUG, __FUNCTION__,"Request failed ");
                        error = telux::common::ErrorCode::GENERIC_FAILURE;
                    }
                }
                i++;
            }
            if (i == size) {
                LOG(DEBUG, __FUNCTION__,"FileId not found ");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
    }
    //Create response
    tel::IccResult requestedRecord;
    response->set_error(static_cast<tel::ErrorCode>(error));
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_delay(cbDelay);
    response->set_status(static_cast<tel::Status>(status));

    if(error == telux::common::ErrorCode::SUCCESS) {
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
    } else {
        std::string s = "";
        requestedRecord.set_sw1(0);
        requestedRecord.set_sw2(0);
        requestedRecord.set_pay_load(s);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
    }
    *response->mutable_result() = requestedRecord;
    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::WriteEFTransparent(ServerContext* context,
    const ::tel::WriteEFTransparentRequest* request,
    tel::WriteEFTransparentReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    std::string apiname = "WriteEFTransparent";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(slotId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::string filepath = request->file_path();
    uint16_t fileId  = request->file_id();
    std::string aid = request->aid();
    const char* appid = aid.c_str();
    telux::tel::IccResult result;
    std::vector<uint8_t> data;
    for(int d : request->data()) {
        data.emplace_back(d);
    }
    std::string str1 = "";
    std::string str2 = "";
    uint32_t cbDelay;
    telux::common::Status status;
    telux::common::ErrorCode error;
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );
    int i = 0;

    int sizeofADF = rootObj["ICardManager"]["EFs"]["ADF"].size();
    LOG(DEBUG, __FUNCTION__,"Size of ADF is", sizeofADF);
    int index;
    if(status == telux::common::Status::SUCCESS) {
        bool foundAppId = findAppId(rootObj, appid, index);
        if(foundAppId) {
            int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"].size();
            LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size );
            LOG(DEBUG, __FUNCTION__,"TransparentEFfiles index ", index );
            while (i < size ) {
                uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                    ["TransparentEFFiles"][i]["fileId"].asInt();
                LOG(DEBUG, __FUNCTION__,"FileId is  ", tmpfileId);
                if (tmpfileId == fileId) {
                    str1 = Helper::convertVectorToString(data, false);
                    rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"][i]\
                        ["data"] = str1;
                    LOG(DEBUG, __FUNCTION__,"String is  ", str1);
                    str1 = Helper::convertVectorToString(data, true);
                    rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"][i]\
                        ["payload"] = str1;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[slotId] = rootObj;
                    result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["TransparentEFFiles"][i]["sw1"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                    result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["TransparentEFFiles"][i]["sw2"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                    rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"]\
                        [i]["payload"] =
                    result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["TransparentEFFiles"][i]["payload"].asString();
                    LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                    std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["TransparentEFFiles"][i]["data"].asString();
                        result.data = Helper::convertStringToVector(input);
                    break;
                }
                i++;
            }
            if (i == size) {
                LOG(DEBUG, __FUNCTION__,"FileId not found ");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        } else {
            int i = 0;
            int size = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"].size();
            LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size);
            while (i < size ) {
                uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                    [i]["fileId"].asInt();
                if (tmpfileId == fileId) {
                    str1 = Helper::convertVectorToString(data, false);
                    rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"][i]["data"] = str1;
                    LOG(DEBUG, __FUNCTION__,"String is  ", str1);
                    str1 = Helper::convertVectorToString(data, true);
                    rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"][i]["payload"] = str1;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[slotId] = rootObj;
                    result.sw1 = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"][i]\
                        ["sw1"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                    result.sw2 = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"][i]\
                        ["sw2"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                    result.payload = rootObj["ICardManager"]["EFs"]["DFTransparentEFRecords"]\
                        [i]["payload"].asString();
                    LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                    std::string input = rootObj["ICardManager"]["EFs"]\
                        ["DFTransparentEFRecords"][i]["data"].asString();
                    result.data = Helper::convertStringToVector(input);
                    break;
                } else {
                    LOG(DEBUG, __FUNCTION__,"Request failed ");
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
                i++;
            }
            if (i == size) {
                LOG(DEBUG, __FUNCTION__,"FileId not found ");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        }
    }
    //Create response
    response->set_error(static_cast<tel::ErrorCode>(error));
    tel::IccResult requestedRecord;
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_delay(cbDelay);
    response->set_status(static_cast<tel::Status>(status));

    if(error == telux::common::ErrorCode::SUCCESS) {
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
    } else {
        std::string s = "";
        requestedRecord.set_sw1(0);
        requestedRecord.set_sw2(0);
        requestedRecord.set_pay_load(s);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
    }
    *response->mutable_result() = requestedRecord;
    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::RequestEFAttributes(ServerContext* context,
    const ::tel::EFAttributesRequest* request,
    tel::RequestEFAttributesReply* response) {

    int slotId = request->slot_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(slotId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(slotId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::string filepath = request->file_path();
    ::tel::EfType eftype = request->ef_type();
    telux::tel::EfType ef_type= static_cast<telux::tel::EfType>(eftype);
    uint16_t fileId  = request->file_id();
    std::string aid = request->aid();
    const char* appid = aid.c_str();

    telux::tel::IccResult result;
    telux::tel::FileAttributes attributes;
    tel::ErrorCode tmp;
    telux::common::ErrorCode error;
    std::string apiname = "RequestEFAttributes";
    uint32_t cbDelay;
    int index;
    int i = 0;
    telux::common::Status status;
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );

    int sizeofADF = rootObj["ICardManager"]["EFs"]["ADF"].size();
    LOG(DEBUG, __FUNCTION__,"Size of ADF is", sizeofADF);

    if(status == telux::common::Status::SUCCESS) {
        bool foundAppId = findAppId(rootObj, appid, index);
        if(foundAppId) {
            if (ef_type == telux::tel::EfType::TRANSPARENT ) {
                tmp = getTransparentFileAttributes(rootObj, i, fileId, attributes, index);
                if(tmp == tel::ErrorCode::ERROR_CODE_SUCCESS) {
                    std::string data = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["TransparentEFFiles"][i]["data"].asString();
                    std::vector<int> tmp = Helper::convertStringToVector(data);
                    result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["LinearFixedEFFiles"][i]["sw1"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                    result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["LinearFixedEFFiles"][i]["sw2"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                    result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["LinearFixedEFFiles"][i]["payload"].asString();
                    LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                } else {
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else if (ef_type == telux::tel::EfType::LINEAR_FIXED) {
                tmp = getLinearfixedFileAttributes(rootObj, i, fileId, attributes, index);
                if(tmp == tel::ErrorCode::ERROR_CODE_SUCCESS) {
                    result.sw1 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["LinearFixedEFFiles"][i+1]["sw1"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
                    result.sw2 = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["LinearFixedEFFiles"][i+1]["sw2"].asInt();
                    LOG(DEBUG, __FUNCTION__,"sw2 ", result.sw2);
                    result.payload = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["LinearFixedEFFiles"][i+1]["payload"].asString();
                    LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
                    std::string input = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                        ["LinearFixedEFFiles"][i+1]["data"].asString();
                    result.data = Helper::convertStringToVector(input);
                } else {
                    error = telux::common::ErrorCode::GENERIC_FAILURE;
                }
            } else {
                LOG(DEBUG, __FUNCTION__,"Unknown EFType ");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        } else {
            LOG(DEBUG, __FUNCTION__, "Valid AppId not found");
            error = telux::common::ErrorCode::GENERIC_FAILURE;
        }
    }

    //Create response
    tel::IccResult requestedRecord;

    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    response->set_delay(cbDelay);
    if(error == telux::common::ErrorCode::SUCCESS) {
        requestedRecord.set_sw1(result.sw1);
        requestedRecord.set_sw2(result.sw2);
        requestedRecord.set_pay_load(result.payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
        *response->mutable_result() = requestedRecord;
        tel::FileAttributes requestedAttributes;
        requestedAttributes.set_file_size(attributes.fileSize);
        requestedAttributes.set_record_size(attributes.recordSize);
        requestedAttributes.set_record_count(attributes.recordCount);
        *response->mutable_file_attributes() = requestedAttributes;
    } else {
        std::string payload = "";
        requestedRecord.set_sw1(0);
        requestedRecord.set_sw2(0);
        requestedRecord.set_pay_load(payload);
        for(auto &it : result.data) {
            requestedRecord.add_data(it);
        }
        *response->mutable_result() = requestedRecord;
        tel::FileAttributes requestedAttributes;
        requestedAttributes.set_file_size(0);
        requestedAttributes.set_record_size(0);
        requestedAttributes.set_record_count(0);
        *response->mutable_file_attributes() = requestedAttributes;
    }
    return grpc::Status::OK;
}

tel::ErrorCode CardManagerServerImpl::getTransparentFileAttributes(Json::Value rootObj,
    int& i, uint16_t fileId, telux::tel::FileAttributes& attributes, int& index) {
    int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["TransparentEFFiles"].size();
    LOG(DEBUG, __FUNCTION__,"TransparentEFfiles size ", size);
    tel::ErrorCode error = tel::ErrorCode::ERROR_CODE_SUCCESS;
    while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
            ["TransparentEFFiles"][i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            std::string data = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                ["TransparentEFFiles"][i]["data"].asString();
            std::vector<int> tmp = Helper::convertStringToVector(data);
            attributes.recordSize = tmp.size();
            attributes.fileSize = attributes.recordSize;
            break;
        } else {
            LOG(DEBUG, __FUNCTION__,"FileId not found ");
            i++;
        }
    }
    if(i == size) {
        LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
        error = tel::ErrorCode::GENERIC_FAILURE;
    }
    return error;
}

tel::ErrorCode CardManagerServerImpl::getLinearfixedFileAttributes(Json::Value rootObj,
    int& i, uint16_t fileId, telux::tel::FileAttributes& attributes, int& index ) {
    int size = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"].size();
    LOG(DEBUG, __FUNCTION__,"LinearFixedEFfiles size ", size);
    tel::ErrorCode error = tel::ErrorCode::ERROR_CODE_SUCCESS;
    while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]\
            ["LinearFixedEFFiles"][i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            attributes.recordCount = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                ["LinearFixedEFFiles"][i]["numberOfRecords"].asInt(); //
            std::string data = rootObj["ICardManager"]["EFs"]["ADF"][index]\
                ["LinearFixedEFFiles"][i+1]["data"].asString();
            std::vector<int> tmp = Helper::convertStringToVector(data);
            attributes.recordSize = tmp.size();
            attributes.fileSize = (attributes.recordCount)*(attributes.recordSize);
            error = tel::ErrorCode::ERROR_CODE_SUCCESS;
            break;
        } else {
            LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
            int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
                [i]["numberOfRecords"].asInt();
            i = i + num + 1;
            LOG(DEBUG, __FUNCTION__,"Incremented value is ", i );
        }
    }
    if(i == size) {
        LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
        error = tel::ErrorCode::GENERIC_FAILURE;
    }
    return error;
}

grpc::Status CardManagerServerImpl::OpenLogicalChannel(ServerContext* context,
    const ::tel::OpenLogicalChannelRequest* request,
    tel::OpenLogicalChannelReply* response) {

    LOG(DEBUG, __FUNCTION__);
    int phoneid = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneid, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneid, jsonObjApiResponseFileName, jsonObjApiResponse);
    telux::common::ErrorCode error;
    telux::common::Status status;
    uint32_t cbDelay;
    telux::tel::IccResult result;
    int channelId;

    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "OpenLogicalChannel", status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        bool isChannelOpen = rootObj["ICardManager"]["OpenLogicalChannel"]["isOpen"].asBool();
        if (isChannelOpen) {
            LOG(DEBUG, __FUNCTION__, "already open");
            error = telux::common::ErrorCode::GENERIC_FAILURE;
        } else {
            rootObj["ICardManager"]["OpenLogicalChannel"]["isOpen"] = true;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneid] = rootObj;
            result.sw1 = rootObj["ICardManager"]["TransmitApduLogicalChannel"]\
                ["onChannelResponseSw1"].asInt();
            LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw1);
            result.sw2 = rootObj["ICardManager"]["TransmitApduLogicalChannel"]\
                ["onChannelResponseSw2"].asInt();
            LOG(DEBUG, __FUNCTION__,"sw1 ", result.sw2);
            result.payload = rootObj["ICardManager"]["TransmitApduLogicalChannel"]\
                ["onChannelResponsePayload"].asString();
            LOG(DEBUG, __FUNCTION__,"payload ", result.payload);
            std::string tmp = rootObj["ICardManager"]["TransmitApduLogicalChannel"]\
                ["onChannelResponseData"].asString();
            std::vector<int> data = Helper::convertStringToVector(tmp);
            result.data = data;
            channelId = rootObj["ICardManager"]["OpenLogicalChannel"]\
                ["onChannelResponseChannel"].asInt();
            LOG(DEBUG, __FUNCTION__,"channelId ", channelId);
        }
    }
    //Create response
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    response->set_delay(cbDelay);
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, "OpenLogicalChannel");
    response->set_iscallback(iscallback);
    tel::IccResult requestedRecord;
    requestedRecord.set_sw1(result.sw1);
    requestedRecord.set_sw2(result.sw2);
    requestedRecord.set_pay_load(result.payload);
    for(auto &it : result.data) {
        requestedRecord.add_data(it);
    }
    *response->mutable_result() = requestedRecord;
    response->set_channel_id(channelId);

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::CloseLogicalChannel(ServerContext* context,
    const ::tel::CloseLogicalChannelRequest* request,
    tel::CloseLogicalChannelReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    int channel = request->channel_id();
    telux::common::Status status;
    telux::common::ErrorCode error;
    uint32_t cbDelay;
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "CloseLogicalChannel", status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        int inputchannel = rootObj["ICardManager"]["OpenLogicalChannel"]\
            ["onChannelResponseChannel"].asInt();
        if (inputchannel == channel) {
            bool isChannelOpen = rootObj["ICardManager"]["OpenLogicalChannel"]["isOpen"].asBool();
            if (isChannelOpen) {
                rootObj["ICardManager"]["OpenLogicalChannel"]["isOpen"] = false;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
            } else {
                LOG(DEBUG, __FUNCTION__, "already closed");
                error = telux::common::ErrorCode::GENERIC_FAILURE;
            }
        } else {
            LOG(DEBUG, __FUNCTION__, "Invalid channel");
            error = telux::common::ErrorCode::GENERIC_FAILURE;
        }
    }

    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    response->set_delay(cbDelay);
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, "CloseLogicalChannel");
    response->set_iscallback(iscallback);

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::TransmitAPDU(ServerContext* context,
    const ::tel::TransmitAPDURequest* request,
    tel::TransmitAPDUReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::vector<uint8_t> data;
    telux::common::Status status;
    telux::common::ErrorCode error;
    uint32_t cbDelay;
    for(int d : request->data()) {
        data.emplace_back(d);
    }
    std::string str1 = "";
    telux::tel::IccResult result;
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", "TransmitApduLogicalChannel", status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        str1 = Helper::convertVectorToString(data, false);
        rootObj["ICardManager"]["TransmitApduLogicalChannel"]["onChannelResponseData"] = str1;
        LOG(DEBUG, __FUNCTION__,"String is  ", str1);
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        jsonObjSystemStateSlot_[phoneId] = rootObj;


        str1 = Helper::convertVectorToString(data, true);
        rootObj["ICardManager"]["TransmitApduLogicalChannel"]["onChannelResponsePayload"] = str1;
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        jsonObjSystemStateSlot_[phoneId] = rootObj;

        result.sw1 = rootObj["ICardManager"]["TransmitApduLogicalChannel"]\
            ["onChannelResponseSw1"].asInt();
        result.sw2 = rootObj["ICardManager"]["TransmitApduLogicalChannel"]\
            ["onChannelResponseSw2"].asInt();
        result.payload = rootObj["ICardManager"]["TransmitApduLogicalChannel"]\
            ["onChannelResponsePayload"].asString();
        std::string tmp = rootObj["ICardManager"]["TransmitApduLogicalChannel"]\
            ["onChannelResponseData"].asString();
        result.data = Helper::convertStringToVector(tmp);
    }

    //Create response

    tel::IccResult requestedRecord;
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    response->set_delay(cbDelay);
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, "TransmitApduLogicalChannel");
    response->set_iscallback(iscallback);
    requestedRecord.set_sw1(result.sw1);
    requestedRecord.set_sw2(result.sw2);
    requestedRecord.set_pay_load(result.payload);
    for(auto &it : result.data) {
        requestedRecord.add_data(it);
    }
    *response->mutable_result() = requestedRecord;

    return grpc::Status::OK;

}

grpc::Status CardManagerServerImpl::exchangeSimIO(ServerContext* context,
    const ::tel::exchangeSimIORequest* request,
    tel::exchangeSimIOReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::vector<uint8_t> data;
    for(int d : request->data()) {
        data.emplace_back(d);
    }
    std::string str1 = "";
    telux::common::Status status;
    telux::common::ErrorCode error;
    uint32_t cbDelay;
    std::string apiname = "ExchangeSimIO";
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );
    telux::tel::IccResult result;

    if(status == telux::common::Status::SUCCESS) {
        str1 = Helper::convertVectorToString(data, false);
        rootObj["ICardManager"]["ExchangeSimIO"]["onChannelResponseData"] = str1;
        LOG(DEBUG, __FUNCTION__,"String is  ", str1);
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        jsonObjSystemStateSlot_[phoneId] = rootObj;

        str1 = Helper::convertVectorToString(data, true);
        rootObj["ICardManager"]["ExchangeSimIO"]["onChannelResponsePayload"] = str1;
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        jsonObjSystemStateSlot_[phoneId] = rootObj;

        result.sw1 = rootObj["ICardManager"]["ExchangeSimIO"]\
            ["onChannelResponseSw1"].asInt();
        result.sw2 = rootObj["ICardManager"]["ExchangeSimIO"]\
            ["onChannelResponseSw2"].asInt();
        result.payload = rootObj["ICardManager"]["ExchangeSimIO"]\
            ["onChannelResponsePayload"].asString();
        std::string tmp = rootObj["ICardManager"]["ExchangeSimIO"]\
            ["onChannelResponseData"].asString();
        result.data = Helper::convertStringToVector(tmp);
    }

    //Create response
    tel::IccResult requestedRecord;
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    response->set_delay(cbDelay);
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    requestedRecord.set_sw1(result.sw1);
    requestedRecord.set_sw2(result.sw2);
    requestedRecord.set_pay_load(result.payload);
    for(auto &it : result.data) {
        requestedRecord.add_data(it);
    }
    *response->mutable_result() = requestedRecord;

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::TransmitBasicAPDU(ServerContext* context,
    const ::tel::TransmitBasicAPDURequest* request,
    tel::TransmitBasicAPDUReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::vector<uint8_t> data;
    for(int d : request->data()) {
        data.emplace_back(d);
    }
    std::string str1 = "";
    telux::common::Status status;
    telux::common::ErrorCode error;
    uint32_t cbDelay;
    telux::tel::IccResult result;
    std::string apiname = "TransmitApduBasicChannel";
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        str1 = Helper::convertVectorToString(data, false);
        rootObj["ICardManager"]["TransmitApduBasicChannel"]["onChannelResponseData"] = str1;
        LOG(DEBUG, __FUNCTION__,"String is  ", str1);
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        jsonObjSystemStateSlot_[phoneId] = rootObj;

        str1 = Helper::convertVectorToString(data, true);
        rootObj["ICardManager"]["TransmitApduBasicChannel"]["onChannelResponsePayload"] = str1;
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        jsonObjSystemStateSlot_[phoneId] = rootObj;

        result.sw1 = rootObj["ICardManager"]["TransmitApduBasicChannel"]\
            ["onChannelResponseSw1"].asInt();
        result.sw2 = rootObj["ICardManager"]["TransmitApduBasicChannel"]\
            ["onChannelResponseSw2"].asInt();
        result.payload = rootObj["ICardManager"]["TransmitApduBasicChannel"]\
            ["onChannelResponsePayload"].asString();
        std::string tmp = rootObj["ICardManager"]["TransmitApduBasicChannel"]\
            ["onChannelResponseData"].asString();
        result.data = Helper::convertStringToVector(tmp);
    }

    //Create response
    tel::IccResult requestedRecord;
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    response->set_delay(cbDelay);
    response->set_iscallback(iscallback);
    requestedRecord.set_sw1(result.sw1);
    requestedRecord.set_sw2(result.sw2);
    requestedRecord.set_pay_load(result.payload);
    for(auto &it : result.data) {
        requestedRecord.add_data(it);
    }
    *response->mutable_result() = requestedRecord;

    return grpc::Status::OK;

}

grpc::Status CardManagerServerImpl::requestEid(ServerContext* context,
    const ::tel::requestEidRequest* request,
    tel::requestEidReply* response) {

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    telux::common::Status status;
    telux::common::ErrorCode errorCodefromUser;
    uint32_t cbDelay;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    std::string eid = rootObj["ICardManager"]["RequestEid"]["eid"].asString();

    //Create response
    std::string apiname = "TransmitApduBasicChannel";
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        errorCodefromUser, cbDelay );
    tel::ErrorCode error = static_cast<tel::ErrorCode>(errorCodefromUser);
    tel::Status status_value = static_cast<tel::Status>(status);
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_eid(eid);
    response->set_delay(cbDelay);
    response->set_iscallback(iscallback);
    response->set_error(error);
    response->set_status(status_value);

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::updateSimStatus(ServerContext* context,
    const ::tel::updateSimStatusRequest* request,
    tel::updateSimStatusReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    int state = rootObj["ICardManager"]["GetState"]["cardState"].asInt();
    response->set_card_state(static_cast<tel::CardState>(state));

    int size = rootObj["ICardManager"]["GetApplications"].size();
    for (int i = 0 ; i < size ; i++) {
        tel::CardApp *apps = response->add_card_apps();
        tel::AppType apptype = static_cast<tel::AppType>(rootObj["ICardManager"]\
            ["GetApplications"][i]["appType"].asInt());
        LOG(DEBUG, __FUNCTION__,"apptype is  ", static_cast<int>(apptype));
        apps->set_app_type(apptype);
        tel::AppState appstate = static_cast<tel::AppState>(rootObj["ICardManager"]\
            ["GetApplications"][i]["appState"].asInt());
        LOG(DEBUG, __FUNCTION__,"appstate is  ", static_cast<int>(appstate));
        apps->set_app_state(appstate);
        std::string appid = rootObj["ICardManager"]["GetApplications"][i]["appId"].asString();
        LOG(DEBUG, __FUNCTION__,"appid is  ", appid);
        apps->set_app_id(appid);
    }
    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::ChangePinLock(ServerContext* context,
    const ::tel::ChangePinLockRequest* request, tel::ChangePinLockReply* response) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    ::tel::CardLockType locktype = request->lock_type();
    string oldPwd = request->old_pin();
    string newPwd = request->new_pin();
    string appId = request->aid();
    std::string password;
    int retrycount;
    bool IsCardInfoChanged = false;
    telux::common::Status status;
    telux::common::ErrorCode error;
    uint32_t cbDelay;
    std::string apiname = "ChangeCardPassword";
    CommonUtils::getValues(jsonObjApiResponse, "ICardManager", apiname, status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        if(locktype == ::tel::CardLockType::PIN1) {
            password = rootObj["ICardManager"]["Pin1password"].asString();
            retrycount = rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin1"].asInt();
            if((oldPwd == password) && (retrycount != -1)) {
                rootObj["ICardManager"]["Pin1password"] = newPwd;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
            } else {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin1"].asInt();
                LOG(DEBUG, __FUNCTION__, "retrycount is ", retrycount);
                if (retrycount < 0) {
                    LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                    error = telux::common::ErrorCode::PIN_BLOCKED;
                    //Update the app state to puk for app
                    int size = rootObj["ICardManager"]["GetApplications"].size();
                    for (int i = 0; i < size; i++) {
                        std::string id = rootObj["ICardManager"]["GetApplications"]\
                            [i]["appId"].asString();
                        if (id == appId) {
                            rootObj["ICardManager"]["GetApplications"]\
                                [i]["appState"] = 3; //puk state
                            JsonParser::writeToJsonFile(rootObj, jsonfilename);
                            jsonObjSystemStateSlot_[phoneId] = rootObj;
                            IsCardInfoChanged = true;
                            break;
                        } else {
                            LOG(DEBUG, __FUNCTION__,"No matching appId found");
                            error = telux::common::ErrorCode::INVALID_ARG;
                        }
                    }
                } else {
                    if(retrycount >= -1) {
                        retrycount--;
                        LOG(DEBUG, __FUNCTION__, "retrycount is ", retrycount);
                        error = telux::common::ErrorCode::PASSWORD_INCORRECT;

                        rootObj["ICardManager"]["ChangeCardPassword"]\
                            ["retryCountPin1"] = retrycount;
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                        IsCardInfoChanged = true;
                    }
                }
            }
        } else if (locktype == ::tel::CardLockType::PIN2) {
            password = rootObj["ICardManager"]["Pin2password"].asString();
            retrycount = rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin2"].asInt();
            if(oldPwd == password && (retrycount != -1)) {
                rootObj["ICardManager"]["Pin2password"] = newPwd;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin2"].asInt();
            } else {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin2"].asInt();
                if (retrycount < 0) {
                    LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                    error = telux::common::ErrorCode::PIN_BLOCKED;
                } else {
                    if(retrycount >= -1) {
                        retrycount--;
                        error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                    }
                    rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin2"] = retrycount;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                }
            }
        } else {
            LOG(DEBUG, __FUNCTION__,"Not Supported LockType");
            error = telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
        }
    }

    //Create response
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    response->set_delay(cbDelay);
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_retry_count(retrycount);
    response->set_iscardinfochanged(IsCardInfoChanged);

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::UnlockByPin(ServerContext* context,
    const ::tel::UnlockByPinRequest* request,
    tel::UnlockByPinReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    ::tel::CardLockType locktype = request->lock_type();
    string pwd = request->pin();
    string appId = request->aid();
    std::string password;
    int retrycount;
    bool IsCardInfoChanged = false;
    uint32_t cbDelay;
    telux::common::Status status;
    telux::common::ErrorCode error;
    std::string apiname = "UnlockCardByPin";
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        if(locktype == ::tel::CardLockType::PIN1) {
            password = rootObj["ICardManager"]["Pin1password"].asString();
            retrycount = rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin1"].asInt();
            if((pwd == password) && (retrycount != -1)) {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin1"].asInt();
            } else {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin1"].asInt();
                if (retrycount < 0) {
                    LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                    error = telux::common::ErrorCode::PIN_BLOCKED;
                    //Update the app state to puk for app
                    int size = rootObj["ICardManager"]["GetApplications"].size();
                    for (int i = 0; i < size; i++) {
                        std::string id = rootObj["ICardManager"]["GetApplications"]\
                            [i]["appId"].asString();
                        if (id == appId) {
                            rootObj["ICardManager"]["GetApplications"][i]\
                                ["appState"] = 3; //puk state
                            JsonParser::writeToJsonFile(rootObj, jsonfilename);
                            jsonObjSystemStateSlot_[phoneId] = rootObj;
                            IsCardInfoChanged = true;
                            break;
                        } else {
                            LOG(DEBUG, __FUNCTION__,"No matching appId found");
                            error = telux::common::ErrorCode::INVALID_ARG;
                        }
                    }
                } else {
                    if(retrycount >= -1) {
                        retrycount--;
                        error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                    }
                    rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin1"] = retrycount;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    IsCardInfoChanged = true;
                }
            }
        } else if (locktype == ::tel::CardLockType::PIN2) {
            password = rootObj["ICardManager"]["Pin2password"].asString();
            retrycount = rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin2"].asInt();
            if(pwd == password && (retrycount != -1)) {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin2"].asInt();
            } else {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin2"].asInt();
                if (retrycount < 0) {
                LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                error = telux::common::ErrorCode::PIN_BLOCKED;
                } else {
                    if(retrycount >= -1) {
                        retrycount--;
                        error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                    }
                    rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin2"] = retrycount;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                }
            }
        } else {
            LOG(DEBUG, __FUNCTION__,"Not Supported LockType");
            error = telux::common::ErrorCode::INVALID_ARG;
        }
    }
    //Create response
    response->set_retry_count(retrycount);
    response->set_iscardinfochanged(IsCardInfoChanged);
    response->set_error(static_cast<tel::ErrorCode>(error));
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_delay(cbDelay);
    response->set_status(static_cast<tel::Status>(status));

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::UnlockByPuk(ServerContext* context,
    const ::tel::UnlockByPukRequest* request,
    tel::UnlockByPukReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    ::tel::CardLockType locktype = request->lock_type();
    string pwd = request->new_pin();
    string appId = request->aid();
    string puk = request->puk();
    std::string password;
    int retrycount;
    bool IsCardInfoChanged = false;
    telux::common::ErrorCode error;
    telux::common::Status status;
    uint32_t cbDelay;
    std::string apiname = "UnlockCardByPuk";
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        if(locktype == ::tel::CardLockType::PUK1) {
            password = rootObj["ICardManager"]["Puk1password"].asString();
            retrycount = rootObj["ICardManager"]["UnlockCardByPuk"]["retryCountPin1"].asInt();
            if((puk == password) && (retrycount != -1)) {
                rootObj["ICardManager"]["Pin1password"] = pwd;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
                rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin1"] = 3;
                //Update the app state to puk for app
                int size = rootObj["ICardManager"]["GetApplications"].size();
                for (int i = 0; i < size; i++) {
                    std::string id = rootObj["ICardManager"]["GetApplications"][i]\
                        ["appId"].asString();
                    if (id == appId) {
                        rootObj["ICardManager"]["GetApplications"][i]\
                            ["appState"] = 5; //ready state
                        JsonParser::writeToJsonFile(rootObj, jsonfilename);
                        jsonObjSystemStateSlot_[phoneId] = rootObj;
                        IsCardInfoChanged = true;
                        break;
                    } else {
                        LOG(DEBUG, __FUNCTION__,"No matching appId found");
                        error = telux::common::ErrorCode::INVALID_ARG;
                    }
                }
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
                retrycount = rootObj["ICardManager"]["UnlockCardByPuk"]["retryCountPin1"].asInt();
            } else {
                retrycount = rootObj["ICardManager"]["UnlockCardByPuk"]["retryCountPin1"].asInt();
                if (retrycount < 0) {
                    LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                    error = telux::common::ErrorCode::PIN_BLOCKED;
                }
                else {
                    if(retrycount >= -1) {
                        retrycount--;
                        error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                    }
                    rootObj["ICardManager"]["UnlockCardByPuk"]["retryCountPin1"] = retrycount;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    IsCardInfoChanged = true;
                }
            }
        } else if (locktype == ::tel::CardLockType::PUK2) {
            password = rootObj["ICardManager"]["Puk2password"].asString();
            retrycount = rootObj["ICardManager"]["UnlockCardByPuk"]["retryCountPin2"].asInt();
            if((puk == password) && (retrycount != -1)) {
                rootObj["ICardManager"]["Pin2password"] = pwd;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
                rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin2"] = 3;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
                retrycount = rootObj["ICardManager"]["UnlockCardByPuk"]["retryCountPin2"].asInt();
            } else {
                retrycount = rootObj["ICardManager"]["UnlockCardByPuk"]["retryCountPin2"].asInt();
                if (retrycount < 0) {
                    LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                    error = telux::common::ErrorCode::PIN_BLOCKED;
                }
                else {
                    if(retrycount >= -1) {
                        retrycount--;
                        error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                    }
                    rootObj["ICardManager"]["UnlockCardByPuk"]["retryCountPin2"] = retrycount;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    IsCardInfoChanged = true;
                }
            }
        } else {
            LOG(DEBUG, __FUNCTION__,"Not Supported LockType");
            error = telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
        }
    }
    //Create response
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    response->set_iscallback(iscallback);
    response->set_delay(cbDelay);
    response->set_retry_count(retrycount);
    response->set_iscardinfochanged(IsCardInfoChanged);

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::SetCardLock(ServerContext* context,
    const ::tel::SetCardLockRequest* request, tel::SetCardLockReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    ::tel::CardLockType locktype = request->lock_type();
    string pwd = request->pwd();
    bool enable = request->enable();
    string appId = request->aid();
    std::string password;
    int retrycount;
    bool IsCardInfoChanged = false;
    telux::common::ErrorCode error;
    telux::common::Status status;
    uint32_t cbDelay;
    std::string apiname = "SetCardLock";
    CommonUtils::getValues(jsonObjApiResponse, "ICardManager", apiname, status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        if(locktype == ::tel::CardLockType::PIN1) {
            password = rootObj["ICardManager"]["Pin1password"].asString();
            retrycount = rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin1"].asInt();
            if((pwd == password) && (retrycount != -1)) {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin1"].asInt();
                rootObj["ICardManager"]["SetCardLock"]["isPin1Available"] = enable;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
            } else {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin1"].asInt();
                if (retrycount < 0) {
                    LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                    error = telux::common::ErrorCode::PIN_BLOCKED;
                    //Update the app state to puk for app
                    int size = rootObj["ICardManager"]["GetApplications"].size();
                    for (int i = 0; i < size; i++) {
                        std::string id = rootObj["ICardManager"]["GetApplications"][i]\
                            ["appId"].asString();
                        if (id == appId) {
                            rootObj["ICardManager"]["GetApplications"]\
                                [i]["appState"] = 3; //puk state
                            JsonParser::writeToJsonFile(rootObj, jsonfilename);
                            jsonObjSystemStateSlot_[phoneId] = rootObj;
                            IsCardInfoChanged = true;
                            break;
                        } else {
                            LOG(DEBUG, __FUNCTION__,"No matching appId found");
                            error = telux::common::ErrorCode::INVALID_ARG;
                        }
                    }
                }
                else {
                    if(retrycount >= -1) {
                        retrycount--;
                        error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                    }
                    rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin1"] = retrycount;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                    IsCardInfoChanged = true;
                }
            }
        } else if (locktype == ::tel::CardLockType::FDN) {
            password = rootObj["ICardManager"]["Pin2password"].asString();
            retrycount = rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin2"].asInt();
            if(pwd == password && (retrycount != -1)) {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin2"].asInt();
                rootObj["ICardManager"]["SetCardLock"]["isPin2Available"] = enable;
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
            } else {
                retrycount = rootObj["ICardManager"]["ChangeCardPassword"]\
                    ["retryCountPin2"].asInt();
                if (retrycount < 0) {
                    LOG(DEBUG, __FUNCTION__,"Sim Card is blocked");
                    error = telux::common::ErrorCode::PIN_BLOCKED;
                } else {
                    if(retrycount >= -1) {
                        retrycount--;
                        error = telux::common::ErrorCode::PASSWORD_INCORRECT;
                    }
                    rootObj["ICardManager"]["ChangeCardPassword"]["retryCountPin2"] = retrycount;
                    JsonParser::writeToJsonFile(rootObj, jsonfilename);
                    jsonObjSystemStateSlot_[phoneId] = rootObj;
                }
            }
        } else {
            LOG(DEBUG, __FUNCTION__,"Not Supported LockType");
            error = telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
        }
    }
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_status(static_cast<tel::Status>(status));
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_delay(cbDelay);
    response->set_retry_count(retrycount);
    response->set_iscardinfochanged(IsCardInfoChanged);

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::QueryPin1Lock(ServerContext* context,
    const ::tel::QueryPin1LockRequest* request, tel::QueryPin1LockReply* response) {
    LOG(DEBUG, __FUNCTION__);
        int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    getJsonForSystemData(phoneId, jsonfilename, rootObj);

    telux::common::Status status;
    telux::common::ErrorCode error;
    uint32_t cbDelay;
    std::string apiname = "QueryPin1LockState";
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        error, cbDelay );

    bool state = rootObj["ICardManager"]["SetCardLock"]["isPin1Available"].asBool();
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);

    response->set_iscallback(iscallback);
    response->set_state(state);
    response->set_status(static_cast<tel::Status>(status));
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_delay(cbDelay);

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::QueryFdnLock(ServerContext* context,
    const ::tel::QueryFdnLockRequest* request,tel::QueryFdnLockReply* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    telux::common::Status status;
    telux::common::ErrorCode errorCodefromUser;
    uint32_t cbDelay;

    std::string apiname = "QueryFdnLockState";
    CommonUtils::getValues(jsonObjApiResponse,"ICardManager", apiname, status,
        errorCodefromUser, cbDelay );
    bool state = rootObj["ICardManager"]["SetCardLock"]["fdnState"].asBool();
    bool isAvailable = rootObj["ICardManager"]["SetCardLock"]["isPin2Available"].asBool();
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_delay(cbDelay);
    response->set_iscallback(iscallback);
    response->set_state(state);
    response->set_is_available(isAvailable);
    response->set_error(static_cast<tel::ErrorCode>(errorCodefromUser));
    response->set_status(static_cast<tel::Status>(status));

    return grpc::Status::OK;
}

grpc::Status CardManagerServerImpl::CardPower(ServerContext* context,
    const ::tel::CardPowerRequest* request,
    tel::CardPowerResponse* response) {
    LOG(DEBUG, __FUNCTION__);
    int phoneId = request->phone_id();
    std::string jsonfilename = "";
    std::string apiname = "SetCardPower";
    Json::Value rootObj;
    std::string jsonObjApiResponseFileName = "";
    Json::Value jsonObjApiResponse;
    getJsonForApiResponseSlot(phoneId, jsonObjApiResponseFileName, jsonObjApiResponse);
    getJsonForSystemData(phoneId, jsonfilename, rootObj);
    telux::common::Status status;
    telux::common::ErrorCode error;
    uint32_t cbDelay;
    CommonUtils::getValues(jsonObjApiResponse, "ICardManager", apiname, status,
        error, cbDelay );

    if(status == telux::common::Status::SUCCESS) {
        bool powerup = request->powerup();
        bool currentstate = rootObj["ICardManager"]["SetCardPower"]["cardPowerState"].asBool();
        if (currentstate != powerup) {
            rootObj["ICardManager"]["SetCardPower"]["cardPowerState"] = powerup;
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[phoneId] = rootObj;
            if(powerup) {
                rootObj["ICardManager"]["GetState"]["cardState"] = 1;
                    //Update Card State to PRESENT
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
            } else {
                rootObj["ICardManager"]["GetState"]["cardState"] = 0;
                    //Update Card State to ABSENT
                JsonParser::writeToJsonFile(rootObj, jsonfilename);
                jsonObjSystemStateSlot_[phoneId] = rootObj;
            }
        } else {
            error = telux::common::ErrorCode::NO_EFFECT;
        }
    }
    bool iscallback = isCallbackNeeded(jsonObjApiResponse, apiname);
    response->set_iscallback(iscallback);
    response->set_error(static_cast<tel::ErrorCode>(error));
    response->set_delay(cbDelay);
    response->set_status(static_cast<tel::Status>(status));

    return grpc::Status::OK;
}

tel::ErrorCode CardManagerServerImpl::findmatchingrecordDF (Json::Value rootObj, int& size,
    int& recordNum, uint16_t& fileId, int& i) {
    tel::ErrorCode error = tel::ErrorCode::ERROR_CODE_SUCCESS;
    while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"]\
            [i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            int num = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i]\
                ["numberOfRecords"].asInt();
            LOG(DEBUG, __FUNCTION__,"NumberOfRecords ", num);
            if(recordNum <= num ) {
                error = tel::ErrorCode::ERROR_CODE_SUCCESS;
                break;
            } else {
                error = tel::ErrorCode::GENERIC_FAILURE;
                LOG(DEBUG, __FUNCTION__, "Invalid Record");
                break;
            }
        } else {
            LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
            int num = rootObj["ICardManager"]["EFs"]["DFLinearFixedEFRecords"][i]\
                ["numberOfRecords"].asInt();
            i = i + num + 1;
            LOG(DEBUG, __FUNCTION__,"Incremented value is ", i );
        }
    }
    if(i == size) {
        LOG(DEBUG, __FUNCTION__,"Valid record not found ", i );
        error = tel::ErrorCode::GENERIC_FAILURE;
    }
    return error;
}
void CardManagerServerImpl::onEventUpdate(std::string event) {
    std::string token;
    if (EVENT_FLAG == EventParserUtil::getNextToken(event, DEFAULT_DELIMITER)) {
        token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
        handleEvent(token, event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
}

void CardManagerServerImpl::handleEvent(std::string token , std::string event) {
    LOG(DEBUG, __FUNCTION__, "The received event is: \"",token,"\"");
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token, "The leftover string is: ", event);
    if (token == "cardInfoChanged") {
        handleCardInfoChanged(event);
    }
}

void CardManagerServerImpl::handleCardInfoChanged(std::string eventParams) {
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    int slotId;
    std::string jsonfilename = "";
    std::string apiname = "SetCardPower";
    Json::Value rootObj;
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        slotId = 1;
    } else {
        try {
            slotId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);
    // Fetch card power
    int input;
    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "Card power input not passed, assuming power ON");
        input = true;
    } else {
        try {
            input = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    getJsonForSystemData(slotId, jsonfilename, rootObj);
    bool cardpower = static_cast<bool>(input);
    LOG(DEBUG, __FUNCTION__, "The fetched card power state id is: ", cardpower);
    bool currentstate = rootObj["ICardManager"]["SetCardPower"]["cardPowerState"].asBool();
    if (currentstate != cardpower) {
        rootObj["ICardManager"]["SetCardPower"]["cardPowerState"] = cardpower;
        JsonParser::writeToJsonFile(rootObj, jsonfilename);
        jsonObjSystemStateSlot_[slotId] = rootObj;
        if(cardpower) {
            rootObj["ICardManager"]["GetState"]["cardState"] = 1;
                //Update Card State to PRESENT
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[slotId] = rootObj;
        } else {
            rootObj["ICardManager"]["GetState"]["cardState"] = 0;
                //Update Card State to ABSENT
            JsonParser::writeToJsonFile(rootObj, jsonfilename);
            jsonObjSystemStateSlot_[slotId] = rootObj;
        }
    } else {
         LOG(DEBUG, __FUNCTION__, "No change in card state ");
    }
}