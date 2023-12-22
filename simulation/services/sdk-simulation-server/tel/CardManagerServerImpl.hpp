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
 * @file       CardManagerServerImpl.hpp
 *
 *
 */

#ifndef CARD_MANAGER_SERVER_HPP
#define CARD_MANAGER_SERVER_HPP

#include <iostream>
#include <memory>
#include <string>

#include <grpcpp/ext/proto_server_reflection_plugin.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/health_check_service_interface.h>
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/CardDefines.hpp>
#include <telux/tel/CardFileHandler.hpp>
#include "libs/common/Logger.hpp"
#include "libs/common/JsonParser.hpp"
#include "libs/common/event-manager/EventParserUtil.hpp"
#include "libs/tel/CardFileHandlerStub.hpp"
#include "protos/proto-src/tel.grpc.pb.h"
#include "libs/common/CommonUtils.hpp"
#include "event/ServerEventManager.hpp"
#include "event/EventService.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using telStub::CardService;
using commonStub::ServiceStatus;
using commonStub::GetServiceStatusReply;


class CardManagerServerImpl final : public telStub::CardService::Service,
                                    public IServerEventListener,
                                    public std::enable_shared_from_this<CardManagerServerImpl> {
 public:
    CardManagerServerImpl();
    grpc::Status InitService(ServerContext *context, const google::protobuf::Empty *request,
        commonStub::GetServiceStatusReply* response) override ;
    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::GetServiceStatusReply* response) override;
    grpc::Status IsSubsystemReady(ServerContext* context, const google::protobuf::Empty* request,
        commonStub::IsSubsystemReadyReply* response) override;
    grpc::Status GetCardState(ServerContext *context, const telStub::GetCardStateRequest *request,
        telStub::GetCardStateReply *reply) override;
    grpc::Status ReadEFLinearFixed(ServerContext* context,
        const telStub::ReadEFLinearFixedRequest* request,
        telStub::ReadEFLinearFixedReply* response) override;
    grpc::Status ReadEFLinearFixedAll(ServerContext* context,
        const telStub::ReadEFLinearFixedAllRequest*
        request, telStub::ReadEFLinearFixedAllReply* response) override;
    grpc::Status ReadEFTransparent(ServerContext* context,
        const telStub::ReadEFTransparentRequest*
        request, telStub::ReadEFTransparentReply* response) override;
    grpc::Status WriteEFLinearFixed(ServerContext* context,
        const telStub::WriteEFLinearFixedRequest*
        request, telStub::WriteEFLinearFixedReply* response) override;
    grpc::Status WriteEFTransparent(ServerContext* context,
        const telStub::WriteEFTransparentRequest*
        request, telStub::WriteEFTransparentReply* response) override;
    grpc::Status RequestEFAttributes(ServerContext* context,
        const telStub::EFAttributesRequest*
        request, telStub::RequestEFAttributesReply* response) override;
    grpc::Status OpenLogicalChannel(ServerContext* context,
        const telStub::OpenLogicalChannelRequest*
        request, telStub::OpenLogicalChannelReply* response) override;
    grpc::Status CloseLogicalChannel(ServerContext* context,
        const telStub::CloseLogicalChannelRequest*
        request, telStub::CloseLogicalChannelReply* response) override;
    grpc::Status TransmitAPDU(ServerContext* context,
        const telStub::TransmitAPDURequest* request,
        telStub::TransmitAPDUReply* response) override;
    grpc::Status TransmitBasicAPDU(ServerContext* context,
        const telStub::TransmitBasicAPDURequest* request,
        telStub::TransmitBasicAPDUReply* response) override;
    grpc::Status exchangeSimIO(ServerContext* context,
        const ::telStub::exchangeSimIORequest* request,
        telStub::exchangeSimIOReply* response) override;
    grpc::Status requestEid(ServerContext* context,
        const ::telStub::requestEidRequest* request,
        telStub::requestEidReply* response) override;
    grpc::Status updateSimStatus(ServerContext* context,
        const ::telStub::updateSimStatusRequest* request,
        telStub::updateSimStatusReply* response) override;
    grpc::Status SetCardLock(ServerContext* context, const telStub::SetCardLockRequest* request,
        telStub::SetCardLockReply* response) override;
    grpc::Status QueryPin1Lock(ServerContext* context,
        const telStub::QueryPin1LockRequest* request,
        telStub::QueryPin1LockReply* response) override;
    grpc::Status ChangePinLock(ServerContext* context,
        const telStub::ChangePinLockRequest* request,
        telStub::ChangePinLockReply* response) override;
    grpc::Status UnlockByPin(ServerContext* context, const telStub::UnlockByPinRequest* request,
        telStub::UnlockByPinReply* response) override;
    grpc::Status UnlockByPuk(ServerContext* context, const telStub::UnlockByPukRequest* request,
        telStub::UnlockByPukReply* response) override;
    grpc::Status QueryFdnLock(ServerContext* context, const telStub::QueryFdnLockRequest* request,
        telStub::QueryFdnLockReply* response) override;
    grpc::Status CardPower(ServerContext* context, const ::telStub::CardPowerRequest* request,
        telStub::CardPowerResponse* response) override;
    void onEventUpdate(::eventService::UnsolicitedEvent event) override;

    template <typename T>
    commonStub::ErrorCode findmatchingrecordADF (Json::Value rootObj, T response,
    int& size, int& index, int& recordNum, uint16_t& fileId, int& i) {
        commonStub::ErrorCode error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
        while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
            [i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i]\
                ["numberOfRecords"].asInt();
            LOG(DEBUG, __FUNCTION__,"NumberOfRecords ", num);
            if(recordNum <= num ) {
                error = commonStub::ErrorCode::ERROR_CODE_SUCCESS;
                break;
            } else {
                error = commonStub::ErrorCode::GENERIC_FAILURE;
                LOG(DEBUG, __FUNCTION__, "Invalid Record");
                break;
            }
        } else {
            LOG(DEBUG, __FUNCTION__,"FileId not found ", i );
            int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i]\
                ["numberOfRecords"].asInt();
            i = i + num + 1;
            LOG(DEBUG, __FUNCTION__,"Incremented value is ", i );
        }
    }
    if(i == size) {
        LOG(DEBUG, __FUNCTION__,"Valid record not found ", i );
        error = commonStub::ErrorCode::GENERIC_FAILURE;
    }
    return error;
    }
 private:
    Json::Value rootObjSystemStateSlot1_;
    Json::Value rootObjSystemStateSlot2_;
    Json::Value rootObjApiResponseSlot1_;
    Json::Value rootObjApiResponseSlot2_;
    std::map <int, Json::Value> jsonObjSystemStateSlot_;
    std::map <int, std::string> jsonObjSystemStateFileName_;
    std::map <int, Json::Value> jsonObjApiResponseSlot_;
    std::map <int, std::string> jsonObjApiResponseFileName_;
    grpc::Status readJson();
    bool isCallbackNeeded(Json::Value rootObj, std::string apiname);
    bool findAppId(Json::Value rootObj, const char* appid, int& index);
    commonStub::ErrorCode findmatchingrecordDF (Json::Value rootObj, int& size, int& recordNum,
        uint16_t& fileId, int& i);
    commonStub::ErrorCode getTransparentFileAttributes(Json::Value rootObj,
        int& i, uint16_t fileId,
        telux::tel::FileAttributes& attributes, int& index);
    commonStub::ErrorCode getLinearfixedFileAttributes(Json::Value rootObj,
        int& i, uint16_t fileId,
        telux::tel::FileAttributes& attributes, int& index );
    void getJsonForSystemData (int phoneId, std::string& jsonfilename, Json::Value& rootObj );
    void getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
        Json::Value& rootObj );
    void handleEvent(std::string token , std::string event);
    void handleCardInfoChanged(std::string eventParams);
    void onEventUpdate(std::string event);
};

#endif // CARD_MANAGER_SERVER_HPP