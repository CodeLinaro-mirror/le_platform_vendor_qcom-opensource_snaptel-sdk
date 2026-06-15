/*
 *  Copyright (c) 2017-2019, The Linux Foundation. All rights reserved.
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

#include <iostream>
#include <string>
#include <memory>
#include <future>

#include <telux/tel/PhoneListener.hpp>
#include <telux/tel/SmsManager.hpp>
#include <telux/tel/SubscriptionManager.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/loc/LocationFactory.hpp>

#include "MyLocationListener.hpp"
#include "MySmsListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

const std::string GREEN    = "\033[0;32m";
const std::string RED      = "\033[0;31m";
const std::string BOLD_RED = "\033[1;31m";
const std::string DONE     = "\033[0m";  // No color

/**
 * Main routine
 */
int main(int, char **) {

    // [1] Get the PhoneFactory
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

    // [2] Get subscriptionMgr and read Phone number
    std::promise<telux::common::ServiceStatus> subscriptionMgrprom;
    telux::common::Status status;
    auto subscriptionMgr = telux::tel::PhoneFactory::getInstance().getSubscriptionManager(
        [&](telux::common::ServiceStatus status) { subscriptionMgrprom.set_value(status); });
    if (!subscriptionMgr) {
        std::cout << "ERROR - Failed to get SubscriptionManager instance \n";
        return 0;
    }
    // [3] Check if SubscriptionManager subsystem is ready
    telux::common::ServiceStatus subscriptionMgrStatus = subscriptionMgr->getServiceStatus();
    if (subscriptionMgrStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "SubscriptionManager subsystem is not ready, Please wait \n";
    }
    // [3.1] Wait for SubscriptionManager subsystem to be ready
    subscriptionMgrStatus = subscriptionMgrprom.get_future().get();
    if (subscriptionMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "SubscriptionManager subsystem is ready \n";
    } else {
        // Exit the application, if SDK is unable to initialize telephony subsystems
        std::cout << "ERROR - Unable to initialize SubscriptionManager subsystem \n";
        return 0;
    }
    auto subscription = subscriptionMgr->getSubscription(DEFAULT_SLOT_ID, &status);
    if (!subscription) {
        std::cout << " *** ERROR - Subscription is empty" << std::endl;
        return 0;
    }

    std::cout << "\n\n";
    std::cout << "-------------------------------------------\n";
    std::cout << "            Location Tracker App \n";
    std::cout << "-------------------------------------------\n";

    // [4] Get the LocationFactory and LocationManager instances
    auto &locationFactory = telux::loc::LocationFactory::getInstance();

    std::promise<telux::common::ServiceStatus> locationMgrProm;
    std::shared_ptr<telux::loc::ILocationManager> locationMgr
        = locationFactory.getLocationManager(
            [&locationMgrProm](telux::common::ServiceStatus srvStatus) {
                locationMgrProm.set_value(srvStatus);
            });
    if (!locationMgr) {
        std::cout << " *** ERROR - Failed to get LocationManager instance" << std::endl;
        return 0;
    }

    // [5] Wait for location subsystem to be ready
    telux::common::ServiceStatus locServiceStatus = locationMgrProm.get_future().get();

    // [6.2] Exit the application, if SDK is unable to initialize location subsystems
    if (locServiceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << std::endl << "Send SMS in the following format";
        std::cout << std::endl
                  << "to the phone number " << subscription->getPhoneNumber()
                  << " to get current location" << std::endl;
        std::cout << std::endl
                  << "Format: " << GREEN << "Location <SMS Token>" << DONE << std::endl;
        std::cout << "Example: Location 1234" << std::endl;
    } else {
        std::cout << " *** ERROR - Unable to initialize GNSS Location subsystem" << std::endl;
        exit(1);
    }

    // [7] Get Default SMS manager instance
    std::promise<telux::common::ServiceStatus> smsMgrProm;
    std::shared_ptr<telux::tel::ISmsManager> smsManager = phoneFactory.getSmsManager(
        DEFAULT_PHONE_ID,
        [&smsMgrProm](telux::common::ServiceStatus srvStatus) {
            smsMgrProm.set_value(srvStatus);
        });
    if (!smsManager) {
        std::cout << " *** ERROR - Failed to get SmsManager instance" << std::endl;
        exit(1);
    }
    telux::common::ServiceStatus smsServiceStatus = smsMgrProm.get_future().get();
    if (smsServiceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << " *** ERROR - SmsManager subsystem unavailable" << std::endl;
        exit(1);
    }

    // [8] Instantiate LocationListener, SmsListener
    std::shared_ptr<MyLocationListener> myLocationListener = std::make_shared<MyLocationListener>();
    std::shared_ptr<MySmsListener> mySmsListener           = std::make_shared<MySmsListener>();
    mySmsListener->setLocationListener(myLocationListener);
    smsManager->registerListener(mySmsListener);

    // [9] Instantiate global ILocationListener
    locationMgr->registerListenerEx(myLocationListener);
    // [10]Starting the reports for fixes
    locationMgr->startDetailedReports(1000, NULL);
    // [11] Exit logic is specific to an application
    std::cout << "Press enter to exit" << std::endl;
    std::string input;
    std::getline(std::cin, input);
    locationMgr->deRegisterListenerEx(myLocationListener);
    std::cout << "Exiting application..." << std::endl;
    return 0;
}
