/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef BACKHAULLISTENER_HPP
#define BACKHAULLISTENER_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/net/BackhaulManager.hpp>

class BackhaulListener : public telux::data::net::IBackhaulManagerListener {
public:
    BackhaulListener();
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onBackhaulStatusChange(telux::data::net::BackhaulStatusInfo 
                                         backhaulStatusInfo)    override;

};

#endif  // BACKHAULLISTENER_HPP
