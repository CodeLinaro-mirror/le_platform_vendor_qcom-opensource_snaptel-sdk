/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATASETTINGSLISTENER_HPP
#define DATASETTINGSLISTENER_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataSettingsManager.hpp>

class DataSettingsListener : public telux::data::IDataSettingsListener {
public:
    DataSettingsListener();
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
};

#endif  // DATASETTINGSLISTENER_HPP
