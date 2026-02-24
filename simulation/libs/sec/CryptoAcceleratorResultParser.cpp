/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "internal-temp.h"

#include "CryptoAcceleratorUtils.hpp"
#include "common/Logger.hpp"

#include <telux/sec/CryptoAcceleratorManager.hpp>

namespace telux {
namespace sec {

/*
 * Gives unique ID.
 */
uint32_t ResultParser::getId(const OperationResult &result) {

    return (result.id & 0xFFFU);
}

/*
 * Gives type of operation; ECC verification or ECQV point multiplication.
 */
OperationType ResultParser::getOperationType(const OperationResult &result) {
    if ((result.operationType & 0x7) == 0) {  // Assuming 0 = VERIFY
        return OperationType::OP_TYPE_VERIFY;
    }
    return OperationType::OP_TYPE_CALCULATE;
}

/*
 * Indicates; verification/calculation succeeded or failed.
 */
telux::common::ErrorCode ResultParser::getErrorCode(const OperationResult &result) {
    if (result.result > 0xFU) {
        LOG(ERROR, " result field has invalid bits set: ", result.result,
            " - possible data corruption");
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }
    if (result.errCode > 0x1FFU) {
        LOG(ERROR, " errCode field has invalid bits set: ", result.errCode,
            " - possible data corruption");
        return telux::common::ErrorCode::GENERIC_FAILURE;
    }

    // Parse the result field (bits 0-3) and errCode field (bits 0-8)
    uint32_t mainResult = result.result & 0xFU;
    uint32_t subErrCode = result.errCode & 0x1FFU;

    if (mainResult == 0) {  // MVM_RESULT_SUCCESS
        return telux::common::ErrorCode::SUCCESS;
    }

    // mainResult == 1 (MVM_RESULT_FAILED)
    if (subErrCode == 0) {  // MVM_ERROR_NONE
        // Verification completed but signature was invalid
        return telux::common::ErrorCode::VERIFICATION_FAILED;
    }

    // PKE error occurred
    return mapPkeErrorToTelux(subErrCode);
}

/*
 * Gives further insight about failure cause. Specifically hardware PKE errors.
 */
telux::common::ErrorCode ResultParser::getCAErrorCode(const OperationResult &result) {
    uint32_t subErrCode = result.errCode & 0x1FFU;
    return mapPkeErrorToTelux(subErrCode);
}

/*
 * Gives raw result buffer obtained from accelerator as is.
 */
uint8_t *ResultParser::getData(OperationResult &result) {

    return result.data;
}

}  // End of namespace sec
}  // End of namespace telux
