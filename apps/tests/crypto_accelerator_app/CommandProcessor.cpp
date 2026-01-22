/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>
#include <cstdio>

#include "CommandProcessor.hpp"

void ResultAndSSRListener::onVerificationResult(
    uint32_t uniqueId, telux::common::ErrorCode ec, std::vector<uint8_t> resultData) {

    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "verification failed, err: " << static_cast<int>(ec)
                  << " uniqueId: " << uniqueId << std::endl;
    } else {
        std::cout << "verification passed, uniqueId: " << uniqueId << std::endl;
    }

    if (resultData.size()) {
        std::cout << "verification result: " << std::endl;
        uint8_t *data = resultData.data();
        for (uint32_t x = 0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x", data[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
        fflush(stdout);
    }

    barrier_.set_value();
}

void ResultAndSSRListener::onCalculationResult(
    uint32_t uniqueId, telux::common::ErrorCode ec, std::vector<uint8_t> resultData) {

    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "calculation failed, err: " << static_cast<int>(ec)
                  << " uniqueId: " << uniqueId << std::endl;
    } else {
        std::cout << "calculation done, uniqueId: " << uniqueId << std::endl;
    }

    if (resultData.size()) {
        std::cout << "calculation result: " << std::endl;
        uint8_t *data = resultData.data();
        for (uint32_t x = 0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x", data[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
        fflush(stdout);
    }

    barrier_.set_value();
}

void ResultAndSSRListener::onServiceStatusChange(telux::common::ServiceStatus newStatus) {

    std::cout << "New status: " << static_cast<int>(newStatus) << std::endl;
}

void ResultAndSSRListener::setResultSynchronizer(std::promise<void> barrier) {

    barrier_ = std::move(barrier);
}

telux::common::ErrorCode CommandProcessor::init(telux::sec::Mode mode) {

    telux::common::ErrorCode ec;

    mode_         = mode;
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    try {
        resultAndSSRListener_ = std::make_shared<ResultAndSSRListener>();
    } catch (const std::exception &e) {
        std::cout << "can't create ResultAndSSRListener" << std::endl;
        return telux::common::ErrorCode::NO_MEMORY;
    }

    cryptAccelMgr_ = secFact.getCryptoAcceleratorManager(ec, mode_, resultAndSSRListener_);
    if (!cryptAccelMgr_) {
        std::cout << "can't get ICryptoAcceleratorManager, err: " << static_cast<int>(ec)
                  << std::endl;
        return ec;
    }

    return telux::common::ErrorCode::SUCCESS;
}

