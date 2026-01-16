/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * Sample application to demonstrate how to:
 * 1. Generate AES symmetric key
 * 2. Encrypt given data using this key
 * 3. Decrypt given data using this key
 */

#include <iostream>
#include <cstring>

#include <telux/sec/SecurityFactory.hpp>
#include <telux/sec/CryptoManager.hpp>
#include <telux/sec/CryptoParamBuilder.hpp>

int generateAESKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr, std::vector<uint8_t> &kb) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    // Define parameters for the key
    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_AES)
             .setCryptoOperation(telux::sec::CryptoOperation::CRYPTO_OP_ENCRYPT
                                 | telux::sec::CryptoOperation::CRYPTO_OP_DECRYPT)
             .setKeySize(128)
             .setBlockMode(
                 telux::sec::BlockMode::BLOCK_MODE_CBC | telux::sec::BlockMode::BLOCK_MODE_CTR)
             .setPadding(telux::sec::Padding::PADDING_PKCS7 | telux::sec::Padding::PADDING_NONE)
             .setCallerNonce(true)
             .build();

    ec = cryptMgr->generateKey(cp, kb);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't generate AES sym key, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int encryptDataWithAESKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
    std::vector<uint8_t> kb, std::vector<uint8_t> pt,
    std::shared_ptr<telux::sec::EncryptedData> &ed) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    // Specify initialization vector
    std::vector<uint8_t> initVector((128 / 8), 0x01);

    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_AES)
             .setBlockMode(telux::sec::BlockMode::BLOCK_MODE_CBC)
             .setPadding(telux::sec::Padding::PADDING_PKCS7)
             .setInitVector(initVector)
             .build();

    ec = cryptMgr->encryptData(cp, kb, pt, ed);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't encrypt data, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int decryptDataWithAESKey(std::shared_ptr<telux::sec::ICryptoManager> cryptMgr,
    std::vector<uint8_t> kb, std::vector<uint8_t> et, std::vector<uint8_t> &dt) {

    std::shared_ptr<telux::sec::ICryptoParam> cp;
    telux::common::ErrorCode ec;

    // Specify initialization vector
    std::vector<uint8_t> initVector((128 / 8), 0x01);

    cp = telux::sec::CryptoParamBuilder()
             .setAlgorithm(telux::sec::Algorithm::ALGORITHM_AES)
             .setBlockMode(telux::sec::BlockMode::BLOCK_MODE_CBC)
             .setPadding(telux::sec::Padding::PADDING_PKCS7)
             .setInitVector(initVector)
             .build();

    ec = cryptMgr->decryptData(cp, kb, et, dt);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cout << "Can't decrypt data, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    return 0;
}

int main(int argc, char **argv) {

    int ret;
    telux::common::ErrorCode ec;
    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr;

    std::shared_ptr<telux::sec::EncryptedData> ed;
    std::vector<uint8_t> dt;
    std::vector<uint8_t> kb;

    // Specify data to be encrypted
    std::vector<uint8_t> pt{'h', 'e', 'l', 'l', 'o'};

    // Get SecurityFactory instance
    auto &secFact = telux::sec::SecurityFactory::getInstance();

    // Get CryptoManager instance
    cryptMgr = secFact.getCryptoManager(ec);
    if (!cryptMgr) {
        std::cout << "Can't allocate CryptoManager, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }

    // Generate AES asymmetric key
    ret = generateAESKey(cryptMgr, kb);
    if (ret) {
        return ret;
    }

    // Encrypt data
    ret = encryptDataWithAESKey(cryptMgr, kb, pt, ed);
    if (ret) {
        return ret;
    }

    // Decrypt data
    ret = decryptDataWithAESKey(cryptMgr, kb, ed->encryptedText, dt);
    if (ret) {
        return ret;
    }

    // Compare encrypted and decrypted data matches
    if (pt == dt) {
        std::cout << "Enc & Dec data matches!" << std::endl;
    } else {
        std::cout << "Enc & Dec data do not match!" << std::endl;
    }

    return ret;
}
