/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * @file CryptoParamBuilder.hpp
 *
 * @brief Helps to setup input parameters for a given crypto operation.
 */

#ifndef TELUX_SEC_CRYPTOPARAMBUILDER_HPP
#define TELUX_SEC_CRYPTOPARAMBUILDER_HPP

#include <memory>
#include <vector>

#include <telux/sec/CryptoDefines.hpp>

namespace telux {
namespace sec {

/** @addtogroup telematics_sec_mgmt
 * @{ */

/**
 * CryptoParamBuilder helps to setup input parameters for a given crypto operation.
 */
class CryptoParamBuilder {

  public:
    /** Allocates an instance of CryptoParamBuilder. */
    CryptoParamBuilder();

    /** When generating the keys, specifies with which algorithm these keys will be used.
        For crypto operation, specifies the algorithm to use. Use @ref telux::sec::Algorithm
        enumeration to define this. */
    CryptoParamBuilder setAlgorithm(AlgorithmTypes algorithm);

    /** When generating the keys, specifies crypto operation(s) for which the key will be
        used. For crypto operation, specifies operation itself (encryption/decryption/
        signing/verification). Use @ref telux::sec::CryptoOperation enumeration to define
        this. Multiple operation values can be OR'ed. */
    CryptoParamBuilder setCryptoOperation(CryptoOperationTypes operation);

    /** When generating the keys, specifies the digest algorithm(s) that may be used with
        the key to perform signing and verification operations using RSA, ECDSA and HMAC
        keys For crypto operation, specifies exact digest algorithm to be used. Use
        @ref telux::sec::Digest enumeration to define this. Multiple values can be OR'ed. */
    CryptoParamBuilder setDigest(DigestTypes digest);

    /** When generating the keys, specifies the padding modes that may be used with the RSA
        and AES key. For crypto operation, specifies exact padding to be used. Use
        @ref telux::sec::Padding enumeration to define this. Multiple padding values can be
        OR'ed. */
    CryptoParamBuilder setPadding(PaddingTypes padding);

    /** When generating the keys, specifies size in bits, of the key, measured in the
        regular way for the key's algorithm. For RSA keys, specifies the size of the public
        modulus. For AES keys, specifies length of the secret key material. For HMAC key,
        it specifies the key size in bits. For EC, it is used to select the EC group. */
    CryptoParamBuilder setKeySize(int32_t keySize);

    /** When generating the keys, specifies minimum length of the MAC in bits that can be
        requested or verified with this key for HMAC keys and AES keys that support GCM
        mode.*/
    CryptoParamBuilder setMinimumMacLength(int32_t minMacLength);

    /** For crypto operation, specifies requested length of a MAC or GCM (which is guaranteed
        to be no less then minimum length of the MAC/GCM used when generating the key. */
    CryptoParamBuilder setMacLength(int32_t macLength);

    /** When generating the keys, specifies block cipher mode(s) with which this key can
        be used. For crypto operation, specifies exact block mode to be used. Use
        @ref telux::sec::BlockMode enumeration to define this. Multiple block mode
        values can be OR'ed. */
    CryptoParamBuilder setBlockMode(BlockModeTypes blockMode);

    /** When generating the keys using EC algorithm, only key size, only curve or both key
        size and curve can be specified. If only key size is specified, appropriate NIST
        curve is selected automatically. If only curve is specified, given curve is used.
        If both are specified then given curve is used and also key size is validated. */
    CryptoParamBuilder setCurve(int32_t curve);

    /** When generating the AES key, set this to true. And when performing AES crypto
        operations define initialization vector using iv field in this structure. */
    CryptoParamBuilder setCallerNonce(bool callerNonce);

    /** When generating the RSA key, specifies the value of the public exponent for an
        RSA key pair (necessary for all RSA keys). */
    CryptoParamBuilder setPublicExponent(uint64_t publicExponent);

    /** When performing AES crypto operations, specifies initialization vector to be used. */
    CryptoParamBuilder setInitVector(std::vector<uint8_t> initVector);

    /** When generating or importing a key, an optional arbitrary value can be supplied through
        this method. In all subsequent use of the key, this value must be supplied again. The
        data given is bound to the key cryptographically. This data ties the key to the
        caller. */
    CryptoParamBuilder setUniqueData(std::vector<uint8_t> uniqueData);

    /** When encrypting/decrypting data, this specifies optional associated data to be used.
        This is applicable only for AES-GCM algorithm. */
    CryptoParamBuilder setAssociatedData(std::vector<uint8_t> associatedData);

    /** Finally creates an instance of ICryptoParam based on the setter methods invoked
     * on the builder. After building the builder's state is resetted. */
    std::shared_ptr<ICryptoParam> build(void);

  private:
    std::shared_ptr<ICryptoParam> cryptoParam_;
};

/** @} */ /* end_addtogroup telematics_sec_mgmt */

}  // End of namespace sec
}  // End of namespace telux

#endif // TELUX_SEC_CRYPTOPARAMBUILDER_HPP
