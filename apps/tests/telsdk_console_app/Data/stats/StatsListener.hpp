/*
 *  Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef STATSLISTENER_HPP
#define STATSLISTENER_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/StatsManager.hpp>

class StatsListener : public telux::data::IStatsListener {
public:
    StatsListener();
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    virtual void onClientDataUsageResetImminent(
        const std::vector<telux::data::ClientDataUsage> devicesDataUsage,
        telux::data::UsageResetReason reason) override {};

};

#endif  // STATSLISTENER_HPP