/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: qUtils.cpp
 *
 * @brief: Implementation of Utilities for qApplication.
 *
 */

#include "qUtils.hpp"
#include <cstring>

QUtils::QUtils() {
    telux::common::ErrorCode ec;
    /* Get CryptoManager instance with TRNG as source */
    rngMgr_ = telux::sec::SecurityFactory::getInstance().getRandomNumberManager(
        telux::sec::RNGSource::QTI_HW_TRNG, ec);
    if (!rngMgr_) {
        std::cerr << "Can't allocate IRandomNumberManager, err: " << static_cast<int>(ec)
                  << std::endl;
    }
}

void QUtils::initDiagLog() {
    v2x_diag_log_init();
}

void QUtils::deInitDiagLog() {
    v2x_diag_log_deinit();
}

int QUtils::hwTRNGInt(uint32_t &randomNumber) {
    telux::common::ErrorCode ec;
    if (rngMgr_ == nullptr) {
        /* Get CryptoManager instance with TRNG as source */
        rngMgr_ = telux::sec::SecurityFactory::getInstance().getRandomNumberManager(
            telux::sec::RNGSource::QTI_HW_TRNG, ec);
        if (!rngMgr_) {
            std::cerr << "Can't allocate IRandomNumberManager, err: " << static_cast<int>(ec)
                      << std::endl;
            return -1;
        }
    }
    /* Generate 32 bit random number */
    ec = rngMgr_->getRandomNumber(randomNumber);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cerr << "failed 32 bit number generation, err: " << static_cast<int>(ec) << std::endl;
        return -1;
    }
    return 0;
}

int QUtils::hwTRNGChar(uint8_t &randomNumber) {
    telux::common::ErrorCode ec;
    std::vector<uint8_t> generatedData(1, 0);
    size_t numBytes = 0;
    if (rngMgr_ == nullptr) {
        /* Get CryptoManager instance with TRNG as source */
        rngMgr_ = telux::sec::SecurityFactory::getInstance().getRandomNumberManager(
            telux::sec::RNGSource::QTI_HW_TRNG, ec);
        if (!rngMgr_) {
            std::cerr << "Can't allocate IRandomNumberManager, err: " << static_cast<int>(ec)
                      << std::endl;
            return -1;
        }
    }
    ec = rngMgr_->getRandomData(generatedData, numBytes);
    if (ec != telux::common::ErrorCode::SUCCESS) {
        std::cerr << "failed random uint8_t data generation, err: " << static_cast<int>(ec)
                  << std::endl;
        return -1;
    }
    if (numBytes == 1) {
        randomNumber = generatedData[0];
    } else {
        std::cerr << "failed random uint8_t data generation, err: " << static_cast<int>(ec)
                  << std::endl;
        return -1;
    }
    return 0;
}

void QUtils::fillVersion(uint32_t *version) {
    if (version != NULL) {
        *version = V2X_QITS_LOG_VERSION;
    }
}
