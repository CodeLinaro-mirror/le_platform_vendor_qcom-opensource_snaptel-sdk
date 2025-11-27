/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef CLIENTLISTENER_HPP
#define CLIENTLISTENER_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/ClientManager.hpp>

class ClientListener : public telux::data::IClientListener {
 public:
    ClientListener();
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onDeviceDataUsageResetImminent(
        const std::vector<telux::data::DeviceDataUsage> devicesDataUsage,
        telux::data::UsageResetReason reason) override;
};

#endif  // CLIENTLISTENER_HPP
