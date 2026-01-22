/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
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
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "protos/proto-src/tel_simulation.grpc.pb.h"
#include "libs/common/CommonUtils.hpp"
#include "libs/common/AsyncTaskQueue.hpp"
#include "event/ServerEventManager.hpp"
#include <telux/tel/SmsManager.hpp>
#include "event/EventService.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using commonStub::GetServiceStatusReply;
using commonStub::ServiceStatus;
using telStub::SmsService;

struct SmsMsg {
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

class SmsManagerServerImpl final : public telStub::SmsService::Service,
                                   public IServerEventListener,
                                   public std::enable_shared_from_this<SmsManagerServerImpl> {
 public:
    SmsManagerServerImpl();
    grpc::Status InitService(ServerContext *context,
        const ::commonStub::GetServiceStatusRequest *request,
        commonStub::GetServiceStatusReply *response) override;
    grpc::Status GetServiceStatus(ServerContext *context,
        const ::commonStub::GetServiceStatusRequest *request,
        commonStub::GetServiceStatusReply *response) override;
    grpc::Status SetSmscAddress(ServerContext *context,
        const telStub::SetSmscAddressRequest *request,
        telStub::SetSmscAddressReply *response) override;
    grpc::Status GetSmscAddress(ServerContext *context,
        const telStub::GetSmscAddressRequest *request,
        telStub::GetSmscAddressReply *response) override;
    grpc::Status RequestSmsMessageList(ServerContext *context,
        const ::telStub::RequestSmsMessageListRequest *request,
        telStub::RequestSmsMessageListReply *response) override;
    grpc::Status ReadMessage(ServerContext *context, const telStub::ReadMessageRequest *request,
        telStub::ReadMessageReply *reply) override;
    grpc::Status DeleteMessage(ServerContext *context, const telStub::DeleteMessageRequest *request,
        telStub::DeleteMessageRequestReply *response) override;
    grpc::Status SetPreferredStorage(ServerContext *context,
        const telStub::SetPreferredStorageRequest *request,
        telStub::SetPreferredStorageReply *response) override;
    grpc::Status RequestPreferredStorage(ServerContext *context,
        const telStub::RequestPreferredStorageRequest *request,
        telStub::RequestPreferredStorageReply *response) override;
    grpc::Status SetTag(ServerContext *context, const telStub::SetTagRequest *request,
        telStub::SetTagReply *response) override;
    grpc::Status RequestStorageDetails(ServerContext *context,
        const telStub::RequestStorageDetailsRequest *request,
        telStub::RequestStorageDetailsReply *response) override;
    grpc::Status GetMessageAttributes(ServerContext *context,
        const telStub::GetMessageAttributesRequest *request,
        telStub::GetMessageAttributesReply *response) override;
    grpc::Status IsMemoryFull(ServerContext *context, const telStub::IsMemoryFullRequest *request,
        telStub::IsMemoryFullReply *response) override;
    grpc::Status SendSmsWithoutSmsc(ServerContext *context,
        const telStub::SendSmsWithoutSmscRequest *request,
        telStub::SendSmsWithoutSmscReply *response) override;
    grpc::Status SendSms(ServerContext *context, const telStub::SendSmsRequest *request,
        telStub::SendSmsReply *response) override;
    grpc::Status SendRawSms(ServerContext *context, const telStub::SendRawSmsRequest *request,
        telStub::SendRawSmsReply *response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent message);

 private:
    Json::Value rootObjSystemStateSlot1_;
    Json::Value rootObjSystemStateSlot2_;
    Json::Value rootObjApiResponseSlot1_;
    Json::Value rootObjApiResponseSlot2_;
    std::map<int, Json::Value> jsonObjSystemStateSlot_;
    std::map<int, std::string> jsonObjSystemStateFileName_;
    std::map<int, Json::Value> jsonObjApiResponseSlot_;
    std::map<int, std::string> jsonObjApiResponseFileName_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    grpc::Status readJson();
    bool isCallbackNeeded(Json::Value rootObj, std::string apiname);
    void getJsonForSystemData(int phoneId, std::string &jsonfilename, Json::Value &rootObj);
    void getJsonForApiResponseSlot(int phoneId, std::string &jsonfilename, Json::Value &rootObj);
    int getSMSStorage(int phoneId, std::string storageName);
    void parseMessageAtIndex(int phoneId, int index, SmsMsg &msg, std::string storageName);
    telux::common::ErrorCode deletedSmsatIndex(
        int phoneId, std::vector<int> index, std::string storageName);
    void handleIncomingSms(std::string eventParams);
    void handleMemoryFullEvent(std::string eventParams);
    void triggerIncomingSmsEvent(int phoneId, int numberOfSegments, int refNumber,
        int segmentNumber, int msgIndex, std::string tagType, std::string encoding,
        bool isMetaInfoValid, std::string pdu, std::string receiver, std::string sender,
        std::string text);
    /* Sorts the database of SMS messages.
     * Example: Current JSON datbase message Index elements are 1 , 3 , 4 , 2
     * New JSON datbase message Index elements are 1 , 2 , 3 , 4
     * It shifts all the elements of current database to right for indexes greater than new index
     * and pushes the data of new message on corresponding index in database.
     */
    void sortDatabase(int phoneId, Json::Value newSms, int index, std::string storageName);
    void onEventUpdate(std::string event);
    /* Returns the message index of new MT SMS database, where the new MT SMS can be stored */
    int getNewSmsIndex(int phoneId, std::string storageName);
};
#endif  // SMS_MANAGER_SERVER_HPP