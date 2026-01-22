/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to configure signal strength notification. The steps are as
 * follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a IPhoneManager instance from the PhoneFactory.
 * 3. Wait for the phone manager service to become available.
 * 4. Create a phone instance.
 * 5. Configure signal strength information.
 * 6. Update the response.
 *
 * Usage:
 * # ./configure_signal_strength_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneManager.hpp>

class PhoneMaker : public std::enable_shared_from_this<PhoneMaker> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        phoneMgr_ = phoneFactory.getPhoneManager(
            [&p](telux::common::ServiceStatus status) { p.set_value(status); });

        if (!phoneMgr_) {
            std::cout << "Can't get IPhoneManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Phone manager service unavailable, status "
                      << static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }
        /* Step - 4 */
        if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::vector<int> phoneIds;
            telux::common::Status status = phoneMgr_->getPhoneIds(phoneIds);
            if (status == telux::common::Status::SUCCESS) {
                for (unsigned int index = 1; index <= phoneIds.size(); index++) {
                    auto phone = phoneMgr_->getPhone(index);
                    if (phone != nullptr) {
                        phones_.emplace_back(phone);
                    }
                }
            }
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int configureSignalStrength() {
        auto respCb = [this](telux::common::ErrorCode errorCode) {
            onConfigureSignalStrengthResponse(errorCode);
        };
        telux::common::Status status;
        int phoneId = DEFAULT_PHONE_ID;
        if (phones_.empty()) {
            std::cout << "No phones available" << std::endl;
            return -ENODEV;
        }
        auto phone = phones_[phoneId - 1];
        /* Step - 5 */
        /* Use case 1: Configure delta for LTE */
        std::vector<telux::tel::SignalStrengthConfigEx> sigStrengthConfigList = {};
        telux::tel::SignalStrengthConfigEx sigStrengthConfig                  = {};
        telux::tel::SignalStrengthConfigMask configMask                       = {};
        telux::tel::SignalStrengthConfigData sigData                          = {};
        configMask.set(telux::tel::SignalStrengthConfigExType::DELTA);
        sigStrengthConfig.radioTech      = telux::tel::RadioTechnology::RADIO_TECH_LTE;
        sigStrengthConfig.configTypeMask = configMask;
        sigData.sigMeasType              = telux::tel::SignalStrengthMeasurementType::RSSI;
        sigData.delta                    = 100;  // dbm * 10
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigData.sigMeasType = telux::tel::SignalStrengthMeasurementType::RSRP;
        sigData.delta       = 200;  // dbm * 10
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigData.sigMeasType = telux::tel::SignalStrengthMeasurementType::RSRQ;
        sigData.delta       = 150;  // dbm * 10
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigData.sigMeasType = telux::tel::SignalStrengthMeasurementType::SNR;
        sigData.delta       = 100;  // dbm * 10
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigStrengthConfigList.emplace_back(sigStrengthConfig);
        int hysTimer = 0;
        status       = phone->configureSignalStrength(sigStrengthConfigList, hysTimer, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't configure signal strength, err " << static_cast<int>(status)
                      << std::endl;
            return -EIO;
        }

        /* Use case 2: Configure threshold list for NR5G */
        // cear previous data
        sigStrengthConfig.sigConfigData.clear();
        sigStrengthConfigList.clear();
        configMask.reset();
        configMask.set(telux::tel::SignalStrengthConfigExType::THRESHOLD);
        sigStrengthConfig.radioTech      = telux::tel::RadioTechnology::RADIO_TECH_NR5G;
        sigStrengthConfig.configTypeMask = configMask;
        sigData.sigMeasType              = telux::tel::SignalStrengthMeasurementType::RSRP;
        sigData.thresholdList            = {-1400, -440};
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigData.sigMeasType   = telux::tel::SignalStrengthMeasurementType::RSRQ;
        sigData.thresholdList = {-200, -30};
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigData.sigMeasType   = telux::tel::SignalStrengthMeasurementType::SNR;
        sigData.thresholdList = {-2000, 3000};
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigStrengthConfigList.emplace_back(sigStrengthConfig);
        hysTimer = 0;
        status   = phone->configureSignalStrength(sigStrengthConfigList, hysTimer, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't configure signal strength, err " << static_cast<int>(status)
                      << std::endl;
            return -EIO;
        }

        /* Use case 3: Configure thresholdlist and hystersis db for wcdma */
        // cear previous data
        sigStrengthConfig.sigConfigData.clear();
        sigStrengthConfigList.clear();
        configMask.reset();
        configMask.set(telux::tel::SignalStrengthConfigExType::THRESHOLD);
        configMask.set(telux::tel::SignalStrengthConfigExType::HYSTERESIS_DB);
        sigStrengthConfig.radioTech      = telux::tel::RadioTechnology::RADIO_TECH_UMTS;
        sigStrengthConfig.configTypeMask = configMask;
        sigData.sigMeasType              = telux::tel::SignalStrengthMeasurementType::RSSI;
        sigData.thresholdList            = {-1130, -510};
        sigData.hysteresisDb             = 150;
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigData.sigMeasType   = telux::tel::SignalStrengthMeasurementType::ECIO;
        sigData.thresholdList = {-240, 0};
        sigData.hysteresisDb  = 200;
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigData.sigMeasType   = telux::tel::SignalStrengthMeasurementType::RSCP;
        sigData.thresholdList = {-1200, -240};
        sigData.hysteresisDb  = 100;
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigStrengthConfigList.emplace_back(sigStrengthConfig);
        hysTimer = 0;
        status   = phone->configureSignalStrength(sigStrengthConfigList, hysTimer, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't configure signal strength, err " << static_cast<int>(status)
                      << std::endl;
            return -EIO;
        }

        /* Use case 4: Configure hystersis timer */
        // cear previous data
        sigStrengthConfig.sigConfigData.clear();
        sigStrengthConfigList.clear();
        configMask.reset();
        configMask.set(telux::tel::SignalStrengthConfigExType::DELTA);
        sigStrengthConfig.radioTech      = telux::tel::RadioTechnology::RADIO_TECH_LTE;
        sigStrengthConfig.configTypeMask = configMask;
        sigData.sigMeasType              = telux::tel::SignalStrengthMeasurementType::RSSI;
        sigData.delta                    = 100;
        hysTimer                         = 5000;
        sigStrengthConfig.sigConfigData.emplace_back(sigData);
        sigStrengthConfigList.emplace_back(sigStrengthConfig);
        status = phone->configureSignalStrength(sigStrengthConfigList, hysTimer, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't configure signal strength, err " << static_cast<int>(status)
                      << std::endl;
            return -EIO;
        }
        return 0;
    }

    /* Step - 6 */
    void onConfigureSignalStrengthResponse(telux::common::ErrorCode error) {

        std::cout << "\n";
        if (error == telux::common::ErrorCode::SUCCESS) {
            std::cout << "Configure SignalStrength request executed successfully" << std::endl;
        } else {
            std::cout
                << "Configure SignalStrength request failed, errorCode: " << static_cast<int>(error)
                << std::endl;
        }
    }

 private:
    std::vector<std::shared_ptr<telux::tel::IPhone>> phones_;
    std::shared_ptr<telux::tel::IPhoneManager> phoneMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<PhoneMaker> app;

    try {
        app = std::make_shared<PhoneMaker>();
    } catch (const std::exception &e) {
        std::cout << "Can't allocate PhoneMaker" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->configureSignalStrength();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses */
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "\nConfigure signal strength app exiting" << std::endl;
    return 0;
}
