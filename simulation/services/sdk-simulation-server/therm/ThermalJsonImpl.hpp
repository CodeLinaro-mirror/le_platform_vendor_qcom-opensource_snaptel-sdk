/*
 *  Copyright (c) 2024 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef THERMAL_JSON_IMPL_HPP
#define THERMAL_JSON_IMPL_HPP

#include <algorithm>
#include <map>
#include <telux/common/CommonDefines.hpp>
#include "../../libs/common/Logger.hpp"
#include "../../../../include/telux/therm/ThermalManager.hpp"
#include "../../../libs/common/CommonUtils.hpp"
#include "../../../common/therm/ThermalZone.hpp"
#include "../../../common/therm/CoolingDevice.hpp"

#define TREND_RAISING   1
#define TREND_STABLE    0
#define TREND_DROPPING  -1
#define CROSSED_OVER    1
#define CROSSED_UNDER   0

class ThermalJsonImpl {
    public:

        ThermalJsonImpl();
        ~ThermalJsonImpl();

        telux::common::Status init();

        telux::common::ServiceStatus initServiceStatus();
        telux::common::ServiceStatus getServiceStatus();

        int getSubsystemReadyDelay();
        telux::therm::TripType getTripType(std::string tripType);

        telux::common::Status getThermalZones();
        telux::common::Status getCoolingDevices();

        telux::common::Status getThermalZones(
                std::vector<std::shared_ptr<telux::therm::ThermalZone>> &tZones);
        telux::common::Status getCoolingDevices(
                std::vector<std::shared_ptr<telux::therm::CoolingDevice>> &cDevs);

        telux::common::Status getThermalZoneById(int tZoneId,
                std::shared_ptr<telux::therm::ThermalZone> &tz);
        telux::common::Status getCoolingDeviceById(int cDevId,
                std::shared_ptr<telux::therm::CoolingDevice> &cDev);

        std::map<int, int> getCoolingDeviceLevel(
                int tZoneId, int tripId, int trend, int cDevId = -1);

        std::vector<std::shared_ptr<telux::therm::ThermalZone>> tZoneList_;
        std::vector<std::shared_ptr<telux::therm::CoolingDevice>> cDevList_;

    private:
        telux::common::Status findId(std::string api, int id);

        Json::Value thermState_;
        Json::Value thermMgrApi_;
        telux::common::ServiceStatus serviceStatus_ =
            telux::common::ServiceStatus::SERVICE_UNAVAILABLE;
        std::mutex mutex_;
};

#endif // THERMAL_JSON_IMPL_HPP
