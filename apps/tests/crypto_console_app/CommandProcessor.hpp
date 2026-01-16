/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef COMMANDPROCESSOR_HPP
#define COMMANDPROCESSOR_HPP

#include <telux/sec/SecurityFactory.hpp>

struct Request {
    bool callerNoncePresent;
    uint32_t keySize;
    uint32_t macLength;
    uint32_t minMacLength;
    uint32_t publicExponent;
    telux::sec::Algorithm algo;
    telux::sec::KeyFormat keyFmt;
    telux::sec::DigestTypes digest;
    telux::sec::PaddingTypes padding;
    telux::sec::BlockModeTypes blockMode;
    telux::sec::CryptoOperationTypes operation;
    std::shared_ptr<telux::sec::EncryptedData> encData;
    std::vector<uint8_t> textA;
    std::vector<uint8_t> textB;
    std::vector<uint8_t> textC;
    std::vector<uint8_t> initVector;
    std::vector<uint8_t> uniqueData;
    std::vector<uint8_t> associatedData;
};

class CommandProcessor {
 public:
    int init(void);

    void generateKey(Request request, std::shared_ptr<std::string> keyBlobFile);

    void signData(Request request, std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> plainTxtFile, std::shared_ptr<std::string> signatureFile);

    void verifySignature(Request request, std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> plainTxtFile, std::shared_ptr<std::string> signatureFile);

    void encryptData(Request request, std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> plainTxtFile, std::shared_ptr<std::string> encTxtFile);

    void decryptData(Request request, std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> encTxtFile, std::shared_ptr<std::string> plainTxtFile);

    void importKey(Request request, std::shared_ptr<std::string> keyDataFile,
        std::shared_ptr<std::string> keyBlobFile);

    void exportKey(Request request, std::shared_ptr<std::string> keyBlobFile,
        std::shared_ptr<std::string> expDataFile);

    void upgradeKey(Request request, std::shared_ptr<std::string> keyBlobFileOld,
        std::shared_ptr<std::string> keyBlobFileNew);

 private:
    std::shared_ptr<telux::sec::ICryptoManager> cryptMgr_;

    void byteArrayToHexString(std::vector<uint8_t> data);

    int saveOnFileSystem(
        std::vector<uint8_t> const &data, std::shared_ptr<std::string> absoluteFilePath);

    int readFileIntoVector(
        std::vector<uint8_t> &data, std::shared_ptr<std::string> absoluteFilePath);
};

#endif  // COMMANDPROCESSOR_HPP
