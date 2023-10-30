/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SensorFactoryStub.hpp
 *
 *
 */

#ifndef SENSORFACTORYIMPL_HPP
#define SENSORFACTORYIMPL_HPP

#include <memory>
#include <vector>
#include <mutex>

#include <telux/sensor/SensorFactory.hpp>
#include "SensorFeatureManagerStub.hpp"

namespace telux{
namespace sensor{

class SensorFactoryStub : public SensorFactory {
   public:
      static SensorFactory &getInstance();
      std::shared_ptr<ISensorManager> getSensorManager(
          telux::common::InitResponseCb clientCallback = nullptr) override;
      std::shared_ptr<ISensorFeatureManager> getSensorFeatureManager(
          telux::common::InitResponseCb clientCallback = nullptr) override;

   private:
      SensorFactoryStub();
      ~SensorFactoryStub();

      /**
       * Helper method to notify all listeners the completion of initialization with the provided
       * status
       */
      void initCompleteNotifier(std::vector<telux::common::InitResponseCb> &initCallbacks_,
          telux::common::ServiceStatus status);
      std::weak_ptr<SensorFeatureManagerStub> sensorFeatureManager_;
      std::vector<telux::common::InitResponseCb> smInitCallbacks_;  //SensorManager callbacks
      std::vector<telux::common::InitResponseCb> sfmInitCallbacks_; //SensorFeatureManager callbacks
      std::mutex factoryGuard_;
  };
}
}
#endif  // SENSORFACTORY_HPP