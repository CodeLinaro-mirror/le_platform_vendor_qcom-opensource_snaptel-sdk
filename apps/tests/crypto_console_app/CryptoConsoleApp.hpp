/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOCONSOLEAPP_HPP
#define CRYPTOCONSOLEAPP_HPP

#include "common/console_app_framework/ConsoleApp.hpp"

#include "CommandProcessor.hpp"

class CryptoConsoleApp : public ConsoleApp {
 public:
    CryptoConsoleApp(std::string appName, std::string cursor);
    ~CryptoConsoleApp();

    void init(void);

    void generateKey(void);
    void signData(void);
    void verifySignature(void);
    void encryptData(void);
    void decryptData(void);
    void importKey(void);
    void exportKey(void);
    void upgradeKey(void);

 private:
    std::shared_ptr<CommandProcessor> cmdProcessor_;

    void getHexStringAsByteArrayFromUsr(
        const std::string choiceToDisplay, std::vector<uint8_t> &usrEntry, const uint32_t length);
    void getChoiceNumberFromUsr(const std::string choicesToDisplay, const uint32_t minVal,
        const uint32_t maxVal, uint32_t &selection, bool multipleOfEigth);
    void getMultipleChoiceNumberFromUsr(const std::string choicesToDisplay, const uint32_t minVal,
        const uint32_t maxVal, std::vector<uint32_t> &selection);
    void getFileFromUser(std::shared_ptr<std::string> &absoluteFilePath);
    void getAbsoluteFilePathFromUser(
        const std::string choicesToDisplay, std::shared_ptr<std::string> &absoluteFilePath);

    void getAlgorithmFromUser(telux::sec::Algorithm &algo, uint32_t restriction);
    void getDigestFromUser(telux::sec::DigestTypes &digest, uint32_t restriction);
    void getPaddingFromUser(telux::sec::PaddingTypes &padding, uint32_t restriction);
    void getBlockModeFromUser(telux::sec::BlockModeTypes &blockMode);
    void getCallerNoncePresentFromUser(bool &callerNoncePresent);
    void getKeySizeFromUser(uint32_t &keySize);
    void getPublicExponentFromUser(uint32_t &publicExponent);
    void getMinMacLengthFromUser(uint32_t &minMacLength, uint32_t maxVal);
    void getMacLengthFromUser(uint32_t &macLength, uint32_t maxVal);
    void getKeyFormatFromUser(telux::sec::KeyFormat &keyFmt);
    void getInitVectorFromUser(std::vector<uint8_t> &iv, uint32_t length);
    void getAssociatedDataFromUser(std::vector<uint8_t> &associatedData);
    void getUniqueDataFromUser(std::vector<uint8_t> &uniqueData);
    void getKeyDataFromUser(std::vector<uint8_t> &keyData);
    void getKeyBlobFromUser(std::vector<uint8_t> &keyBlob);
    void getPlainTextFromUser(std::vector<uint8_t> &plainText);
    void getSignatureFromUser(std::vector<uint8_t> &signature);
    void getEncryptedTextFromUser(std::vector<uint8_t> &encText);
    void getOperationFromUser(telux::sec::CryptoOperationTypes &operation, uint32_t restriction);
};

#endif  // CRYPTOCONSOLEAPP_HPP
