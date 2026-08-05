/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Changes from Qualcomm Technologies, Inc. are provided under the following license:
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to get domain preferences and how to configure and query
 * the system information notification filter. The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a IServingSystemManager instance from the PhoneFactory.
 * 3. Wait for the serving system service to become available.
 * 4. Register a listener for SYSTEM_INFO notifications.
 * 5. Set the system information notification filter.
 * 6. Get the currently configured system information notification filter.
 * 7. Request service domain preference.
 * 8. Receive the service domain preference response.
 * 9. Deregister the listener before exiting.
 *
 * Usage:
 * # ./serving_system_app
 */

#include <errno.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <memory>
#include <string>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/ServingSystemManager.hpp>

/*
 * Listener implementation that prints the notifications that are affected by the system
 * information notification filter, i.e. onSystemInfoChanged, onCallBarringInfoChanged and
 * onSmsCapabilityChanged.
 */
class ServingSystemListener : public telux::tel::IServingSystemListener {
 public:
    void onSystemInfoChanged(telux::tel::ServingSystemInfo sysInfo) override {
        std::cout << "[Notification] System info changed - RAT: " << static_cast<int>(sysInfo.rat)
                  << ", domain: " << static_cast<int>(sysInfo.domain)
                  << ", state: " << static_cast<int>(sysInfo.state) << std::endl;
    }

    void onCallBarringInfoChanged(std::vector<telux::tel::CallBarringInfo> barringInfo) override {
        std::cout << "[Notification] Call barring info changed, count: " << barringInfo.size()
                  << std::endl;
    }

    void onSmsCapabilityChanged(telux::tel::SmsCapability smsCapability) override {
        std::cout << "[Notification] SMS capability changed - domain: "
                  << static_cast<int>(smsCapability.domain) << std::endl;
    }
};

