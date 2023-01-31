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
    telux::sec::DataDigest digest;
    telux::sec::ECCPoint publicKey;
    telux::sec::Signature signature;
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
    telux::sec::ECCPoint multiplicandPoint;
    telux::sec::ECCPoint addendPoint;
    telux::sec::Scalar scalar;
};

class CommandProcessor {
 public:
    void verifyDigest(const VerificationRequest request);

    void calculatePoint(const CalculationRequest request);

 private:
    void verifyDigestSync(const VerificationRequest request);
    void verifyDigestAsyncPoll(const VerificationRequest request);
    void verifyDigestAsyncListener(const VerificationRequest request);

    void calculatePointSync(const CalculationRequest request);
    void calculatePointAsyncPoll(const CalculationRequest request);
    void calculatePointAsyncListener(const CalculationRequest request);
};

#endif  // COMMANDPROCESSOR_HPP
