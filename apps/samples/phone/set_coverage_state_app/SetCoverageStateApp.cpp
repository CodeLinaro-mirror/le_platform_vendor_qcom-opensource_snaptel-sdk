/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to set coverage state to tune the 5G scan behavior.
 * The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a INetworkSelectionManager instance from the PhoneFactory.
 * 3. Wait for the network selection manager service to become available.
 * 4. Set coverage state.
 * 5. Deinit app.
 *
 * Usage:
 * # ./set_coverage_state_app <SlotId (1 / 2)>
 * E.g. ./set_coverage_state_app 1
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
#include <telux/tel/ServingSystemDefines.hpp>
#include <telux/tel/NetworkSelectionManager.hpp>

class SetCoverageStateApp : public telux::tel::INetworkSelectionListener,
                            public std::enable_shared_from_this<SetCoverageStateApp> {
 public:
    int init(int slotId) {
        telux::common::ServiceStatus serviceStatus;
        std::promise<telux::common::ServiceStatus> p{};

        auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

        nwSelectionMgr_ = phoneFactory.getNetworkSelectionManager(
            slotId, [&p](telux::common::ServiceStatus status) { p.set_value(status); });

        if (!nwSelectionMgr_) {
            std::cout << "Can't get INetworkSelectionManager" << std::endl;
            return -ENOMEM;
        }

        serviceStatus = p.get_future().get();
        if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Network selection manager service unavailable, status "
                      << static_cast<int>(serviceStatus) << std::endl;
            return -EIO;
        }

        auto status = nwSelectionMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    int userInputForCoverageState(telux::tel::CoverageState &state) {
        int stateInput = -1;
        std::cout << "Enter coverage state (1-IN_5G-COVERAGE, 2-OUT_OF_5G_COVERAGE): ";
        std::cin >> stateInput;

        switch (stateInput) {
            case 1:
                state = telux::tel::CoverageState::IN_5G_COVERAGE;
                break;
            case 2:
                state = telux::tel::CoverageState::OUT_OF_5G_COVERAGE;
                break;
            default:
                std::cout << "Invalid coverage state input" << std::endl;
                return -EIO;
        }
        return 0;
    }

    int setCoverageState(telux::tel::CoverageState &state) {
        auto errCode = nwSelectionMgr_->setCoverageState(state);
        if (errCode != telux::common::ErrorCode::SUCCESS) {
            std::cout << "Can't set coverage state, err " << static_cast<int>(errCode) << std::endl;
            return -EIO;
        }

        std::cout << " set coverage state succeed" << std::endl;
        return 0;
    }

    int deinit() {
        telux::common::Status status;

        status = nwSelectionMgr_->deregisterListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't deregister listener, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }
        return 0;
    }

 private:
    std::shared_ptr<telux::tel::INetworkSelectionManager> nwSelectionMgr_;
};

int main(int argc, char *argv[]) {

    int ret, slotId;
    std::shared_ptr<SetCoverageStateApp> app;
    telux::tel::CoverageState state;

    if (argc != 2) {
        std::cout << "./set_coverage_state_app <SlotId>" << std::endl;
        return -EINVAL;
    }

    if ((std::atoi(argv[1]) != SlotId::SLOT_ID_1) && (std::atoi(argv[1]) != SlotId::SLOT_ID_2)) {
        std::cout << " Invalid slotId, valid values: 1/2" << std::endl;
        return -EINVAL;
    }
    slotId = static_cast<int>(std::atoi(argv[1]));

    try {
        app = std::make_shared<SetCoverageStateApp>();
    } catch (const std::exception &e) {
        std::cout << "Can't allocate: insufficient memory" << std::endl;
        return -ENOMEM;
    }

    /** Step - 1 */
    ret = app->init(slotId);
    if (ret < 0) {
        return ret;
    }

    /** Step - 2 */
    ret = app->userInputForCoverageState(state);
    if (ret < 0) {
        return ret;
    }

    /** Step - 3 */
    ret = app->setCoverageState(state);
    if (ret < 0) {
        return ret;
    }

    /** Step - 4 */
    ret = app->deinit();
    if (ret < 0) {
        return ret;
    }

    std::cout << "\nSet coverage state app exiting" << std::endl;
    return 0;
}
