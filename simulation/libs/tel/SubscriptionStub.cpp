/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SubscriptionStub.hpp"

namespace telux {

namespace tel {

SubscriptionStub::SubscriptionStub(int slotId, std::string carrierName, std::string iccId, int mcc,
    int mnc, std::string number, std::string imsi, std::string gid1, std::string gid2)
   : simSlotIndex_(slotId)
   , carrierName_(carrierName)
   , iccId_(iccId)
   , mcc_(mcc)
   , mnc_(mnc)
   , number_(number)
   , imsi_(imsi)
   , gid1_(gid1)
   , gid2_(gid2) {
    LOG(DEBUG, __FUNCTION__);
}

SubscriptionStub::~SubscriptionStub() {
    LOG(DEBUG, __FUNCTION__);
}
void SubscriptionStub::cleanup() {
    // reset data member to default
    carrierName_.clear();
    iccId_.clear();
    gid1_.clear();
    gid2_.clear();
    number_.clear();
    imsi_.clear();
    mcc_ = 0;
    mnc_ = 0;
}

std::string SubscriptionStub::getCarrierName() {
    LOG(DEBUG, __FUNCTION__, carrierName_);
    return carrierName_;
}

std::string SubscriptionStub::getIccId() {
    LOG(DEBUG, __FUNCTION__, iccId_);
    return iccId_;
}

int SubscriptionStub::getMcc() {
    LOG(DEBUG, __FUNCTION__, mcc_);
    return mcc_;
}

int SubscriptionStub::getMnc() {
    LOG(DEBUG, __FUNCTION__, mnc_);
    return mnc_;
}

std::string SubscriptionStub::getMobileCountryCode() {
    LOG(DEBUG, __FUNCTION__, mcc_);
    return std::to_string(mcc_);
}

std::string SubscriptionStub::getMobileNetworkCode() {
    LOG(DEBUG, __FUNCTION__, mnc_);
    return std::to_string(mnc_);
}

std::string SubscriptionStub::getPhoneNumber() {
    LOG(DEBUG, __FUNCTION__, number_);
    return number_;
}

int SubscriptionStub::getSlotId() {
    LOG(DEBUG, __FUNCTION__, simSlotIndex_);
    return simSlotIndex_;
}

std::string SubscriptionStub::getImsi() {
    LOG(DEBUG, __FUNCTION__, imsi_);
    return imsi_;
}
std::string SubscriptionStub::getGID1() {
    LOG(DEBUG, __FUNCTION__, gid1_);
    return gid1_;
}

std::string SubscriptionStub::getGID2() {
    LOG(DEBUG, __FUNCTION__, gid2_);
    return gid2_;
}

void SubscriptionStub::updateSubscription(int slotId, std::string carrierName, std::string iccId,
    int mcc, int mnc, std::string number, std::string imsi, std::string gid1, std::string gid2) {
    LOG(DEBUG, __FUNCTION__, slotId);
    simSlotIndex_ = slotId;
    carrierName_  = carrierName;
    iccId_        = iccId;
    mcc_          = mcc;
    mnc_          = mnc;
    number_       = number;
    imsi_         = imsi;
    gid1_         = gid1;
    gid2_         = gid2;

    LOG(DEBUG, __FUNCTION__, " Carrier name is ", carrierName_, " Phone number is ", number_,
        " iccid is ", iccId_, " mcc is ", mcc_, " mnc is ", mnc_, " imsi is ", imsi_, " gid1 is ",
        gid1_, "gid2 is ", gid2_);
}
}  // end of namespace tel

}  // end of namespace telux