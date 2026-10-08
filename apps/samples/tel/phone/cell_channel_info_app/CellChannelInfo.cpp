/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/*
 * This application demonstrates how to request LTE/NR5G cell channel information. The steps are
 * as follows:
 *
 * 1. Get a PhoneFactory instance.
 * 2. Get a IPhoneManager instance from the PhoneFactory.
 * 3. Wait for the phone manager service to become available.
 * 4. Create a phone instance.
 * 5. Request cell channel information.
 * 6. Update the response.
 *
 * Usage:
 * # ./cell_channel_info_app
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

    int requestCellChannelInfo() {
        auto respCb = [this](telux::tel::CellChannelInfoList channelInfoList,
                          telux::common::ErrorCode errorCode) {
            onCellChannelInfoResponse(channelInfoList, errorCode);
        };

        telux::common::Status status;
        int phoneId = DEFAULT_PHONE_ID;
        if (phones_.empty()) {
            std::cout << "No phones available" << std::endl;
            return -ENODEV;
        }
        auto phone = phones_[phoneId - 1];
        /* Step - 5 */
        status = phone->requestCellChannelInfo(respCb);
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Can't request cell channel info, err " << static_cast<int>(status)
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

    /* Step - 6 */
    void onCellChannelInfoResponse(
        const telux::tel::CellChannelInfoList &channelInfoList, telux::common::ErrorCode error) {
        std::cout << std::endl;
        if (error != telux::common::ErrorCode::SUCCESS) {
            std::cout << "requestCellChannelInfo failed, errorCode: " << static_cast<int>(error)
                      << std::endl;
            return;
        }

        std::cout << "LTE Serving/SCell ARFCNs (" << channelInfoList.lteServingAndSCellArfcns.size()
                  << "):" << std::endl;
        for (size_t i = 0; i < channelInfoList.lteServingAndSCellArfcns.size(); i++) {
            const auto &arfcn = channelInfoList.lteServingAndSCellArfcns[i];
            std::cout << "  [" << i << "] DL EARFCN: " << arfcn.dlEarfcn
                      << ", UL EARFCN: " << arfcn.ulEarfcn
                      << ", Bandwidth: " << lteBandwidthToString(arfcn.bandwidth) << std::endl;
        }

        std::cout << "LTE Neighbor ARFCNs (" << channelInfoList.lteNeighborArfcns.size()
                  << "):" << std::endl;
        for (size_t i = 0; i < channelInfoList.lteNeighborArfcns.size(); i++) {
            const auto &arfcn = channelInfoList.lteNeighborArfcns[i];
            std::cout << "  [" << i << "] DL EARFCN: " << arfcn.dlEarfcn
                      << ", UL EARFCN: " << arfcn.ulEarfcn
                      << ", Bandwidth: " << lteBandwidthToString(arfcn.bandwidth) << std::endl;
        }

        std::cout << "NR5G Serving/SCell ARFCNs ("
                  << channelInfoList.nr5gServingAndSCellArfcns.size() << "):" << std::endl;
        for (size_t i = 0; i < channelInfoList.nr5gServingAndSCellArfcns.size(); i++) {
            std::cout << "  [" << i
                      << "] NR-ARFCN: " << channelInfoList.nr5gServingAndSCellArfcns[i].dlEarfcn
                      << std::endl;
        }

        std::cout << "NR5G Neighbor ARFCNs (" << channelInfoList.nr5gNeighborArfcns.size()
                  << "):" << std::endl;
        for (size_t i = 0; i < channelInfoList.nr5gNeighborArfcns.size(); i++) {
            std::cout << "  [" << i
                      << "] NR-ARFCN: " << channelInfoList.nr5gNeighborArfcns[i].dlEarfcn
                      << std::endl;
        }
    }

    /* Unsolicited notification triggered on cell channel info change */
    void onCellChannelInfoListChanged(
        int phoneId, const telux::tel::CellChannelInfoList channelInfoList) override {
        std::cout << "\nonCellChannelInfoListChanged, phoneId: " << phoneId << std::endl;
        onCellChannelInfoResponse(channelInfoList, telux::common::ErrorCode::SUCCESS);
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

    ret = app->requestCellChannelInfo();
    if (ret < 0) {
        return ret;
    }

    /* Wait for receiving all asynchronous responses and notifications */
    std::this_thread::sleep_for(std::chrono::seconds(3));

    app->deInit();

    std::cout << "\nCell channel info app exiting" << std::endl;
    return 0;
}
