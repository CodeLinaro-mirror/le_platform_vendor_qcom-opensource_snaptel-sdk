/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef DATALINKLISTENER_HPP
#define DATALINKLISTENER_HPP

#include <telux/data/DataFactory.hpp>
#include <telux/data/DataLinkManager.hpp>

class DataLinkListener : public telux::data::IDataLinkListener {
public:
    DataLinkListener();
    ~DataLinkListener();
    void onServiceStatusChange(telux::common::ServiceStatus status) override;
    void onLinkStatusChange(const telux::data::LinkStatusInfo& info) override;
};

#endif  // DATALINKLISTENER_HPP
