/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "WlanFactoryStub.hpp"
#include "common/CommonUtils.hpp"
#include "common/Logger.hpp"

#include "ApInterfaceManagerStub.hpp"
#include "StaInterfaceManagerStub.hpp"
#include "WlanDeviceManagerStub.hpp"

namespace telux {
namespace wlan {

WlanFactoryStub::WlanFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
}

WlanFactoryStub::~WlanFactoryStub() {
    LOG(DEBUG, __FUNCTION__);
}

WlanFactory::WlanFactory() {
    LOG(DEBUG, __FUNCTION__);
}

WlanFactory::~WlanFactory() {
    LOG(DEBUG, __FUNCTION__);
}

WlanFactory &WlanFactoryStub::getInstance() {
    static WlanFactoryStub instance;
    return instance;
}

// Forwarding call from the base class to the stub implementation.
WlanFactory &WlanFactory::getInstance() {
    return WlanFactoryStub::getInstance();
}

std::shared_ptr<IWlanDeviceManager> WlanFactoryStub::getWlanDeviceManager(
    telux::common::InitResponseCb clientCallback) {
    std::function<std::shared_ptr<telux::wlan::IWlanDeviceManager>(telux::common::InitResponseCb)>
        createAndInit = [this](telux::common::InitResponseCb initCb)
        -> std::shared_ptr<telux::wlan::IWlanDeviceManager> {
        std::shared_ptr<telux::wlan::WlanDeviceManagerStub> manager
            = std::make_shared<telux::wlan::WlanDeviceManagerStub>();
        if (manager && telux::common::Status::SUCCESS != manager->init(initCb)) {
            LOG(ERROR, __FUNCTION__, " WLAN Factory unable to initialize Wlan Device Manager");
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Wlan Device Manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str());
    // Use FactoryHelper to manage singleton instance and callbacks.
    auto manager = getManager<telux::wlan::IWlanDeviceManager>(
        type, wlanDeviceManager_, wlanDeviceManagerCallbacks_, clientCallback, createAndInit);
    return manager;
}

std::shared_ptr<IApInterfaceManager> WlanFactoryStub::getApInterfaceManager() {
    std::function<std::shared_ptr<telux::wlan::IApInterfaceManager>()> createAndInit
        = [this]() -> std::shared_ptr<telux::wlan::IApInterfaceManager> {
        std::shared_ptr<telux::wlan::ApInterfaceManagerStub> manager
            = std::make_shared<telux::wlan::ApInterfaceManagerStub>();
        if (manager && telux::common::Status::SUCCESS != manager->init()) {
            LOG(ERROR, __FUNCTION__, " WLAN Factory unable to initialize Ap Interface Manager");
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Ap Interface Manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str());
    // Use FactoryHelper to manage singleton instance.
    auto manager
        = getManager<telux::wlan::IApInterfaceManager>(type, apInterfaceManager_, createAndInit);
    return manager;
}

std::shared_ptr<IStaInterfaceManager> WlanFactoryStub::getStaInterfaceManager() {
    std::function<std::shared_ptr<telux::wlan::IStaInterfaceManager>()> createAndInit
        = [this]() -> std::shared_ptr<telux::wlan::IStaInterfaceManager> {
        std::shared_ptr<telux::wlan::StaInterfaceManagerStub> manager
            = std::make_shared<telux::wlan::StaInterfaceManagerStub>();
        if (manager && telux::common::Status::SUCCESS != manager->init()) {
            LOG(ERROR, __FUNCTION__, " WLAN Factory unable to initialize Sta Interface Manager");
            return nullptr;
        }
        return manager;
    };
    auto type = std::string("Sta Interface Manager");
    LOG(DEBUG, __FUNCTION__, ": Requesting ", type.c_str());
    // Use FactoryHelper to manage singleton instance.
    auto manager
        = getManager<telux::wlan::IStaInterfaceManager>(type, staInterfaceManager_, createAndInit);
    return manager;
}

}  // namespace wlan
}  // namespace telux