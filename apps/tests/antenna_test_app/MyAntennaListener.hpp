/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef MYANTENNALISTENER_HPP
#define MYANTENNALISTENER_HPP

#include <telux/platform/hardware/AntennaListener.hpp>

using namespace telux::platform;
using namespace telux::common;

class MyAntennaListener : public telux::platform::hardware::IAntennaListener {
 public:
    void onServiceStatusChange(ServiceStatus status) override;
    void onActiveAntennaChange(int antIndex) override;

    ~MyAntennaListener(){};
};

#endif  // MYANTENNALISTENER_HPP
