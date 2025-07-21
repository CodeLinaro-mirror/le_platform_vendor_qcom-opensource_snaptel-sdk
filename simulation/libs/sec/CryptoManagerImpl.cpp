/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <memory>
#include <vector>
#include <iostream>
#include <algorithm>

#include <telux/sec/CryptoDefines.hpp>

#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

#include "CryptoManagerImpl.hpp"

namespace telux {
namespace sec {

/* OS version, OS security patch, ROT must be same as qseecomd */
CryptoManagerImpl::CryptoManagerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

CryptoManagerImpl::~CryptoManagerImpl() {
    LOG(DEBUG, __FUNCTION__);
}

void CryptoManagerImpl::addPurpose(
    std::shared_ptr<CryptoParamImpl> cryptoParamImpl, std::vector<keymaster_key_param_t> &kp) {
}

void CryptoManagerImpl::addDigest(
    std::shared_ptr<CryptoParamImpl> cryptoParamImpl, std::vector<keymaster_key_param_t> &kp) {
}

void CryptoManagerImpl::addBlockMode(
    std::shared_ptr<CryptoParamImpl> cryptoParamImpl, std::vector<keymaster_key_param_t> &kp) {
}

void CryptoManagerImpl::addPadding(
    std::shared_ptr<CryptoParamImpl> cryptoParamImpl, std::vector<keymaster_key_param_t> &kp) {
}

void CryptoManagerImpl::addCurve(
    std::shared_ptr<CryptoParamImpl> cryptoParamImpl, std::vector<keymaster_key_param_t> &kp) {
}

int32_t CryptoManagerImpl::getKeyFmt(telux::sec::KeyFormat keyFmt, keymaster_key_format_t *fmt) {
    return 0;
}

telux::common::ErrorCode CryptoManagerImpl::deinit() {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::init() {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::generateKey(
    std::shared_ptr<ICryptoParam> cryptoParam, std::vector<uint8_t> &keyBlob) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::importKey(std::shared_ptr<ICryptoParam> cryptoParam,
    telux::sec::KeyFormat keyFmt, std::vector<uint8_t> const &keyData,
    std::vector<uint8_t> &keyBlob) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::exportKey(telux::sec::KeyFormat keyFmt,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> &keyData) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::upgradeKey(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &oldKeyBlob, std::vector<uint8_t> &newKeyBlob) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::signData(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
    std::vector<uint8_t> &signature) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::verifyData(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
    std::vector<uint8_t> const &signature) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::encryptData(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &plainText,
    std::shared_ptr<EncryptedData> &encryptedData) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

telux::common::ErrorCode CryptoManagerImpl::decryptData(std::shared_ptr<ICryptoParam> cryptoParam,
    std::vector<uint8_t> const &keyBlob, std::vector<uint8_t> const &encryptedText,
    std::vector<uint8_t> &decryptedText) {

    return telux::common::ErrorCode::NOT_SUPPORTED;
}

}  // End of namespace sec
}  // End of namespace telux
