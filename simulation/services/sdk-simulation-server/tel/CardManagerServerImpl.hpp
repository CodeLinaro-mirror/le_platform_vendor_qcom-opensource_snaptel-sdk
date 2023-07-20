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
#include "../../../libs/common/Logger.hpp"
#include "../../../libs/common/JsonParser.hpp"
#include "../../../libs/common/ResponseHandler.hpp"
#include "../../../libs/tel/CardFileHandlerStub.hpp"
#include "Helper.hpp"
#include "../../../protos/proto-src/tel.grpc.pb.h"
#include "../../../libs/common/CommonUtils.hpp"
#include "../../libs/common/event-manager/EventManager.hpp"


using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using tel::CardService;
using tel::ServiceState;
using tel::GetServiceStatusReply;


class CardManagerServerImpl final : public tel::CardService::Service,
                                    public IEventListener,
                                    public std::enable_shared_from_this<CardManagerServerImpl> {
 public:
    CardManagerServerImpl();
    grpc::Status InitService(ServerContext *context, const google::protobuf::Empty *request,
        tel::GetServiceStatusReply* response) override ;
    grpc::Status GetServiceStatus(ServerContext* context, const google::protobuf::Empty* request,
        tel::GetServiceStatusReply* response) override;
    grpc::Status IsSubsystemReady(ServerContext* context, const google::protobuf::Empty* request,
        tel::IsSubsystemReadyReply* response) override;
    grpc::Status GetCardState(ServerContext *context, const tel::GetCardStateRequest *request,
        tel::GetCardStateReply *reply) override;
    grpc::Status ReadEFLinearFixed(ServerContext* context,
        const tel::ReadEFLinearFixedRequest* request,
        tel::ReadEFLinearFixedReply* response) override;
    grpc::Status ReadEFLinearFixedAll(ServerContext* context,
        const tel::ReadEFLinearFixedAllRequest*
        request, tel::ReadEFLinearFixedAllReply* response) override;
    grpc::Status ReadEFTransparent(ServerContext* context, const tel::ReadEFTransparentRequest*
        request, tel::ReadEFTransparentReply* response) override;
    grpc::Status WriteEFLinearFixed(ServerContext* context, const tel::WriteEFLinearFixedRequest*
        request, tel::WriteEFLinearFixedReply* response) override;
    grpc::Status WriteEFTransparent(ServerContext* context, const tel::WriteEFTransparentRequest*
        request, tel::WriteEFTransparentReply* response) override;
    grpc::Status RequestEFAttributes(ServerContext* context, const tel::EFAttributesRequest*
        request, tel::RequestEFAttributesReply* response) override;
    grpc::Status OpenLogicalChannel(ServerContext* context, const tel::OpenLogicalChannelRequest*
        request, tel::OpenLogicalChannelReply* response) override;
    grpc::Status CloseLogicalChannel(ServerContext* context, const tel::CloseLogicalChannelRequest*
        request, tel::CloseLogicalChannelReply* response) override;
    grpc::Status TransmitAPDU(ServerContext* context, const tel::TransmitAPDURequest* request,
        tel::TransmitAPDUReply* response) override;
    grpc::Status TransmitBasicAPDU(ServerContext* context,
        const tel::TransmitBasicAPDURequest* request,
        tel::TransmitBasicAPDUReply* response) override;
    grpc::Status exchangeSimIO(ServerContext* context, const ::tel::exchangeSimIORequest* request,
        tel::exchangeSimIOReply* response) override;
    grpc::Status requestEid(ServerContext* context, const ::tel::requestEidRequest* request,
        tel::requestEidReply* response) override;
    grpc::Status updateSimStatus(ServerContext* context,
        const ::tel::updateSimStatusRequest* request,
        tel::updateSimStatusReply* response) override;
    grpc::Status SetCardLock(ServerContext* context, const tel::SetCardLockRequest* request,
        tel::SetCardLockReply* response) override;
    grpc::Status QueryPin1Lock(ServerContext* context, const tel::QueryPin1LockRequest* request,
        tel::QueryPin1LockReply* response) override;
    grpc::Status ChangePinLock(ServerContext* context, const tel::ChangePinLockRequest* request,
        tel::ChangePinLockReply* response) override;
    grpc::Status UnlockByPin(ServerContext* context, const tel::UnlockByPinRequest* request,
        tel::UnlockByPinReply* response) override;
    grpc::Status UnlockByPuk(ServerContext* context, const tel::UnlockByPukRequest* request,
        tel::UnlockByPukReply* response) override;
    grpc::Status QueryFdnLock(ServerContext* context, const tel::QueryFdnLockRequest* request,
        tel::QueryFdnLockReply* response) override;
    grpc::Status CardPower(ServerContext* context, const ::tel::CardPowerRequest* request,
        tel::CardPowerResponse* response) override;
    void onEventUpdate(std::string event);

    template <typename T>
    tel::ErrorCode findmatchingrecordADF (Json::Value rootObj, T response,
    int& size, int& index, int& recordNum, uint16_t& fileId, int& i) {
        tel::ErrorCode error = tel::ErrorCode::ERROR_CODE_SUCCESS;
        while (i < size ) {
        uint16_t tmpfileId = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"]\
            [i]["fileId"].asInt();
        if (tmpfileId == fileId) {
            int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i]\
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
            int num = rootObj["ICardManager"]["EFs"]["ADF"][index]["LinearFixedEFFiles"][i]\
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
 private:
    Json::Value rootObjSystemStateSlot1_;
    Json::Value rootObjSystemStateSlot2_;
    Json::Value rootObjApiResponseSlot1_;
    Json::Value rootObjApiResponseSlot2_;
    std::map <int, Json::Value> jsonObjSystemStateSlot_;
    std::map <int, std::string> jsonObjSystemStateFileName_;
    std::map <int, Json::Value> jsonObjApiResponseSlot_;
    std::map <int, std::string> jsonObjApiResponseFileName_;
    void readJson();
    bool isCallbackNeeded(Json::Value rootObj, std::string apiname);
    bool findAppId(Json::Value rootObj, const char* appid, int& index);
    tel::ErrorCode findmatchingrecordDF (Json::Value rootObj, int& size, int& recordNum,
        uint16_t& fileId, int& i);
    tel::ErrorCode getTransparentFileAttributes(Json::Value rootObj, int& i, uint16_t fileId,
        telux::tel::FileAttributes& attributes, int& index);
     tel::ErrorCode getLinearfixedFileAttributes(Json::Value rootObj, int& i, uint16_t fileId,
        telux::tel::FileAttributes& attributes, int& index );
    void getJsonForSystemData (int phoneId, std::string& jsonfilename, Json::Value& rootObj );
    void getJsonForApiResponseSlot(int phoneId, std::string& jsonfilename,
        Json::Value& rootObj );
    void handleEvent(std::string token , std::string event);
    void handleCardInfoChanged(std::string eventParams);
};

#endif // CARD_MANAGER_SERVER_HPP