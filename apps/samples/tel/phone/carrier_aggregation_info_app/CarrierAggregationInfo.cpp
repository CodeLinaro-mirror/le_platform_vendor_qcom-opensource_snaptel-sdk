/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to request Carrier Aggregation information. The steps are
 * as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a IPhoneManager instance from the PhoneFactory.
 * 3. Wait for the phone manager service to become available.
 * 4. Create a phone instance.
 * 5. Request Carrier Aggregation information.
 * 6. Update the response.
 *
 * Usage:
 * # ./carrier_aggregation_info_app
 */

#include <errno.h>

#include <iostream>
#include <memory>
#include <cstdlib>
#include <future>
#include <chrono>
#include <thread>

#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Phone.hpp>
#include <telux/tel/PhoneDefines.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include <telux/tel/PhoneManager.hpp>
#include <telux/tel/PhoneListener.hpp>

class PhoneMaker : public telux::tel::IPhoneListener,
                   public std::enable_shared_from_this<PhoneMaker> {
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

        /* Step - 5 */
        telux::common::Status status = phoneMgr_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't register listener, err " << static_cast<int>(status) << std::endl;
            return -EIO;
        }

        std::cout << "Initialization complete" << std::endl;
        return 0;
    }

    void deInit() {
        phoneMgr_->removeListener(shared_from_this());
    }

    int requestCarrierAggregationCellInfo() {
        auto respCb = [this](const telux::tel::CarrierAggregationInfo &info,
                          telux::common::ErrorCode errorCode) {
            onCarrierAggregationInfoResponse(info, errorCode);
        };

        telux::common::Status status;
        int phoneId = DEFAULT_PHONE_ID;
        if (phones_.empty()) {
            std::cout << "No phones available" << std::endl;
            return -ENODEV;
        }
        auto phone = phones_[phoneId - 1];
        /* Step - 5 */
        status = phone->requestCarrierAggregationCellInfo(
            telux::tel::CarrierAggregationType::LteCa, respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request Carrier Aggregation info, err " << static_cast<int>(status)
                      << std::endl;
            return -EIO;
        }

        return 0;
    }

    std::string lteBandwidthToString(telux::tel::LteBandwidth bw) {
        switch (bw) {
            case telux::tel::LteBandwidth::BW_1_4_MHZ:
                return "1.4 MHz";
            case telux::tel::LteBandwidth::BW_3_MHZ:
                return "3 MHz";
            case telux::tel::LteBandwidth::BW_5_MHZ:
                return "5 MHz";
            case telux::tel::LteBandwidth::BW_10_MHZ:
                return "10 MHz";
            case telux::tel::LteBandwidth::BW_15_MHZ:
                return "15 MHz";
            case telux::tel::LteBandwidth::BW_20_MHZ:
                return "20 MHz";
            default:
                return "Unknown";
        }
    }

    std::string lteSCellStateToString(telux::tel::LteSCellState state) {
        switch (state) {
            case telux::tel::LteSCellState::DECONFIGURED:
                return "DECONFIGURED";
            case telux::tel::LteSCellState::CONFIGURED_DEACTIVATED:
                return "CONFIGURED_DEACTIVATED";
            case telux::tel::LteSCellState::CONFIGURED_ACTIVATED:
                return "CONFIGURED_ACTIVATED";
            default:
                return "Unknown";
        }
    }

    /* Step - 6 */
    void onCarrierAggregationInfoResponse(
        const telux::tel::CarrierAggregationInfo &info, telux::common::ErrorCode error) {
        std::cout << std::endl;
        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "requestCarrierAggregationCellInfo failed, errorCode: "
                      << static_cast<int>(error) << std::endl;
            return;
        }

        if (info.type == telux::tel::CarrierAggregationType::LteCa && info.lteInfo) {
            std::cout << "RAT: LTE" << std::endl;
            std::cout << "--- LTE PCell ---" << std::endl;
            std::cout << "PCI: " << info.lteInfo->pCell.cell.pci << std::endl;
            std::cout << "Freq (EARFCN): " << info.lteInfo->pCell.cell.freq << std::endl;
            std::cout << "Bandwidth: " << lteBandwidthToString(info.lteInfo->pCell.cell.bandwidth)
                      << std::endl;
            std::cout << "Band: " << static_cast<int>(info.lteInfo->pCell.cell.band) << std::endl;

            std::cout << "Number of SCells: " << info.lteInfo->sCell.size() << std::endl;
            for (size_t i = 0; i < info.lteInfo->sCell.size(); i++) {
                const auto &sCell = info.lteInfo->sCell[i];
                std::cout << "--- LTE SCell[" << i << "] ---" << std::endl;
                std::cout << "PCI: " << sCell.cell.pci << std::endl;
                std::cout << "Freq (EARFCN): " << sCell.cell.freq << std::endl;
                std::cout << "Bandwidth: " << lteBandwidthToString(sCell.cell.bandwidth)
                          << std::endl;
                std::cout << "Band: " << static_cast<int>(sCell.cell.band) << std::endl;
                std::cout << "UL Configured: " << (sCell.ulConfigured ? "Yes" : "No") << std::endl;
                std::cout << "SCell State: " << lteSCellStateToString(sCell.scellState)
                          << std::endl;
            }
        } else {
            std::cout << "No LTE Carrier Aggregation info available" << std::endl;
        }
    }

    /* Unsolicited notification triggered on Carrier Aggregation info change */
    void onCarrierAggregationInfoChanged(
        int phoneId, const telux::tel::CarrierAggregationInfo info) override {
        std::cout << "\nonCarrierAggregationInfoChanged, phoneId: " << phoneId << std::endl;
        onCarrierAggregationInfoResponse(info, telux::common::ErrorCode::SUCCESS);
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

    ret = app->requestCarrierAggregationCellInfo();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses and notifications */
    std::this_thread::sleep_for(std::chrono::seconds(3));

    app->deInit();

    std::cout << "\nCarrier Aggregation info app exiting" << std::endl;
    return 0;
}