class ServingSystemInfo : public std::enable_shared_from_this<ServingSystemInfo> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        servingSystemMgr_ = phoneFactory.getServingSystemManager(
            DEFAULT_SLOT_ID, [&p](telux::common::ServiceStatus status) { p.set_value(status); });

        if (!servingSystemMgr_) {
            std::cout << "Can't get IServingSystemManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Serving system service unavailable, status "
                      << static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int registerServingSystemListener() {
        /* Step - 4 */
        listener_ = std::make_shared<ServingSystemListener>();

        telux::tel::ServingSystemNotificationMask mask;
        mask.set(telux::tel::ServingSystemNotificationType::SYSTEM_INFO);

        telux::common::Status status = servingSystemMgr_->registerListener(listener_, mask);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Registered listener for SYSTEM_INFO notifications" << std::endl;
        return 0;
    }

    void deregisterServingSystemListener() {
        /* Step - 9 */
        telux::tel::ServingSystemNotificationMask mask;
        mask.set(telux::tel::ServingSystemNotificationType::SYSTEM_INFO);

        telux::common::Status status = servingSystemMgr_->deregisterListener(listener_, mask);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " << static_cast<int>(status) << std::endl;
            return;
        }

        std::cout << "Deregistered listener for SYSTEM_INFO notifications" << std::endl;
    }

    int getServingSystemInfo() {
        telux::common::Status status;

        auto responseCb = std::bind(&ServingSystemInfo::serviceDomainResponse, this,
            std::placeholders::_1, std::placeholders::_2);

        /* Step - 7 */
        status = servingSystemMgr_->requestServiceDomainPreference(responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request preference, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        return 0;
    }

    /* Step - 8 */
    void serviceDomainResponse(
        telux::tel::ServiceDomainPreference preference, telux::common::ErrorCode errorCode) {

        if (errorCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to get preference" << std::endl;
            return;
        }

        std::cout << "Preference: " << getServiceDomain(preference) << std::endl;
    }

    int setSystemInfoNotificationFilter() {
        telux::common::ErrorCode errorCode;

        /* Step - 5 */
        /* Restrict the onSystemInfoChanged notification to be triggered only when the
         * service registration state or the serving RAT changes. */
        telux::tel::SystemInfoNotificationFilterMask mask;
        mask.set(telux::tel::SystemInfoNotificationFilter::SRV_STATUS);
        mask.set(telux::tel::SystemInfoNotificationFilter::SRV_RAT_TECH_CHANGE);

        errorCode = servingSystemMgr_->setSystemInfoNotificationFilter(mask);
        if (errorCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set system info notification filter, err "
                      << static_cast<int>(errorCode) << std::endl;
            return -EIO;
        }

        std::cout << "System info notification filter set to: ";
        printFilterMask(mask);
        return 0;
    }

    int getSystemInfoNotificationFilter() {
        telux::common::ErrorCode errorCode;
        telux::tel::SystemInfoNotificationFilterMask mask;

        /* Step - 6 */
        errorCode = servingSystemMgr_->getSystemInfoNotificationFilter(mask);
        if (errorCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't get system info notification filter, err "
                      << static_cast<int>(errorCode) << std::endl;
            return -EIO;
        }

        std::cout << "System info notification filter is currently: ";
        printFilterMask(mask);
        return 0;
    }

 private:
    void printFilterMask(telux::tel::SystemInfoNotificationFilterMask mask) {
        if (mask.none()) {
            std::cout << "empty (filtering disabled, all changes are reported)" << std::endl;
            return;
        }

        if (mask.test(telux::tel::SystemInfoNotificationFilter::SRV_STATUS)) {
            std::cout << "SRV_STATUS ";
        }
        if (mask.test(telux::tel::SystemInfoNotificationFilter::SRV_DOMAIN)) {
            std::cout << "SRV_DOMAIN ";
        }
        if (mask.test(telux::tel::SystemInfoNotificationFilter::CS_BAR_STATUS)) {
            std::cout << "CS_BAR_STATUS ";
        }
        if (mask.test(telux::tel::SystemInfoNotificationFilter::PS_BAR_STATUS)) {
            std::cout << "PS_BAR_STATUS ";
        }
        if (mask.test(telux::tel::SystemInfoNotificationFilter::LTE_SMS_STATUS)) {
            std::cout << "LTE_SMS_STATUS ";
        }
        if (mask.test(telux::tel::SystemInfoNotificationFilter::SRV_STATUS_TOGGLE)) {
            std::cout << "SRV_STATUS_TOGGLE ";
        }
        if (mask.test(telux::tel::SystemInfoNotificationFilter::SRV_RAT_TECH_CHANGE)) {
            std::cout << "SRV_RAT_TECH_CHANGE ";
        }
        std::cout << std::endl;
    }

    std::string getServiceDomain(telux::tel::ServiceDomainPreference preference) {
        switch (preference) {
            case telux::tel::ServiceDomainPreference::CS_ONLY:
                return " Circuit Switched(CS) only";
            case telux::tel::ServiceDomainPreference::PS_ONLY:
                return " Packet Switched(PS) only";
            case telux::tel::ServiceDomainPreference::CS_PS:
                return " Circuit Switched and Packet Switched ";
            default:
                return " Unknown";
        }
    }

    std::shared_ptr<telux::tel::IServingSystemManager> servingSystemMgr_;
    std::shared_ptr<ServingSystemListener> listener_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<ServingSystemInfo> app;

    try {
        app = std::make_shared<ServingSystemInfo>();
    } catch (const std::exception &e) {
        std::cout << "Can't allocate ServingSystemInfo" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->registerServingSystemListener();
    if (ret < 0) {
        return ret;
    }

    ret = app->setSystemInfoNotificationFilter();
    if (ret < 0) {
        return ret;
    }

    ret = app->getSystemInfoNotificationFilter();
    if (ret < 0) {
        return ret;
    }

    ret = app->getServingSystemInfo();
    if (ret < 0) {
        return ret;
    }

    /* Wait for the response for serving system info and any notifications, application
     * specific logic goes here. This wait is just an example */
    std::this_thread::sleep_for(std::chrono::seconds(30));

    app->deregisterServingSystemListener();

    std::cout << "\nServing system app exiting" << std::endl;
    return 0;
}
