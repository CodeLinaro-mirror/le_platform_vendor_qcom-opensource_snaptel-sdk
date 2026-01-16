/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * Sample application to demonstrate how to:
 * 1. Generate EC asymmetric key
 * 2. Sign given data using this key
 * 3. Verify data using this key
 */

#include <iostream>

#include <telux/sec/SecurityFactory.hpp>
#include <telux/sec/CryptoManager.hpp>
#include <telux/sec/CryptoParamBuilder.hpp>

int generateECKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr, std::vector<uint8_t> &kb) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    // Define parameters for the key
    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_EC)
             .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_SIGN
                                 | telux::sec::CryptoOperation::CRYPTO_OP_VERIFY)
             .setKeySize(256)
             .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
             .build();

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate EC asym key, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int signDataUsingECKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
    std::vector<uint8_t> kb, std::vector<uint8_t> pt, std::vector<uint8_t> &sg) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_EC)
             .setDigest(telux::sec::Digest::DIGEST_SHA_2_256)
             .build();

    ec = cryptMgr->signData(cp, kb, pt, sg);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't sign data, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int verifyDataUsingECSignature(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
    std::vector<uint8_t> kb, std::vector<uint8_t> pt, std::vector<uint8_t> sg) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_EC)
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

    // Generate EC asymmetric key
    ret = generateECKey(cryptMgr, kb);
    if (ret) {
        return ret;
    }

    // Sign the given data
    ret = signDataUsingECKey(cryptMgr, kb, pt, sg);
    if (ret) {
        return ret;
    }

    // Verify if signature is valid or not for the given data
    ret = verifyDataUsingECSignature(cryptMgr, kb, pt, sg);
    if (ret) {
        return ret;
    }

    return ret;
}
