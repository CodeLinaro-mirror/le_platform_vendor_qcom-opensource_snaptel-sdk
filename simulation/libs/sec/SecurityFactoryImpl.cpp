/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "common/Logger.hpp"
#include "common/CommonUtils.hpp"

#include "SecurityFactoryImpl.hpp"

namespace telux {
namespace sec {

SecurityFactoryImpl::SecurityFactoryImpl() {
}

SecurityFactoryImpl::~SecurityFactoryImpl() {
    LOG(DEBUG, __FUNCTION__);
}

SecurityFactory::SecurityFactory() {
}

SecurityFactory::~SecurityFactory() {
}

SecurityFactory &SecurityFactoryImpl::getInstance() {
    static SecurityFactoryImpl instance;
    return instance;
}

SecurityFactory &SecurityFactory::getInstance() {
    return SecurityFactoryImpl::getInstance();
}

/**
 * Gets ICryptoManager instance through which key management and
 * cryptographic operations can be performed.
 *
 * @return Shared pointer to the CryptoManagerImpl object.
 */
std::shared_ptr<ICryptoManager> SecurityFactoryImpl::getCryptoManager(
    telux::common::ErrorCode &ec) {

    std::shared_ptr<CryptoManagerImpl> cryptMgr;
    return cryptMgr;
}

/**
 * Gets a CryptoAcceleratorManager instance that can be used to perform
 * cryptographic operations requiring elliptic-curve cryptography (ECC)
 * verifications and calculations.
 *
 * @return Shared pointer to the CryptoAcceleratorManagerImpl object.
 */
std::shared_ptr<ICryptoAcceleratorManager> SecurityFactoryImpl::getCryptoAcceleratorManager(
    telux::common::ErrorCode &ec, Mode mode, std::weak_ptr<ICryptoAcceleratorListener> caListener) {

    std::shared_ptr<CryptoAcceleratorManagerImpl> cryptAccelMgr;
    return cryptAccelMgr;
}

/**
 * Gets an ICAControlManager instance.
 *
 * @return Shared pointer to the CAControlManagerImpl object.
 */
std::shared_ptr<ICAControlManager> SecurityFactoryImpl::getCAControlManager(
    telux::common::ErrorCode &ec) {

    std::shared_ptr<CAControlManagerImpl> caCtrlMgr;

    std::lock_guard<std::mutex> lock(secFactoryGuard_);

    caCtrlMgr = caCtrlMgr_.lock();
    if (caCtrlMgr) {
        ec = telux::common::ErrorCode::SUCCESS;
        return caCtrlMgr;
    }

    try {
        caCtrlMgr = std::make_shared<CAControlManagerImpl>();
    } catch (const std::exception &e) {
        ec = telux::common::ErrorCode::NO_MEMORY;
        LOG(ERROR, __FUNCTION__, " can't create CAControlManagerImpl");
        return nullptr;
    }

    ec = caCtrlMgr->init();
    if (ec != telux::common::ErrorCode::SUCCESS) {
        return nullptr;
    }

    /* Save reference locally */
    caCtrlMgr_ = caCtrlMgr;

    return caCtrlMgr;
}

/**
 * Gets IRandomNumberManager instance that can be used to generate random numbers.
 *
 * @return Shared pointer to the RandomNumberManagerImpl object.
 */
std::shared_ptr<IRandomNumberManager> SecurityFactoryImpl::getRandomNumberManager(
    RNGSource generatorSource, telux::common::ErrorCode &ec) {

    std::shared_ptr<RandomNumberManagerImpl> rngMgr;

    std::lock_guard<std::mutex> lock(secFactoryGuard_);

    try {
        rngMgr = std::make_shared<RandomNumberManagerImpl>();
    } catch (const std::exception &e) {
        ec = telux::common::ErrorCode::NO_MEMORY;
        LOG(ERROR, __FUNCTION__, " can't create RandomNumberManagerImpl");
        return nullptr;
    }

    ec = rngMgr->init(generatorSource);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        return nullptr;
    }

    return rngMgr;
}

}  // end namespace sec
}  // end namespace telux
