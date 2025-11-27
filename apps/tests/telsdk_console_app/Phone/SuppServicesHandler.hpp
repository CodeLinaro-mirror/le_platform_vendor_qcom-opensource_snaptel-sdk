/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file      SuppServicesHandler.hpp
 *
 * @brief     File contains helper class and response callback classed required to handle
 *            the supplementary services response callbacks.
 */

#ifndef TELUX_TEL_SUPP_SERVICES_HANDLER_HPP
#define TELUX_TEL_SUPP_SERVICES_HANDLER_HPP

#include <memory>
#include <string>
#include <vector>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/SuppServicesManager.hpp>

class SuppServicesHelper {
 public:
    static std::string suppServicesStatustoString(telux::tel::SuppServicesStatus svcStatus);
    static std::string SuppSvcProvisionStatustoString(
        telux::tel::SuppSvcProvisionStatus provisionStatus);
};

class SetSuppSvcResponseCallback {
 public:
    static void setSuppSvcResp(
        telux::common::ErrorCode error, telux::tel::FailureCause failureCause);
};

class GetSuppSvcResponseCallback {
 public:
    static void getCallWaitingPrefResp(telux::tel::SuppServicesStatus suppSvcStatus,
        telux::tel::FailureCause failureCause, telux::common::ErrorCode error);

    static void getForwardingPrefResp(std::vector<telux::tel::ForwardInfo> forwardInfoList,
        telux::tel::FailureCause failureCause, telux::common::ErrorCode error);

    static void getOirStatusResp(telux::tel::SuppServicesStatus activeStatus,
        telux::tel::SuppSvcProvisionStatus provisionStatus, telux::tel::FailureCause failureCause,
        telux::common::ErrorCode error);
};

class MySuppServicesListener : public telux::tel::ISuppServicesListener {
 public:
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
};
#endif  // TELUX_TEL_SUPP_SERVICES_HANDLER_HPP
