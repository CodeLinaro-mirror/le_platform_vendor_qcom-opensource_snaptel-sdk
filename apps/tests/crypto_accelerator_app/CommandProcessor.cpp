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

#include <iostream>
#include <cstdio>
#include <future>

#include "CommandProcessor.hpp"

/*
 * Listener class for receiving signature verification and point calculation result.
 */
class ResultListener : public telux::sec::ICryptoAcceleratorListener {

 public:
    ResultListener(std::promise<void> barrier) {
        barrier_ = std::move(barrier);
    }

    void onVerificationResult(uint32_t uniqueId, telux::common::ErrorCode ec,
        std::vector<uint8_t> resultData) {

        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout <<
                "verification failed, err: " << static_cast<int>(ec) <<
                " uniqueId: " << uniqueId << std::endl;
        } else {
            std::cout << "verification passed, uniqueId: " << uniqueId << std::endl;
        }

        if (resultData.size()) {
            uint8_t *data = resultData.data();
            for (uint32_t x = 0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
                printf("%02x ", data[x] & 0xffU);
                if (x & !(x % 32)) {
                    printf("\n");
                }
            }
            printf("\n");
            fflush(stdout);
        }

        barrier_.set_value();
    }

    void onCalculationResult(uint32_t uniqueId, telux::common::ErrorCode ec,
        std::vector<uint8_t> resultData) {

        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout <<
                "calculation failed, err: " << static_cast<int>(ec) <<
                " uniqueId: " << uniqueId << std::endl;
        } else {
            std::cout << "calculation done, uniqueId: " << uniqueId << std::endl;
        }

        if (resultData.size()) {
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

 private:
    std::promise<void> barrier_;
};

void CommandProcessor::verifyDigestSync(const VerificationRequest request) {

    uint8_t *data;
    telux::common::ErrorCode ec;
    std::vector<uint8_t> resultData;
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;

    auto &secFact = telux::sec::SecurityFactory::getInstance();

    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec, telux::sec::Mode::MODE_SYNC);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err: " << static_cast<int>(ec) << std::endl;
        return;
    }

    ec = cryptAccelMgr->eccVerifyDigest(request.digest, request.publicKey, request.signature,
            request.curve, request.uniqueId, request.priority, resultData);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "verification failed, err: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "verification passed" << std::endl;
    }
    fflush(stdout);

    if (resultData.size()) {
        data = resultData.data();
        for (uint32_t x=0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x ", data[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }

    cryptAccelMgr.reset();
}

void CommandProcessor::verifyDigestAsyncPoll(const VerificationRequest request) {

    uint8_t *data;
    uint32_t numResultsRead = 0;
    telux::common::ErrorCode ec;
    std::vector<telux::sec::OperationResult> results(1);
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;
    int timeout = -1;

    auto &secFact = telux::sec::SecurityFactory::getInstance();

    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec, telux::sec::Mode::MODE_ASYNC_POLL);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err: " << static_cast<int>(ec) << std::endl;
        return;
    }

    ec = cryptAccelMgr->eccPostDigestForVerification(request.digest, request.publicKey,
            request.signature, request.curve, request.uniqueId, request.priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, err: " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        cryptAccelMgr.reset();
        return;
    }

    if (request.timeout) {
        timeout = request.timeout;
    }

    ec = cryptAccelMgr->getAsyncResults(results, 1, timeout, numResultsRead);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't get result, " << static_cast<int>(ec) << std::endl;
        cryptAccelMgr.reset();
        fflush(stdout);
        return;
    }

    std::cout << "uniqueId: " << telux::sec::ResultParser::getId(results[0]) << std::endl;

    std::cout << "operation type: " <<
        static_cast<int>(telux::sec::ResultParser::getOperationType(results[0])) << std::endl;

    ec = telux::sec::ResultParser::getErrorCode(results[0]);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "verification failed, err: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "verification passed" << std::endl;
    }
    fflush(stdout);

    ec = telux::sec::ResultParser::getCAErrorCode(results[0]);
    std::cout << "CA err: " << static_cast<int>(ec) << std::endl;

    data = telux::sec::ResultParser::getData(results[0]);
    if (data) {
        for (uint32_t x=0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x ", data[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }

    cryptAccelMgr.reset();
}

void CommandProcessor::verifyDigestAsyncListener(const VerificationRequest request) {

    telux::common::ErrorCode ec;
    std::shared_ptr<ResultListener> resultListener;
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;

    std::promise<void> barrier;
    std::future<void> barrierFuture = barrier.get_future();

    try {
        resultListener = std::make_shared<ResultListener>(std::move(barrier));
    } catch (const std::exception& e) {
        std::cout << "can't create ResultListener" << std::endl;
        return;
    }

    auto &secFact = telux::sec::SecurityFactory::getInstance();

    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec,
        telux::sec::Mode::MODE_ASYNC_LISTENER, resultListener);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err: " << static_cast<int>(ec) << std::endl;
        return;
    }

    ec = cryptAccelMgr->eccPostDigestForVerification(request.digest, request.publicKey,
            request.signature, request.curve, request.uniqueId, request.priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, err: " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        cryptAccelMgr.reset();
        return;
    }

    barrierFuture.wait();
}

