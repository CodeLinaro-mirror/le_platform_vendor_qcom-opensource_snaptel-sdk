/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * Sample application to demonstrate how to:
 * 1. Generate HMAC key
 * 2. Sign given data using this key
 * 3. Verify data using this key
 */

#include <iostream>

#include <telux/sec/CryptoDefines.hpp>
#include <telux/sec/SecurityFactory.hpp>
#include <telux/sec/CryptoManager.hpp>
#include <telux/sec/CryptoParamBuilder.hpp>

int generateHMACKey(
    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr, std::vector<uint8_t> &kb) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    // Define parameters for the key
    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_HMAC)
             .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_SIGN
                                 | telux::sec::CryptoOperation::CRYPTO_OP_VERIFY)
             .setKeySize(128)
             .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
             .setMinimumMacLength(64)
             .build();

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate HMAC key, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int signDataUsingHMACKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
    std::vector<uint8_t> kb, std::vector<uint8_t> pt, std::vector<uint8_t> &sg) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_HMAC)
             .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
             .setMacLength(128)
             .build();

    ec = cryptMgr->signData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't sign data, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int verifyDataUsingHMACSignature(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
    std::vector<uint8_t> kb, std::vector<uint8_t> pt, std::vector<uint8_t> sg) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_HMAC)
             .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
             .build();

    ec = cryptMgr->verifyData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        if (ec == telux::common::ErrorCode::VERIFICATION_FAILED)
            std::cout << "Invalid signature for given data!" << std::endl;
        else
            std::cout << "Can't verify data, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    std::cout << "Data verified!" << std::endl;
    return 0;
}

int main(int argc, char **argv) {

    int ret;
    telux::common::ErrorCode ec;
    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr;

    std::vector<uint8_t> sg;
    std::vector<uint8_t> kb;

    // Specify data to be signed and verified
    std::vector<uint8_t> pt{'h', 'e', 'l', 'l', 'o'};

    // Get SecurityFactory instance
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    // Get CryptoManager instance
    cryptMgr = secFact.getCryptoManager(ec);
    if (!cryptMgr) {
        std::cout << "Can't allocate CryptoManager, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    // Generate HMAC symmetric key
    ret = generateHMACKey(cryptMgr, kb);
    if (ret) {
        return ret;
    }

    // Sign the given data
    ret = signDataUsingHMACKey(cryptMgr, kb, pt, sg);
    if (ret) {
        return ret;
    }

    // Verify if signature is valid or not for the given data
    ret = verifyDataUsingHMACSignature(cryptMgr, kb, pt, sg);
    if (ret) {
        return ret;
    }

    return ret;
}
