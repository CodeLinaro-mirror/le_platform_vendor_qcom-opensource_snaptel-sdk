/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <jsoncpp/json/json.h>
#include "CommonUtils.hpp"

telux::common::Status CommonUtils::mapStatus(std::string status) {
    if (status == "SUCCESS") {
        return telux::common::Status::SUCCESS;
    } else if (status == "FAILED") {
        return telux::common::Status::FAILED;
    } else if (status == "NOCONNECTION") {
        return telux::common::Status::NOCONNECTION;
    } else if (status == "NOSUBSCRIPTION") {
        return telux::common::Status::NOSUBSCRIPTION;
    } else if (status == "INVALIDPARAM") {
        return telux::common::Status::INVALIDPARAM;
    } else if (status == "INVALIDSTATE") {
        return telux::common::Status::INVALIDSTATE;
    } else if (status == "NOTREADY") {
        return telux::common::Status::NOTREADY;
    } else if (status == "NOTALLOWED") {
        return telux::common::Status::NOTALLOWED;
    } else if (status == "NOTIMPLEMENTED") {
        return telux::common::Status::NOTIMPLEMENTED;
    } else if (status == "CONNECTIONLOST") {
        return telux::common::Status::CONNECTIONLOST;
    } else if (status == "EXPIRED") {
        return telux::common::Status::EXPIRED;
    } else if (status == "ALREADY") {
        return telux::common::Status::ALREADY;
    } else if (status == "NOSUCH") {
        return telux::common::Status::NOSUCH;
    } else if (status == "NOTSUPPORTED") {
        return telux::common::Status::NOTSUPPORTED;
    } else if (status == "NOMEMORY") {
        return telux::common::Status::NOMEMORY;
    }

    return telux::common::Status::FAILED;
}

