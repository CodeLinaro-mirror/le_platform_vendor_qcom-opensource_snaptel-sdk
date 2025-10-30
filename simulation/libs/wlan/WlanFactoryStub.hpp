/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       WlanFactoryStub.hpp
 *
 * @brief      Implementation of WlanFactory
 *
 */

#ifndef WLANFACTORYIMPL_HPP
#define WLANFACTORYIMPL_HPP

#include <memory>
#include <mutex>
#include <vector>

#include "common/FactoryHelper.hpp"
#include <common/AsyncTaskQueue.hpp>
#include <telux/wlan/WlanFactory.hpp>

namespace telux {
namespace wlan {

/**
 * @brief WlanFactoryStub is the singleton factory class for WLAN management in
 * simulation.
 */
class WlanFactoryStub : public WlanFactory,
                        public telux::common::FactoryHelper {
public:
  static WlanFactory &getInstance();

  virtual std::shared_ptr<IWlanDeviceManager>
  getWlanDeviceManager(telux::common::InitResponseCb clientCallback) override;

  virtual std::shared_ptr<IApInterfaceManager> getApInterfaceManager() override;

  virtual std::shared_ptr<IStaInterfaceManager>
  getStaInterfaceManager() override;

private:
  WlanFactoryStub();
  ~WlanFactoryStub();

  // Deleted copy constructor and assignment operator to prevent copying.
  WlanFactoryStub(const WlanFactoryStub &) = delete;
  WlanFactoryStub &operator=(const WlanFactoryStub &) = delete;

  std::weak_ptr<IWlanDeviceManager> wlanDeviceManager_;
  std::vector<telux::common::InitResponseCb> wlanDeviceManagerCallbacks_;
  std::weak_ptr<IApInterfaceManager> apInterfaceManager_;
  std::weak_ptr<IStaInterfaceManager> staInterfaceManager_;
};

} // namespace wlan
} // namespace telux

#endif // WLANFACTORYIMPL_HPP