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

/**
 * @file       SmsManagerServerImpl.hpp
 *             It handles solicited request for sending the SMS text canned data.
 *             Supports read, delete message, set tags for messages and stores incoming
 *             messages in SMS JSON database.
 *
 */

#ifndef SMS_MANAGER_SERVER_HPP
#define SMS_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include "../../../libs/common/Logger.hpp"
#include "../../../libs/common/JsonParser.hpp"
#include "../../../libs/common/ResponseHandler.hpp"
#include "Helper.hpp"
#include "../../../protos/proto-src/tel.grpc.pb.h"
#include "../../../libs/common/CommonUtils.hpp"
#include "../../libs/common/event-manager/EventManager.hpp"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using tel::SmsService;
using tel::ServiceState;
using tel::GetServiceStatusReply;

struct SmsMsg  {
    std::string text;
    std::string sender;
    std::string receiver;
    telux::tel::SmsEncoding encoding;
    std::string pdu;
    std::string pduBuffer;
    int messageInfoRefNumber;
    int messageInfoSegments;
    int messageInfoSegmentNumber;
    bool isMetaInfoValid;
    int msgIndex;
    telux::tel::SmsTagType tagType;
};

struct SmsDeliveryInfo {
    telux::common::ErrorCode errorCode;
    int cbDelay;
    int msgRef;
};

class SmsManagerServerImpl final : public tel::SmsService::Service,
                                   public IEventListener,
                                   public std::enable_shared_from_this<SmsManagerServerImpl> {
public:
    SmsManagerServerImpl();
    grpc::Status InitService(ServerContext *context,
        const ::tel::GetServiceStatusRequest* request ,
        tel::GetServiceStatusReply* response) override ;
    grpc::Status GetServiceStatus(ServerContext* context,
        const ::tel::GetServiceStatusRequest* request,
        tel::GetServiceStatusReply* response) override;
    grpc::Status SetSmscAddress(ServerContext* context, const tel::SetSmscAddressRequest* request,
        tel::SetSmscAddressReply* response) override;
    grpc::Status GetSmscAddress(ServerContext* context, const tel::GetSmscAddressRequest* request,
        tel::GetSmscAddressReply* response) override;
    grpc::Status RequestSmsMessageList(ServerContext* context,
        const ::tel::RequestSmsMessageListRequest* request,
        tel::RequestSmsMessageListReply* response) override;
    grpc::Status ReadMessage(ServerContext *context,
        const tel::ReadMessageRequest *request, tel::ReadMessageReply *reply) override;
    grpc::Status DeleteMessage(ServerContext *context, const tel::DeleteMessageRequest *request,
        tel::DeleteMessageRequestReply *response) override;
    grpc::Status SetPreferredStorage(ServerContext *context,
        const tel::SetPreferredStorageRequest *request,
        tel::SetPreferredStorageReply *response) override;
    grpc::Status RequestPreferredStorage(ServerContext *context,
        const tel::RequestPreferredStorageRequest *request,
        tel::RequestPreferredStorageReply *response) override;
    grpc::Status SetTag(ServerContext *context, const tel::SetTagRequest *request,
        tel::SetTagReply *response) override;
    grpc::Status RequestStorageDetails(ServerContext *context,
        const tel::RequestStorageDetailsRequest *request,
        tel::RequestStorageDetailsReply *response) override;
    grpc::Status GetMessageAttributes(ServerContext *context,
        const tel::GetMessageAttributesRequest *request,
        tel::GetMessageAttributesReply *response) override;
    grpc::Status IsMemoryFull(ServerContext *context,
        const tel::IsMemoryFullRequest *request, tel::IsMemoryFullReply *response) override;
    grpc::Status SendSmsWithoutSmsc(ServerContext *context,
        const tel::SendSmsWithoutSmscRequest *request, tel::SendSmsWithoutSmscReply *response)
        override;
    grpc::Status SendSms(ServerContext *context,
        const tel::SendSmsRequest *request, tel::SendSmsReply *response) override;
    grpc::Status SendRawSms(ServerContext *context,
    const tel::SendRawSmsRequest *request, tel::SendRawSmsReply *response) override;
    void onEventUpdate(std::string event);

private:
    Json::Value rootObjSystemStateSlot1;
    Json::Value rootObjSystemStateSlot2_;
    Json::Value rootObjApiResponseSlot1_;
    Json::Value rootObjApiResponseSlot2_;
    std::map <int, Json::Value> jsonObjSystemStateSlot_;
    std::map <int, std::string> jsonObjSystemStateFileName_;
    std::map <int, Json::Value> jsonObjApiResponseSlot_;
    std::map <int, std::string> jsonObjApiResponseFileName_;
    void readJson();
    bool isCallbackNeeded(Json::Value rootObj, std::string apiname);
    void getJsonForSystemData(int phoneId, std::string& jsonfilename, Json::Value& rootObj );
    void getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
        Json::Value& rootObj );
    int getSMSStorage(int phoneId);
    void parseMessageAtIndex(int phoneId, int index,SmsMsg& msg );
    telux::common::ErrorCode deletedSmsatIndex(int phoneId, std::vector<int> index);
    void handleEvent(std::string token , std::string event);
    void handleIncomingSms(std::string eventParams);
};
#endif // SMS_MANAGER_SERVER_HPP