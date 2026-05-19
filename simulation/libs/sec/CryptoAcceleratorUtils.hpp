/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CRYPTOACCELERATORUTILS_HPP
#define CRYPTOACCELERATORUTILS_HPP

#include "internal-temp.h"

#include <telux/common/CommonDefines.hpp>

namespace telux {
namespace sec {

// Add this to CryptoAcceleratorUtils.hpp
inline uint32_t mapTeluxErrorToPke(telux::common::ErrorCode ec) {
    switch (ec) {
        case telux::common::ErrorCode::SUCCESS:
            return 0;  // MVM_ERROR_NONE
        case telux::common::ErrorCode::DMA_ERR:
            return 1;  // MVM_ERROR_HSDMA
        case telux::common::ErrorCode::UNKNOWN:
            return 2;  // MVM_ERROR_RESERVE1
        case telux::common::ErrorCode::DIV_ERR:
            return 4;  // MVM_ERROR_DIV
        case telux::common::ErrorCode::INVALID_LENGTH:
            return 5;  // MVM_ERROR_DATASIZE
        case telux::common::ErrorCode::RNG_UNSEEDED:
            return 6;  // MVM_ERROR_PRNG
        case telux::common::ErrorCode::MEM_ERR:
            return 7;  // MVM_ERROR_MEM_READ
        case telux::common::ErrorCode::MODULUS_ERR:
            return 8;  // MVM_ERROR_MODULUS
        case telux::common::ErrorCode::DECODING_ERR:
            return 9;  // MVM_ERROR_DECODE
        case telux::common::ErrorCode::VERIFICATION_FAILED:
            return 0;  // MVM_ERROR_NONE (not a PKE error)
        default:
            return 2;  // MVM_ERROR_RESERVE1 for unknown
    }
}

// Helper function to map PKE error codes to Telux error codes
inline telux::common::ErrorCode mapPkeErrorToTelux(uint32_t pkeError) {
    // These match the MVM_ERROR_STATUS enum values
    switch (pkeError) {
        case 0:  // MVM_ERROR_NONE
            return telux::common::ErrorCode::SUCCESS;
        case 1:  // MVM_ERROR_HSDMA
            return telux::common::ErrorCode::DMA_ERR;
        case 2:  // MVM_ERROR_RESERVE1
        case 3:  // MVM_ERROR_RESERVE2
            return telux::common::ErrorCode::UNKNOWN;
        case 4:  // MVM_ERROR_DIV
            return telux::common::ErrorCode::DIV_ERR;
        case 5:  // MVM_ERROR_DATASIZE
            return telux::common::ErrorCode::INVALID_LENGTH;
        case 6:  // MVM_ERROR_PRNG
            return telux::common::ErrorCode::RNG_UNSEEDED;
        case 7:  // MVM_ERROR_MEM_READ
            return telux::common::ErrorCode::MEM_ERR;
        case 8:  // MVM_ERROR_MODULUS
            return telux::common::ErrorCode::MODULUS_ERR;
        case 9:  // MVM_ERROR_DECODE
            return telux::common::ErrorCode::DECODING_ERR;
        default:
            return telux::common::ErrorCode::GENERIC_FAILURE;
    }
}

class CryptoAcceleratorUtils {

 public:
    /* Translates libmvm specific error code to telsdk specific error code */
    static telux::common::ErrorCode caToTeluxErrorCode(MVM_RETURN acceleratorErrorNum);

    /* Translates public key engine (PKE) specific error code to telsdk specific error code */
    static telux::common::ErrorCode pkeToTeluxErrorCode(MVM_ERROR_STATUS pkeErrorNum);

 private:
    CryptoAcceleratorUtils() {
    }
};

}  // End of namespace sec
}  // End of namespace telux

#endif  // CRYPTOACCELERATORUTILS_HPP
