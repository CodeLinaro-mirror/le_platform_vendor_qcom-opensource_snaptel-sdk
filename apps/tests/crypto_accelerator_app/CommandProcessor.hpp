/*
 * Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
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

#ifndef COMMANDPROCESSOR_HPP
#define COMMANDPROCESSOR_HPP

#include <telux/sec/SecurityFactory.hpp>

/*
 * Parameters required for ECC signature verification.
 */
struct VerificationRequest {
    uint32_t uniqueId;
    uint32_t timeout;
    telux::sec::Mode mode;
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
    telux::sec::Mode mode;
    telux::sec::RequestPriority priority;
    telux::sec::ECCCurve curve;
    std::vector<uint8_t> scalar;
    std::vector<uint8_t> multiplicandPointX;
    std::vector<uint8_t> multiplicandPointY;
    std::vector<uint8_t> addendPointX;
    std::vector<uint8_t> addendPointY;
};

class CommandProcessor {
 public:
    void verifyDigest(VerificationRequest request);

    void calculatePoint(CalculationRequest request);

 private:
    void verifyDigestSync(VerificationRequest request);
    void verifyDigestAsyncPoll(VerificationRequest request);
    void verifyDigestAsyncListener(VerificationRequest request);

    void calculatePointSync(CalculationRequest request);
    void calculatePointAsyncPoll(CalculationRequest request);
    void calculatePointAsyncListener(CalculationRequest request);
};

#endif  // COMMANDPROCESSOR_HPP
