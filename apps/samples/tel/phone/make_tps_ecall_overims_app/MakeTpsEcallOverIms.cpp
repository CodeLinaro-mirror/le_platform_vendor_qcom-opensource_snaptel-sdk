/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to make a third-party/private eCall.
 * The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a ICallManager instance from the PhoneFactory.
 * 3. Wait for the call manager service to become available.
 * 4. Trigger an ecall.
 * 5. Receive status of the ecall in callback.
 * 6. Wait for the ecall to finish.
 *
 * Usage:
 * # ./make_tps_ecall_overims_app
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
#include <telux/tel/CallManager.hpp>

class ECaller : public std::enable_shared_from_this<ECaller> {
 public:
    int init() {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        /* Step - 1 */
        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        /* Step - 2 */
        callMgr_ = phoneFactory.getCallManager(
            [&p](telux::common::ServiceStatus status) { p.set_value(status); });

        if (!callMgr_) {
            std::cout << "Can't get ICallManager" << std::endl;
            return -ENOMEM;
        }

        /* Step - 3 */
        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Call manager service unavailable, status "
                      << static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int triggerECall() {
        telux::common::Status status;
        std::string dialNumber("+1xxxxxxxxxx");

        /* Optional SIP headers */
        telux::tel::CustomSipHeader header = {telux::tel::CONTENT_HEADER, ""};

        /* MSD data */
        std::vector<uint8_t> rawData{2, 41, 68, 6, 128, 227, 10, 81, 67, 158, 41, 85, 212, 56, 0,
            128, 4, 52, 10, 140, 65, 89, 164, 56, 119, 207, 131, 54, 210, 63, 65, 104, 16, 24, 8,
            32, 19, 198, 68, 0, 0, 48, 20};

        auto responseCb = std::bind(
            &ECaller::makeCallResponse, this, std::placeholders::_1, std::placeholders::_2);

        /* Step - 4 */
        status = callMgr_->makeECall(DEFAULT_PHONE_ID, dialNumber, rawData, header, responseCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't call, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Call initiated" << std::endl;
        return 0;
    }

    /* Step - 5 */
    void makeCallResponse(telux::common::ErrorCode ec, std::shared_ptr<telux::tel::ICall> call) {
        std::cout << "makeCallResponse()" << std::endl;

        if (ec != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Failed to call, err " << static_cast<int>(ec) << std::endl;
            return;
        }

        dialedCall_ = call;

        std::cout << "Index " << call->getCallIndex() << " direction "
                  << static_cast<int>(call->getCallDirection()) << " number "
                  << call->getRemotePartyNumber() << std::endl;
    }

 private:
    std::shared_ptr<telux::tel::ICall> dialedCall_;
    std::shared_ptr<telux::tel::ICallManager> callMgr_;
};

int main(int argc, char *argv[]) {

    int ret;
    std::shared_ptr<ECaller> app;

    try {
        app = std::make_shared<ECaller>();
    } catch (const std::exception &e) {
        std::cout << "Can't allocate ECaller" << std::endl;
        return -ENOMEM;
    }

    ret = app->init();
    if (ret < 0) {
        return ret;
    }

    ret = app->triggerECall();
    if (ret < 0) {
        return ret;
    }

    /* Step - 6 */
    /* Application specific logic goes here, this wait is just an example */
    std::this_thread::sleep_for(std::chrono::minutes(3));

    std::cout << "\nEcall app exiting" << std::endl;
    return 0;
}