void CommandProcessor::verifyDigestSync(VerificationRequest request) {

    uint8_t *data;
    telux::common::ErrorCode ec;
    std::vector<uint8_t> resultData;
    telux::sec::DataDigest digest{};
    telux::sec::ECCPoint publicKey{};
    telux::sec::Signature signature{};

    digest.digest       = request.digest.data();
    digest.digestLength = request.digest.size();

    publicKey.x       = request.publicKeyX.data();
    publicKey.xLength = request.publicKeyX.size();
    publicKey.y       = request.publicKeyY.data();
    publicKey.yLength = request.publicKeyY.size();

    signature.rSignature = request.signatureR.data();
    signature.sSignature = request.signatureS.data();
    signature.rsLength   = request.signatureR.size();

    ec = cryptAccelMgr_->eccVerifyDigest(digest, publicKey, signature, request.curve,
        request.uniqueId, request.priority, resultData);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "verification failed, err: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "verification passed." << std::endl;
    }
    fflush(stdout);

    if (resultData.size()) {
        std::cout << "verification result: " << std::endl;
        data = resultData.data();
        for (uint32_t x = 0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x", data[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }
}

void CommandProcessor::verifyDigestAsyncPoll(VerificationRequest request) {

    uint8_t *data;
    uint32_t numResultsRead = 0;
    telux::common::ErrorCode ec;
    telux::sec::DataDigest digest{};
    telux::sec::ECCPoint publicKey{};
    telux::sec::Signature signature{};
    std::vector<telux::sec::OperationResult> results(1);
    int timeout = -1;

    digest.digest       = request.digest.data();
    digest.digestLength = request.digest.size();

    publicKey.x       = request.publicKeyX.data();
    publicKey.xLength = request.publicKeyX.size();
    publicKey.y       = request.publicKeyY.data();
    publicKey.yLength = request.publicKeyY.size();

    signature.rSignature = request.signatureR.data();
    signature.sSignature = request.signatureS.data();
    signature.rsLength   = request.signatureR.size();

    ec = cryptAccelMgr_->eccPostDigestForVerification(
        digest, publicKey, signature, request.curve, request.uniqueId, request.priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, err: " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        return;
    }

    if (request.timeout) {
        timeout = request.timeout;
    }

    ec = cryptAccelMgr_->getAsyncResults(results, 1, timeout, numResultsRead);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't get result, " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        return;
    }

    std::cout << "uniqueId: " << telux::sec::ResultParser::getId(results[0]) << std::endl;

    std::cout << "operation type: "
              << static_cast<int>(telux::sec::ResultParser::getOperationType(results[0]))
              << std::endl;

    ec = telux::sec::ResultParser::getErrorCode(results[0]);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "verification failed, err: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "verification passed." << std::endl;
    }
    fflush(stdout);

    ec = telux::sec::ResultParser::getCAErrorCode(results[0]);
    std::cout << "CA err: " << static_cast<int>(ec) << std::endl;

    data = telux::sec::ResultParser::getData(results[0]);
    if (data) {
        std::cout << "verification result: " << std::endl;
        for (uint32_t x = 0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x", data[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }
}

void CommandProcessor::verifyDigestAsyncListener(VerificationRequest request) {

    telux::common::ErrorCode ec;
    telux::sec::DataDigest digest{};
    telux::sec::ECCPoint publicKey{};
    telux::sec::Signature signature{};

    std::promise<void> barrier;
    std::future<void> barrierFuture;

    barrier       = std::promise<void>();
    barrierFuture = barrier.get_future();
    resultAndSSRListener_->setResultSynchronizer(std::move(barrier));

    digest.digest       = request.digest.data();
    digest.digestLength = request.digest.size();

    publicKey.x       = request.publicKeyX.data();
    publicKey.xLength = request.publicKeyX.size();
    publicKey.y       = request.publicKeyY.data();
    publicKey.yLength = request.publicKeyY.size();

    signature.rSignature = request.signatureR.data();
    signature.sSignature = request.signatureS.data();
    signature.rsLength   = request.signatureR.size();

    ec = cryptAccelMgr_->eccPostDigestForVerification(
        digest, publicKey, signature, request.curve, request.uniqueId, request.priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, err: " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        return;
    }

    barrierFuture.wait();
}

void CommandProcessor::calculatePointSync(CalculationRequest request) {

    telux::sec::Scalar scalar{};
    telux::sec::ECCPoint addendPoint{};
    telux::sec::ECCPoint multiplicandPoint{};
    telux::common::ErrorCode ec;
    std::vector<uint8_t> resultData;

    scalar.scalar       = request.scalar.data();
    scalar.scalarLength = request.scalar.size();

    multiplicandPoint.x       = request.multiplicandPointX.data();
    multiplicandPoint.xLength = request.multiplicandPointX.size();
    multiplicandPoint.y       = request.multiplicandPointY.data();
    multiplicandPoint.yLength = request.multiplicandPointY.size();

    addendPoint.x       = request.addendPointX.data();
    addendPoint.xLength = request.addendPointX.size();
    addendPoint.y       = request.addendPointY.data();
    addendPoint.yLength = request.addendPointY.size();

    ec = cryptAccelMgr_->ecqvPointMultiplyAndAdd(multiplicandPoint, addendPoint, scalar,
        request.curve, request.uniqueId, request.priority, resultData);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "calculation failed, err: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "calculation done." << std::endl;
    }
    fflush(stdout);

    if (resultData.size()) {
        std::cout << "calculation result: " << std::endl;
        for (uint32_t x = 0; x < resultData.size(); x++) {
            printf("%02x", resultData.at(x) & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }
}

void CommandProcessor::calculatePointAsyncPoll(CalculationRequest request) {

    telux::sec::Scalar scalar{};
    telux::sec::ECCPoint addendPoint{};
    telux::sec::ECCPoint multiplicandPoint{};
    uint8_t *data;
    uint32_t numResultsRead = 0;
    telux::common::ErrorCode ec;
    std::vector<telux::sec::OperationResult> results(1);
    int timeout = -1;

    scalar.scalar       = request.scalar.data();
    scalar.scalarLength = request.scalar.size();

    multiplicandPoint.x       = request.multiplicandPointX.data();
    multiplicandPoint.xLength = request.multiplicandPointX.size();
    multiplicandPoint.y       = request.multiplicandPointY.data();
    multiplicandPoint.yLength = request.multiplicandPointY.size();

    addendPoint.x       = request.addendPointX.data();
    addendPoint.xLength = request.addendPointX.size();
    addendPoint.y       = request.addendPointY.data();
    addendPoint.yLength = request.addendPointY.size();

    ec = cryptAccelMgr_->ecqvPostDataForMultiplyAndAdd(
        multiplicandPoint, addendPoint, scalar, request.curve, request.uniqueId, request.priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        return;
    }

    if (request.timeout) {
        timeout = request.timeout;
    }

    ec = cryptAccelMgr_->getAsyncResults(results, 1, timeout, numResultsRead);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't get result, err: " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        return;
    }

    std::cout << "uniqueId: " << telux::sec::ResultParser::getId(results[0]) << std::endl;

    std::cout << "operation type: "
              << static_cast<int>(telux::sec::ResultParser::getOperationType(results[0]))
              << std::endl;

    ec = telux::sec::ResultParser::getErrorCode(results[0]);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "calculation failed, err: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "calculation done." << std::endl;
    }
    fflush(stdout);

    ec = telux::sec::ResultParser::getCAErrorCode(results[0]);
    std::cout << "CA err: " << static_cast<int>(ec) << std::endl;

    data = telux::sec::ResultParser::getData(results[0]);
    if (data) {
        std::cout << "calculation result: " << std::endl;
        for (uint32_t x = 0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x", data[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }
}

void CommandProcessor::calculatePointAsyncListener(CalculationRequest request) {

    telux::common::ErrorCode ec;
    telux::sec::Scalar scalar{};
    telux::sec::ECCPoint addendPoint{};
    telux::sec::ECCPoint multiplicandPoint{};

    std::promise<void> barrier;
    std::future<void> barrierFuture;

    barrier       = std::promise<void>();
    barrierFuture = barrier.get_future();
    resultAndSSRListener_->setResultSynchronizer(std::move(barrier));

    scalar.scalar       = request.scalar.data();
    scalar.scalarLength = request.scalar.size();

    multiplicandPoint.x       = request.multiplicandPointX.data();
    multiplicandPoint.xLength = request.multiplicandPointX.size();
    multiplicandPoint.y       = request.multiplicandPointY.data();
    multiplicandPoint.yLength = request.multiplicandPointY.size();

    addendPoint.x       = request.addendPointX.data();
    addendPoint.xLength = request.addendPointX.size();
    addendPoint.y       = request.addendPointY.data();
    addendPoint.yLength = request.addendPointY.size();

    ec = cryptAccelMgr_->ecqvPostDataForMultiplyAndAdd(
        multiplicandPoint, addendPoint, scalar, request.curve, request.uniqueId, request.priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, err: " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        return;
    }

    barrierFuture.wait();
}

void CommandProcessor::calculatePoint(CalculationRequest request) {

    switch (mode_) {
        case telux::sec::Mode::MODE_SYNC:
            return calculatePointSync(request);
        case telux::sec::Mode::MODE_ASYNC_POLL:
            return calculatePointAsyncPoll(request);
        case telux::sec::Mode::MODE_ASYNC_LISTENER:
            return calculatePointAsyncListener(request);
        default:
            std::cout << "invalid mode " << static_cast<int>(mode_) << std::endl;
    }
}

void CommandProcessor::verifyDigest(VerificationRequest request) {

    switch (mode_) {
        case telux::sec::Mode::MODE_SYNC:
            return verifyDigestSync(request);
        case telux::sec::Mode::MODE_ASYNC_POLL:
            return verifyDigestAsyncPoll(request);
        case telux::sec::Mode::MODE_ASYNC_LISTENER:
            return verifyDigestAsyncListener(request);
        default:
            std::cout << "invalid mode " << static_cast<int>(mode_) << std::endl;
    }
}