telux::common::ErrorCode CommonUtils::mapErrorCode(std::string errorCode) {
    if (errorCode == "SUCCESS") {
        return telux::common::ErrorCode::SUCCESS;
    } else if (errorCode == "RADIO_NOT_AVAILABLE") {
        return telux::common::ErrorCode::RADIO_NOT_AVAILABLE;
    } else if (errorCode == "GENERIC_FAILURE") {
        return telux::common::ErrorCode::GENERIC_FAILURE;
    } else if (errorCode == "PASSWORD_INCORRECT") {
        return telux::common::ErrorCode::PASSWORD_INCORRECT;
    } else if (errorCode == "SIM_PIN2") {
        return telux::common::ErrorCode::SIM_PIN2;
    } else if (errorCode == "SIM_PUK2") {
        return telux::common::ErrorCode::SIM_PUK2;
    } else if (errorCode == "REQUEST_NOT_SUPPORTED") {
        return telux::common::ErrorCode::REQUEST_NOT_SUPPORTED;
    } else if (errorCode == "CANCELLED") {
        return telux::common::ErrorCode::CANCELLED;
    } else if (errorCode == "OP_NOT_ALLOWED_DURING_VOICE_CALL") {
        return telux::common::ErrorCode::OP_NOT_ALLOWED_DURING_VOICE_CALL;
    } else if (errorCode == "OP_NOT_ALLOWED_BEFORE_REG_TO_NW") {
        return telux::common::ErrorCode::OP_NOT_ALLOWED_BEFORE_REG_TO_NW;
    } else if (errorCode == "SMS_SEND_FAIL_RETRY") {
        return telux::common::ErrorCode::SMS_SEND_FAIL_RETRY;
    } else if (errorCode == "SIM_ABSENT") {
        return telux::common::ErrorCode::SIM_ABSENT;
    } else if (errorCode == "SUBSCRIPTION_NOT_AVAILABLE") {
        return telux::common::ErrorCode::SUBSCRIPTION_NOT_AVAILABLE;
    } else if (errorCode == "MODE_NOT_SUPPORTED") {
        return telux::common::ErrorCode::MODE_NOT_SUPPORTED;
    } else if (errorCode == "FDN_CHECK_FAILURE") {
        return telux::common::ErrorCode::FDN_CHECK_FAILURE;
    } else if (errorCode == "ILLEGAL_SIM_OR_ME") {
        return telux::common::ErrorCode::ILLEGAL_SIM_OR_ME;
    } else if (errorCode == "MISSING_RESOURCE") {
        return telux::common::ErrorCode::MISSING_RESOURCE;
    } else if (errorCode == "NO_SUCH_ELEMENT") {
        return telux::common::ErrorCode::NO_SUCH_ELEMENT;
    } else if (errorCode == "DIAL_MODIFIED_TO_USSD") {
        return telux::common::ErrorCode::DIAL_MODIFIED_TO_USSD;
    } else if (errorCode == "DIAL_MODIFIED_TO_SS") {
        return telux::common::ErrorCode::DIAL_MODIFIED_TO_SS;
    } else if (errorCode == "DIAL_MODIFIED_TO_DIAL") {
        return telux::common::ErrorCode::DIAL_MODIFIED_TO_DIAL;
    } else if (errorCode == "USSD_MODIFIED_TO_DIAL") {
        return telux::common::ErrorCode::USSD_MODIFIED_TO_DIAL;
    } else if (errorCode == "USSD_MODIFIED_TO_SS") {
        return telux::common::ErrorCode::USSD_MODIFIED_TO_SS;
    } else if (errorCode == "USSD_MODIFIED_TO_USSD") {
        return telux::common::ErrorCode::USSD_MODIFIED_TO_USSD;
    } else if (errorCode == "SS_MODIFIED_TO_DIAL") {
        return telux::common::ErrorCode::SS_MODIFIED_TO_DIAL;
    } else if (errorCode == "SS_MODIFIED_TO_USSD") {
        return telux::common::ErrorCode::SS_MODIFIED_TO_USSD;
    } else if (errorCode == "SUBSCRIPTION_NOT_SUPPORTED") {
        return telux::common::ErrorCode::SUBSCRIPTION_NOT_SUPPORTED;
    } else if (errorCode == "SS_MODIFIED_TO_SS") {
        return telux::common::ErrorCode::SS_MODIFIED_TO_SS;
    } else if (errorCode == "LCE_NOT_SUPPORTED") {
        return telux::common::ErrorCode::LCE_NOT_SUPPORTED;
    } else if (errorCode == "NO_MEMORY") {
        return telux::common::ErrorCode::NO_MEMORY;
    } else if (errorCode == "INTERNAL_ERR") {
        return telux::common::ErrorCode::INTERNAL_ERR;
    } else if (errorCode == "SYSTEM_ERR") {
        return telux::common::ErrorCode::SYSTEM_ERR;
    } else if (errorCode == "MODEM_ERR") {
        return telux::common::ErrorCode::MODEM_ERR;
    } else if (errorCode == "INVALID_STATE") {
        return telux::common::ErrorCode::INVALID_STATE;
    } else if (errorCode == "NO_RESOURCES") {
        return telux::common::ErrorCode::NO_RESOURCES;
    } else if (errorCode == "SIM_ERR") {
        return telux::common::ErrorCode::SIM_ERR;
    } else if (errorCode == "INVALID_ARGUMENTS") {
        return telux::common::ErrorCode::INVALID_ARGUMENTS;
    } else if (errorCode == "INVALID_SIM_STATE") {
        return telux::common::ErrorCode::INVALID_SIM_STATE;
    } else if (errorCode == "INVALID_MODEM_STATE") {
        return telux::common::ErrorCode::INVALID_MODEM_STATE;
    } else if (errorCode == "INVALID_CALL_ID") {
        return telux::common::ErrorCode::INVALID_CALL_ID;
    } else if (errorCode == "NO_SMS_TO_ACK") {
        return telux::common::ErrorCode::NO_SMS_TO_ACK;
    } else if (errorCode == "NETWORK_ERR") {
        return telux::common::ErrorCode::NETWORK_ERR;
    } else if (errorCode == "REQUEST_RATE_LIMITED") {
        return telux::common::ErrorCode::REQUEST_RATE_LIMITED;
    } else if (errorCode == "SIM_BUSY") {
        return telux::common::ErrorCode::SIM_BUSY;
    } else if (errorCode == "SIM_FULL") {
        return telux::common::ErrorCode::SIM_FULL;
    } else if (errorCode == "NETWORK_REJECT") {
        return telux::common::ErrorCode::NETWORK_REJECT;
    } else if (errorCode == "OPERATION_NOT_ALLOWED") {
        return telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
    } else if (errorCode == "EMPTY_RECORD") {
        return telux::common::ErrorCode::EMPTY_RECORD;
    } else if (errorCode == "INVALID_SMS_FORMAT") {
        return telux::common::ErrorCode::INVALID_SMS_FORMAT;
    } else if (errorCode == "ENCODING_ERR") {
        return telux::common::ErrorCode::ENCODING_ERR;
    } else if (errorCode == "INVALID_SMSC_ADDRESS") {
        return telux::common::ErrorCode::INVALID_SMSC_ADDRESS;
    } else if (errorCode == "NO_SUCH_ENTRY") {
        return telux::common::ErrorCode::NO_SUCH_ENTRY;
    } else if (errorCode == "NETWORK_NOT_READY") {
        return telux::common::ErrorCode::NETWORK_NOT_READY;
    } else if (errorCode == "NOT_PROVISIONED") {
        return telux::common::ErrorCode::NOT_PROVISIONED;
    } else if (errorCode == "NO_SUBSCRIPTION") {
        return telux::common::ErrorCode::NO_SUBSCRIPTION;
    } else if (errorCode == "NO_NETWORK_FOUND") {
        return telux::common::ErrorCode::NO_NETWORK_FOUND;
    } else if (errorCode == "DEVICE_IN_USE") {
        return telux::common::ErrorCode::DEVICE_IN_USE;
    } else if (errorCode == "ABORTED") {
        return telux::common::ErrorCode::ABORTED;
    } else if (errorCode == "INCOMPATIBLE_STATE") {
        return telux::common::ErrorCode::INCOMPATIBLE_STATE;
    } else if (errorCode == "NO_EFFECT") {
        return telux::common::ErrorCode::NO_EFFECT;
    } else if (errorCode == "DEVICE_NOT_READY") {
        return telux::common::ErrorCode::DEVICE_NOT_READY;
    } else if (errorCode == "MISSING_ARGUMENTS") {
        return telux::common::ErrorCode::MISSING_ARGUMENTS;
    } else if (errorCode == "PIN_PERM_BLOCKED") {
        return telux::common::ErrorCode::PIN_PERM_BLOCKED;
    } else if (errorCode == "PIN_BLOCKED") {
        return telux::common::ErrorCode::PIN_BLOCKED;
    } else if (errorCode == "MALFORMED_MSG") {
        return telux::common::ErrorCode::MALFORMED_MSG;
    } else if (errorCode == "INTERNAL") {
        return telux::common::ErrorCode::INTERNAL;
    } else if (errorCode == "CLIENT_IDS_EXHAUSTED") {
        return telux::common::ErrorCode::CLIENT_IDS_EXHAUSTED;
    }

    return telux::common::ErrorCode::INTERNAL_ERR;
}

