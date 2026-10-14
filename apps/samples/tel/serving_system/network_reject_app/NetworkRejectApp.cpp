/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file: NetworkRejectApp.cpp
 *
 * @brief: Sample application demonstrating how to listen for network
 * registration rejection events, including the Cell identifier of the rejecting cell.
 *
 * This sample registers an IServingSystemListener for onNetworkRejection
 * notification and prints all available reject information is received from the modem.
 * The steps are as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a IServingSystemManager instance from the PhoneFactory.
 * 3. Wait for the serving system manager to become available.
 * 4. Register an IServingSystemListener for onNetworkRejection.
 * 5. Deregister listener before exit
 *
 * Usage:
 * # ./network_reject_app
 */

#include <iostream>
#include <future>
#include <memory>
#include <string>

#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/ServingSystemManager.hpp>

#define DEFAULT_SLOT_ID 1

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

static std::string radioTechToString(telux::tel::RadioTechnology rat) {
    switch (rat) {
        case telux::tel::RadioTechnology::RADIO_TECH_GSM:
            return "GSM";
        case telux::tel::RadioTechnology::RADIO_TECH_UMTS:
            return "WCDMA";
        case telux::tel::RadioTechnology::RADIO_TECH_LTE:
            return "LTE";
        case telux::tel::RadioTechnology::RADIO_TECH_NR5G:
            return "NR5G";
        default:
            return "UNKNOWN";
    }
}

static std::string serviceDomainToString(telux::tel::ServiceDomain domain) {
    switch (domain) {
        case telux::tel::ServiceDomain::NO_SRV:
            return "No Service";
        case telux::tel::ServiceDomain::CS_ONLY:
            return "CS Only";
        case telux::tel::ServiceDomain::PS_ONLY:
            return "PS Only";
        case telux::tel::ServiceDomain::CS_PS:
            return "CS+PS";
        case telux::tel::ServiceDomain::CAMPED:
            return "Camped";
        default:
            return "UNKNOWN";
    }
}

class NetworkRejectListener : public telux::tel::IServingSystemListener {
 public:
    void onNetworkRejection(telux::tel::NetworkRejectInfo rejectInfo) override {
        std::ostringstream oss;
        oss << " Network registration rejection occurred.\n"
            << "  RAT          : " << radioTechToString(rejectInfo.rejectSrvInfo.rat) << "\n"
            << "  Service Domain: " << serviceDomainToString(rejectInfo.rejectSrvInfo.domain)
            << "\n"
            << "  Reject Cause : " << static_cast<int>(rejectInfo.rejectCause) << "\n"
            << "  MCC          : " << rejectInfo.mcc << "\n"
            << "  MNC          : " << rejectInfo.mnc << "\n"
            << "  Cell ID      : "
            << (rejectInfo.cellId == 0xFFFFFFFFFFFFFFFF ? "UNAVAILABLE"
                                                        : std::to_string(rejectInfo.cellId))
            << "\n";
        PRINT_NOTIFICATION << oss.str();
    }
};

int main(int argc, char *argv[]) {
    telux::common::ServiceStatus serviceStatus;
    std::promise<telux::common::ServiceStatus> p{};

    // [1] PhoneFactory instance
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

    /* [2] Get serving system manager instance*/
    auto servingSystemMgr = phoneFactory.getServingSystemManager(
        DEFAULT_SLOT_ID, [&p](telux::common::ServiceStatus status) { p.set_value(status); });

    if (!servingSystemMgr) {
        std::cout << "Can't get IServingSystemManager" << std::endl;
        return -ENOMEM;
    }

    /* [3] Wait for the serving system manager to become available */
    serviceStatus = p.get_future().get();
    if (serviceStatus != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Serving system service unavailable, status "
                  << static_cast<int>(serviceStatus) << std::endl;
        return -EIO;
    }
    std::cout << "Serving subsystem is ready." << std::endl;

    // [4] Register listener for NETWORK_REJ_INFO notifications only
    auto listener = std::make_shared<NetworkRejectListener>();
    telux::tel::ServingSystemNotificationMask mask;
    mask.set(telux::tel::ServingSystemNotificationType::NETWORK_REJ_INFO);
    auto status = servingSystemMgr->registerListener(listener, mask);
    if (status != telux::common::Status::SUCCESS) {
        std::cerr << "ERROR - registerListener failed: " << static_cast<int>(status) << std::endl;
        return 1;
    }
    std::cout << "Listening for network rejection events (including Cell ID)...\n"
              << "Press ENTER to exit.\n"
              << std::endl;

    std::cin.ignore();

    // [5] Deregister listener before exit
    status = servingSystemMgr->deregisterListener(listener, mask);
    if (status != telux::common::Status::SUCCESS) {
        std::cerr << "ERROR - deregisterListener failed: " << static_cast<int>(status) << std::endl;
        return 1;
    }
    return 0;
}