void CommandProcessor::calculatePointSync(const CalculationRequest request) {

    telux::common::ErrorCode ec;
    std::vector<uint8_t> resultData;
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;

    auto &secFact = telux::sec::SecurityFactory::getInstance();

    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec, telux::sec::Mode::MODE_SYNC);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err " << static_cast<int>(ec) << std::endl;
        return;
    }

    ec = cryptAccelMgr->ecqvPointMultiplyAndAdd(request.multiplicandPoint,
            request.addendPoint, request.scalar, request.curve, request.uniqueId,
            request.priority, resultData);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "calculation failed, err: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "calculation done" << std::endl;
    }
    fflush(stdout);

    if (resultData.size()) {
        for (uint32_t x=0; x < resultData.size(); x++) {
            printf("%02x", resultData.at(x) & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }

    cryptAccelMgr.reset();
}

void CommandProcessor::calculatePointAsyncPoll(const CalculationRequest request) {

    uint8_t *data;
    uint32_t numResultsRead = 0;
    telux::common::ErrorCode ec;
    std::vector<telux::sec::OperationResult> results(1);
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;
    int timeout = -1;

    auto &secFact = telux::sec::SecurityFactory::getInstance();

    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec, telux::sec::Mode::MODE_ASYNC_POLL);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err: " << static_cast<int>(ec) << std::endl;
        return;
    }

    ec = cryptAccelMgr->ecqvPostDataForMultiplyAndAdd(request.multiplicandPoint,
            request.addendPoint, request.scalar, request.curve, request.uniqueId,
            request.priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        cryptAccelMgr.reset();
        return;
    }

    if (request.timeout) {
        timeout = request.timeout;
    }

    ec = cryptAccelMgr->getAsyncResults(results, 1, timeout, numResultsRead);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "can't get result, err: " << static_cast<int>(ec) << std::endl;
        cryptAccelMgr.reset();
        fflush(stdout);
        return;
    }

    std::cout << "uniqueId: " << telux::sec::ResultParser::getId(results[0]) << std::endl;

    std::cout << "operation type: " <<
        static_cast<int>(telux::sec::ResultParser::getOperationType(results[0])) << std::endl;

    ec = telux::sec::ResultParser::getErrorCode(results[0]);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "calculation failed, err: " << static_cast<int>(ec) << std::endl;
    } else {
        std::cout << "calculation done" << std::endl;
    }
    fflush(stdout);

    ec = telux::sec::ResultParser::getCAErrorCode(results[0]);
    std::cout << "CA err: " << static_cast<int>(ec) << std::endl;

    data = telux::sec::ResultParser::getData(results[0]);
    if (data) {
        for (uint32_t x=0; x < telux::sec::CA_RESULT_DATA_LENGTH; x++) {
            printf("%02x ", data[x] & 0xffU);
            if (x & !(x % 32)) {
                printf("\n");
            }
        }
        printf("\n");
    }

    cryptAccelMgr.reset();
}

void CommandProcessor::calculatePointAsyncListener(const CalculationRequest request) {

    telux::common::ErrorCode ec;
    std::shared_ptr<ResultListener> resultListener;
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr;

    std::promise<void> barrier;
    std::future<void> barrierFuture = barrier.get_future();

    try {
        resultListener = std::make_shared<ResultListener>(std::move(barrier));
    } catch (const std::exception& e) {
        std::cout << "can't create ResultListener" << std::endl;
        return;
    }

    auto &secFact = telux::sec::SecurityFactory::getInstance();

    cryptAccelMgr = secFact.getCryptoAcceleratorManager(ec,
        telux::sec::Mode::MODE_ASYNC_LISTENER, resultListener);
    if (!cryptAccelMgr) {
        std::cout <<
         "can't get ICryptoAcceleratorManager, err: " << static_cast<int>(ec) << std::endl;
        return;
    }

    ec = cryptAccelMgr->ecqvPostDataForMultiplyAndAdd(request.multiplicandPoint,
            request.addendPoint, request.scalar, request.curve, request.uniqueId,
            request.priority);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "request not sent, err: " << static_cast<int>(ec) << std::endl;
        fflush(stdout);
        cryptAccelMgr.reset();
        return;
    }

    barrierFuture.wait();
}

void CommandProcessor::calculatePoint(const CalculationRequest request) {

    switch (request.mode) {
        case telux::sec::Mode::MODE_SYNC:
            return calculatePointSync(request);
        case telux::sec::Mode::MODE_ASYNC_POLL:
            return calculatePointAsyncPoll(request);
        case telux::sec::Mode::MODE_ASYNC_LISTENER:
            return calculatePointAsyncListener(request);
        default:
            std::cout << "invalid mode " << static_cast<int>(request.mode) << std::endl;
    }
}

void CommandProcessor::verifyDigest(const VerificationRequest request) {

    switch (request.mode) {
        case telux::sec::Mode::MODE_SYNC:
            return verifyDigestSync(request);
        case telux::sec::Mode::MODE_ASYNC_POLL:
            return verifyDigestAsyncPoll(request);
        case telux::sec::Mode::MODE_ASYNC_LISTENER:
            return verifyDigestAsyncListener(request);
        default:
            std::cout << "invalid mode " << static_cast<int>(request.mode) << std::endl;
    }
}
