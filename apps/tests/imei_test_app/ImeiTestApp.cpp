/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: ImeiTestApp.cpp
 *
 * @brief: Simple application that queries IMEI.
 */

#include <future>
#include <string>
#include <vector>
#include <condition_variable>

#include "../../common/utils/Utils.hpp"
#include <telux/platform/PlatformFactory.hpp>

using std::cout;
using std::endl;
using std::string;
using namespace telux::common;
using namespace telux::platform;

int main(int argc, char *argv[]) {
    cout << "Running IMEI test app" << endl;
    auto &platformFactory = PlatformFactory::getInstance();

    std::promise<ServiceStatus> p;
    auto initCb = [&p](ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
    };
    std::shared_ptr<IDeviceInfoManager> deviceInfoManager
        = platformFactory.getDeviceInfoManager(initCb);
    if (deviceInfoManager == nullptr) {
        std::cout << "DeviceInfo manager is nullptr" << std::endl;
        exit(1);
    }
    std::cout << "Obtained deviceInfo manager" << std::endl;
    p.get_future().get();
    if (deviceInfoManager->getServiceStatus() != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "DeviceInfo service not available" << std::endl;
        exit(1);
    }
    std::string imei = "";
    if (Status::SUCCESS == deviceInfoManager->getIMEI(imei)) {
        cout << "Request IMEI successfully: " << imei << endl;
    } else {
        cout << "Error : request for IMEI failed." << endl;
    }
    return 0;
}
