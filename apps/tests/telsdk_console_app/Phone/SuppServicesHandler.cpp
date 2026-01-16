/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include <iostream>

#include "SuppServicesHandler.hpp"
#include "Utils.hpp"

#define PRINT_CB std::cout << "\033[1;35mCallback: \033[0m"
#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::tel;
using namespace telux::common;

std::string SuppServicesHelper::suppServicesStatustoString(
    telux::tel::SuppServicesStatus suppSvcStatus) {
    if (suppSvcStatus == SuppServicesStatus::ENABLED) {
        return "ENABLED";
    } else if (suppSvcStatus == SuppServicesStatus::DISABLED) {
        return "DISABLED";
    } else {
        return "UNKNOWN";
    }
}

std::string SuppServicesHelper::SuppSvcProvisionStatustoString(
    telux::tel::SuppSvcProvisionStatus provisionStatus) {
    std::string status;
    switch (provisionStatus) {
        case SuppSvcProvisionStatus::PROVISIONED:
            status = "PROVISIONED";
            break;
        case SuppSvcProvisionStatus::NOT_PROVISIONED:
            status = "NOT_PROVISIONED";
            break;
        case SuppSvcProvisionStatus::PRESENTATION_RESTRICTED:
            status = "PRESENTATION_RESTRICTED";
            break;
        case SuppSvcProvisionStatus::PRESENTATION_ALLOWED:
            status = "PRESENTATION_ALLOWED";
            break;
        default:
            status = "UNKNOWN";
            break;
    }
    return status;
}

void SetSuppSvcResponseCallback::setSuppSvcResp(ErrorCode error, FailureCause failureCause) {
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << " Set Supplementary Service :" << Utils::getErrorCodeAsString(error)
                 << std::endl;
    } else {
        PRINT_CB << "Set Supplementary Service Failed with, ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error) << std::endl;
    }
}

void GetSuppSvcResponseCallback::getCallWaitingPrefResp(
    SuppServicesStatus suppSvcStatus, FailureCause failureCause, telux::common::ErrorCode error) {
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Get Call Waiting Pref : " << Utils::getErrorCodeAsString(error) << std::endl;
        PRINT_CB << "Call Waiting Status : "
                 << SuppServicesHelper::suppServicesStatustoString(suppSvcStatus) << std::endl;
    } else {
        PRINT_CB << " get Call waiting pref failed with ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error)
                 << " Failure Cause : " << static_cast<int>(failureCause) << std::endl;
    }
}

void GetSuppSvcResponseCallback::getForwardingPrefResp(std::vector<ForwardInfo> forwardInfoList,
    FailureCause failureCause, telux::common::ErrorCode error) {
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB << "Get Forwarding pref : " << Utils::getErrorCodeAsString(error) << std::endl;
        for (auto &forwardInfo : forwardInfoList) {
            PRINT_CB << SuppServicesHelper::suppServicesStatustoString(forwardInfo.status)
                     << std::endl;
            PRINT_CB << "Number to which forwarded : " << forwardInfo.number << std::endl;
        }
    } else {
        PRINT_CB << "get forwarding pref failed with ErrorCode: " << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error)
                 << " Failure Cause : " << static_cast<int>(failureCause) << std::endl;
    }
}

void GetSuppSvcResponseCallback::getOirStatusResp(SuppServicesStatus suppSvcStatus,
    SuppSvcProvisionStatus provisionStatus, FailureCause failureCause,
    telux::common::ErrorCode error) {
    if (error == telux::common::ErrorCode::SUCCESS) {
        PRINT_CB
            << "Get Call Identification Restriction Pref : " << Utils::getErrorCodeAsString(error)
            << std::endl;
        PRINT_CB << "Call Identification Restriction Provision Status : "
                 << SuppServicesHelper::SuppSvcProvisionStatustoString(provisionStatus)
                 << std::endl;
        PRINT_CB << "Call Identification Restriction Status : "
                 << SuppServicesHelper::suppServicesStatustoString(suppSvcStatus) << std::endl;
    } else {
        PRINT_CB << "Get Call Identification Restriction failed with ErrorCode: "
                 << static_cast<int>(error)
                 << ", description: " << Utils::getErrorCodeAsString(error)
                 << " Failure Cause : " << static_cast<int>(failureCause) << std::endl;
    }
}

// Notify SuppServicesManager subsystem status
void MySuppServicesListener::onServiceStatusChange(telux::common::ServiceStatus status) {
    std::string stat = "";
    switch (status) {
        case telux::common::ServiceStatus::SERVICE_AVAILABLE:
            stat = " SERVICE_AVAILABLE";
            break;
        case telux::common::ServiceStatus::SERVICE_UNAVAILABLE:
            stat = " SERVICE_UNAVAILABLE";
            break;
        default:
            stat = " Unknown service status";
            break;
    }
    PRINT_NOTIFICATION << " SuppServices onServiceStatusChange" << stat << "\n";
}
