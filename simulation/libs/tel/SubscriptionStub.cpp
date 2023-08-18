/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 * NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 * GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 * HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "SubscriptionStub.hpp"

namespace telux {

namespace tel {

SubscriptionStub::SubscriptionStub(int slotId)
    :stub_(PhoneService::NewStub(grpc::CreateChannel("localhost:8089",
    grpc::InsecureChannelCredentials()))) {
    LOG(DEBUG, __FUNCTION__);
    getSubscription(slotId);
}

SubscriptionStub::~SubscriptionStub() {
    LOG(DEBUG, __FUNCTION__);
}
void SubscriptionStub::cleanup() {
    // reset data member to default
    carrierName_.clear();
    countryISO_.clear();
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

telux::common::Status SubscriptionStub::getSubscription(int slotId) {
    LOG(DEBUG, __FUNCTION__, slotId);
    ::tel::GetSubscriptionRequest request;
    ::tel::Subscription response;
    ClientContext context;

    request.set_phone_id(slotId);

    grpc::Status status = stub_->GetSubscription(&context, request, &response);

    if (!status.ok()) {
        return telux::common::Status::FAILED;
    }
    simSlotIndex_ = slotId;
    carrierName_  = static_cast<std::string>(response.carrier_name());
    iccId_  = static_cast<std::string>(response.icc_id());
    mcc_  = static_cast<int>(response.mcc());
    mnc_  = static_cast<int>(response.mnc());
    number_  = static_cast<std::string>(response.phone_number());
    imsi_  = static_cast<std::string>(response.imsi());
    gid1_  = static_cast<std::string>(response.gid_1());
    gid2_  = static_cast<std::string>(response.gid_2());

    LOG(DEBUG, __FUNCTION__, " Carrier name is ",carrierName_
    ," Phone number is ",number_, " iccid is ", iccId_ , " mcc is ",
    mcc_ , " mnc is ",mnc_ , " imsi is ",imsi_, " gid1 is ",gid1_ , "gid2 is ",gid2_);

    return telux::common::Status::SUCCESS;
}
} // end of namespace tel

} // end of namespace telux