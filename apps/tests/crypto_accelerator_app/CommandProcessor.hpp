/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef COMMANDPROCESSOR_HPP
#define COMMANDPROCESSOR_HPP

#include <future>

#include <telux/sec/SecurityFactory.hpp>

/*
 * Parameters required for ECC signature verification.
 */
struct VerificationRequest {
    uint32_t uniqueId;
    uint32_t timeout;
    telux::sec::RequestPriority priority;
    telux::sec::ECCCurve curve;
    std::vector<uint8_t> digest;
    std::vector<uint8_t> publicKeyX;
    std::vector<uint8_t> publicKeyY;
    std::vector<uint8_t> signatureR;
    std::vector<uint8_t> signatureS;
};

/*
 * Parameters required for ECQV point calculation.
 */
struct CalculationRequest {
    uint32_t uniqueId;
    uint32_t timeout;
    telux::sec::RequestPriority priority;
    telux::sec::ECCCurve curve;
    std::vector<uint8_t> scalar;
    std::vector<uint8_t> multiplicandPointX;
    std::vector<uint8_t> multiplicandPointY;
    std::vector<uint8_t> addendPointX;
    std::vector<uint8_t> addendPointY;
};

/*
 *  Listener class for receiving signature verification, point calculation result
 *  and SSR events.
 */
class ResultAndSSRListener : public telux::sec::ICryptoAcceleratorListener {

 public:
    void onVerificationResult(
        uint32_t uniqueId, telux::common::ErrorCode ec, std::vector<uint8_t> resultData) override;

    void onCalculationResult(
        uint32_t uniqueId, telux::common::ErrorCode ec, std::vector<uint8_t> resultData) override;

    void onServiceStatusChange(telux::common::ServiceStatus newStatus) override;

    void setResultSynchronizer(std::promise<void> barrier);

 private:
    std::promise<void> barrier_;
};

/*
 *  Does actual crypto operations using crypto acceleraot APIs.
 */
class CommandProcessor {

 public:
    telux::common::ErrorCode init(telux::sec::Mode mode);

    void verifyDigest(VerificationRequest request);

    void calculatePoint(CalculationRequest request);

 private:
    telux::sec::Mode mode_;
    std::shared_ptr<ResultAndSSRListener> resultAndSSRListener_;
    std::shared_ptr<telux::sec::ICryptoAcceleratorManager> cryptAccelMgr_;

    void verifyDigestSync(VerificationRequest request);
    void verifyDigestAsyncPoll(VerificationRequest request);
    void verifyDigestAsyncListener(VerificationRequest request);

    void calculatePointSync(CalculationRequest request);
    void calculatePointAsyncPoll(CalculationRequest request);
    void calculatePointAsyncListener(CalculationRequest request);
};

#endif  // COMMANDPROCESSOR_HPP