telux::common::ErrorCode CommonUtils::toErrorCode(telux::common::Status status) {

    switch (status) {
        case telux::common::Status::SUCCESS:
            return telux::common::ErrorCode::SUCCESS;
        case telux::common::Status::FAILED:
            return telux::common::ErrorCode::GENERIC_FAILURE;
        case telux::common::Status::NOCONNECTION:
        case telux::common::Status::INVALIDSTATE:
        case telux::common::Status::NOTREADY:
        case telux::common::Status::CONNECTIONLOST:
            return telux::common::ErrorCode::INVALID_STATE;
        case telux::common::Status::NOSUBSCRIPTION:
            return telux::common::ErrorCode::NO_SUBSCRIPTION;
        case telux::common::Status::INVALIDPARAM:
        case telux::common::Status::ALREADY:
            return telux::common::ErrorCode::INVALID_ARGUMENTS;
        case telux::common::Status::NOTALLOWED:
            return telux::common::ErrorCode::OPERATION_NOT_ALLOWED;
        case telux::common::Status::NOTIMPLEMENTED:
        case telux::common::Status::NOTSUPPORTED:
            return telux::common::ErrorCode::NOT_SUPPORTED;
        case telux::common::Status::EXPIRED:
        case telux::common::Status::NOSUCH:
            return telux::common::ErrorCode::NO_SUCH_ENTRY;
        case telux::common::Status::NOMEMORY:
            return telux::common::ErrorCode::NO_MEMORY;
        default:
            return telux::common::ErrorCode::GENERIC_FAILURE;
    }
}

void CommonUtils::getValues(Json::Value &values, std::string subsystem,
    std::string method, telux::common::Status &status,
    telux::common::ErrorCode &errorCode, uint32_t &cbDelay) {
        std::string statusStr = values[subsystem][method]["status"].asString();
        status = mapStatus(statusStr);
        std::string errorStr = values[subsystem][method]["error"].asString();
        errorCode = mapErrorCode(errorStr);
        cbDelay = values[subsystem][method]["callbackDelay"].asInt();
}