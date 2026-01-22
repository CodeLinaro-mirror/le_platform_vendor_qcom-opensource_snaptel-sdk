/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       SubscriptionStub.hpp
 *
 * @brief      Implementation of ISubscription
 *
 */

#ifndef SUBSCRIPTION_STUB_HPP
#define SUBSCRIPTION_STUB_HPP

#include "common/Logger.hpp"
#include <telux/common/CommonDefines.hpp>
#include <telux/tel/Subscription.hpp>

namespace telux {
namespace tel {

class SubscriptionStub : public ISubscription {
 public:
    SubscriptionStub(int slotId, std::string carrierName, std::string iccId, int mcc, int mnc,
        std::string number, std::string imsi, std::string gid1, std::string gid2);
    ~SubscriptionStub();
    std::string getCarrierName() override;
    std::string getIccId() override;
    int getMcc() override;
    int getMnc() override;
    std::string getMobileCountryCode() override;
    std::string getMobileNetworkCode() override;
    std::string getPhoneNumber() override;
    int getSlotId() override;
    std::string getImsi() override;
    std::string getGID1() override;
    std::string getGID2() override;
    void updateSubscription(int slotId, std::string carrierName, std::string iccId, int mcc,
        int mnc, std::string number, std::string imsi, std::string gid1, std::string gid2);
    void cleanup();

 private:
    int simSlotIndex_;
    std::string carrierName_;
    std::string iccId_;
    int mcc_;
    int mnc_;
    std::string number_;
    std::string imsi_;
    std::string gid1_;
    std::string gid2_;
};

}  // end of namespace tel

}  // end of namespace telux

#endif  // SUBSCRIPTION_STUB_HPP