/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       CardFileHandlerStub.hpp
 *
 * @brief      Implementation of ICardFileHandler
 *
 */

#ifndef CARD_FILEHANDLER_STUB_HPP
#define CARD_FILEHANDLER_STUB_HPP

#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include "common/AsyncTaskQueue.hpp"
#include "CardAppStub.hpp"
#include <telux/tel/CardManager.hpp>
#include <grpcpp/grpcpp.h>
#include "protos/proto-src/tel_simulation.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using telStub::CardService;

namespace telux {
namespace tel {

class CardFileHandlerStub : public ICardFileHandler {
 public:
    telux::common::Status readEFLinearFixed(std::string filePath, uint16_t fileId, int recordNum,
        std::string aid, EfOperationCallback callback) override;
    telux::common::Status readEFLinearFixedAll(std::string filePath, uint16_t fileId,
        std::string aid, EfReadAllRecordsCallback callback) override;
    telux::common::Status readEFTransparent(std::string filePath, uint16_t fileId, int size,
        std::string aid, EfOperationCallback callback) override;
    telux::common::Status writeEFLinearFixed(std::string filePath, uint16_t fileId, int recordNum,
        std::vector<uint8_t> data, std::string pin2, std::string aid,
        EfOperationCallback callback) override;
    telux::common::Status writeEFTransparent(std::string filePath, uint16_t fileId,
        std::vector<uint8_t> data, std::string aid, EfOperationCallback callback) override;
    telux::common::Status requestEFAttributes(EfType efType, std::string filePath, uint16_t fileId,
        std::string aid, EfGetFileAttributesCallback callback) override;
    SlotId getSlotId() override;
    telux::common::Status updateCardApps(std::vector<std::shared_ptr<CardAppStub>> cardApps);
    bool isAppReady(std::string aid);
    void cleanup();
    CardFileHandlerStub(SlotId slotId);

 private:
    SlotId slotId_;
    std::mutex mtx_;
    std::unique_ptr<::telStub::CardService::Stub> stub_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::vector<std::shared_ptr<CardAppStub>> cardApps_;
    void invokeCallback(EfOperationCallback callback, telux::common::ErrorCode error,
        telux::tel::IccResult iccresult, int cbDelay);
    void invokeCallback(EfReadAllRecordsCallback callback, telux::common::ErrorCode error,
        std::vector<IccResult> records, int cbDelay);
    void invokeCallback(EfGetFileAttributesCallback callback, telux::common::ErrorCode error,
        telux::tel::IccResult iccresult, telux::tel::FileAttributes attributes, int cbDelay);
};

}  // end of namespace tel

}  // end of namespace telux

#endif  // CARD_FILEHANDLER_STUB_HPP