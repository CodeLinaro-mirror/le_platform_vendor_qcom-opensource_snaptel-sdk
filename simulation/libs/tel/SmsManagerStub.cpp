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

#include <string>
#include <future>
#include <exception>
#include <algorithm>
#include <bits/stdc++.h>
#include "SmsManagerStub.hpp"
#include "../common/SimulationConfigParser.hpp"

using namespace telux::common;
using namespace telux::tel;
using namespace std;

SmsManagerStub::SmsManagerStub(int phoneId, telux::common::InitResponseCb callback)
    :stub_(SmsService::NewStub(grpc::CreateChannel("localhost:8089",
    grpc::InsecureChannelCredentials()))) {
    LOG(DEBUG, __FUNCTION__);
    phoneId_ = phoneId;
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    auto f = std::async(std::launch::async,
        [this, callback]() {
            this->initSync(callback);
        }).share();
    taskQ_->add(f);
}

void SmsManagerStub::initSync(telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::tel::GetServiceStatusReply response;
    ::tel::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(phoneId_);

    stub_->InitService(&context, request, &response);

    telux::common::ServiceStatus cbStatus =
        static_cast<telux::common::ServiceStatus>(response.service_status());
    int cbDelay = static_cast<int>(response.delay());
    LOG(DEBUG, __FUNCTION__, " cbDelay::", cbDelay, " cbStatus::", static_cast<int>(cbStatus));
    if(callback) {
        this->invokeInitResponseCallback(cbDelay, cbStatus, callback);
    }
}

void SmsManagerStub::invokeInitResponseCallback(int cbDelay, telux::common::ServiceStatus cbStatus,
    telux::common::InitResponseCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(cbStatus);
    }
}

SmsManagerStub::~SmsManagerStub() {
    LOG(DEBUG, __FUNCTION__);
}

void SmsManagerStub::cleanup() {
    LOG(DEBUG, __FUNCTION__, " PhoneId: ", phoneId_);
    listeners_.clear();
    smsMessageMap_.clear();
}

telux::common::ServiceStatus SmsManagerStub::getServiceStatus() {
    LOG(DEBUG, __FUNCTION__);
    ::tel::GetServiceStatusReply response;
    ::tel::GetServiceStatusRequest request;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status status = stub_->GetServiceStatus(&context, request, &response);
    telux::common::ServiceStatus serviceStatus =
    static_cast<telux::common::ServiceStatus>(response.service_status());
    return serviceStatus;
}

telux::common::Status SmsManagerStub::registerListener(std::weak_ptr<ISmsListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    std::lock_guard<std::mutex> listenerLock(smsManagerMutex_);
    telux::common::Status status = telux::common::Status::FAILED;
    auto spt = listener.lock();
    if (spt != nullptr) {
        if (listeners_.size() == 0) {
            try {
            } catch(exception const & ex) {
                LOG(ERROR, __FUNCTION__, " Exception Occured: ", ex.what());
                status = telux::common::Status::NOMEMORY;
                return status;
            }
            auto &eventManager = telux::common::EventManager::getInstance();
            eventManager.connectToSimulationServer();
            eventManager.registerListener(shared_from_this(), TEL_SMS_FILTER);
        }
        bool existing = 0;
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                existing = 1;
                LOG(DEBUG, __FUNCTION__, "listener already exists");
                return telux::common::Status::ALREADY;
            }
        }
        if (existing == 0) {
            listeners_.emplace_back(listener);
            LOG(DEBUG, __FUNCTION__, " Register Listener : Adding");
            status = telux::common::Status::SUCCESS;
        }
    } else {
        LOG(ERROR, "Null listener");
        return telux::common::Status::INVALIDPARAM;
    }
    return status;
}

telux::common::Status SmsManagerStub::removeListener(
        std::weak_ptr<ISmsListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(smsManagerMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        for (auto iter=listeners_.begin(); iter<listeners_.end();++iter) {
            if (spt == (*iter).lock()) {
                iter = listeners_.erase(iter);
                LOG(DEBUG, __FUNCTION__, " Erasing listener");
                retVal = telux::common::Status::SUCCESS;
                break;
            }
        }
        if (listeners_.size() == 0) {
            auto &eventManager = telux::common::EventManager::getInstance();
            eventManager.deregisterListener(shared_from_this());
        }
    } else {
        LOG(WARNING, "listener is null");
        retVal = telux::common::Status::NOSUCH;
    }
    return (retVal);
}

telux::common::Status SmsManagerStub::sendSms(const std::string &message,
    const std::string &receiverAddress,
    std::shared_ptr<telux::common::ICommandResponseCallback> sentCallback,
    std::shared_ptr<telux::common::ICommandResponseCallback> deliveryCallback) {
    LOG(DEBUG, __FUNCTION__);
    bool isDeliveryReportNeeded = true;

    if (message.empty() || receiverAddress.empty()) {
      LOG(ERROR, __FUNCTION__, " Either message or receiver address is empty");
      return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
      LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
      return telux::common::Status::NOTREADY;
    }

    if (!sentCallback) {
      LOG(DEBUG, __FUNCTION__, " Sent callback is null");
    }
    if (!deliveryCallback) {
      LOG(DEBUG, __FUNCTION__, " Delivery callback is null");
      isDeliveryReportNeeded = false;
    }
    ::tel::SendSmsWithoutSmscRequest request;
    ::tel::SendSmsWithoutSmscReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->SendSmsWithoutSmsc(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::common::ErrorCode sentCallbackErrorcode =
    static_cast<telux::common::ErrorCode>(response.sentcallback_errorcode());
    int noofsegments = static_cast<int>(response.noofsegments());
    int sentCallbackDelay = static_cast<int>(response.sentcallback_callbackdelay());
    std::string ref = static_cast<std::string>(response.sentcallback_msgrefs());
    std::vector<int> refs = SmsHelper::convertStringToVector(ref);
    telux::common::ErrorCode deliveryCallbackErrorCode =
        static_cast<telux::common::ErrorCode>(response.deliverycallback_errorcode());
    int deliveryCallbackDelay = static_cast<int>(response.deliverycallback_callbackdelay());

    LOG(DEBUG, __FUNCTION__, " Invoking callback for old SMS API");
    // Sending the callback response.
    if (status == telux::common::Status::SUCCESS) {
        auto f1 = std::async(std::launch::async,
            [this, sentCallbackDelay, sentCallback, sentCallbackErrorcode]() {
                this->invokesendSmsCallback(sentCallbackDelay, sentCallback,
                    sentCallbackErrorcode);
            }).share();
            taskQ_->add(f1);
        // Send delivery report to listeners.
        if((isDeliveryReportNeeded) &&
            (sentCallbackErrorcode == telux::common::ErrorCode::SUCCESS)) {
            LOG(DEBUG, __FUNCTION__, " Invoking delivery report to listeners");
            auto f2 = std::async(std::launch::async,
            [this, receiverAddress, noofsegments, refs,
                deliveryCallbackErrorCode, deliveryCallbackDelay]() {
                this->invokeDeliveryReportListener(receiverAddress, noofsegments,
                refs, deliveryCallbackErrorCode, deliveryCallbackDelay );
            }).share();
            taskQ_->add(f2);

            // Sending the delivery callback Response.
            LOG(DEBUG, __FUNCTION__, " Invoking delivery callback");
            auto f3 = std::async(std::launch::async,
            [this, deliveryCallbackDelay, deliveryCallback, deliveryCallbackErrorCode]() {
                this->invokesendSmsCallback(deliveryCallbackDelay,
                    deliveryCallback, deliveryCallbackErrorCode);
            }).share();
            taskQ_->add(f3);
        }
    }
    return status;
}

telux::common::Status SmsManagerStub::sendSms(std::string message, std::string receiverAddress,
    bool deliveryReportNeeded, SmsResponseCb sentCallback, std::string smscAddr) {
    LOG(DEBUG, __FUNCTION__);
    if (message.empty() || receiverAddress.empty()) {
        LOG(ERROR, __FUNCTION__, " either message or receiver address is empty");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    if (!sentCallback) {
        LOG(DEBUG, __FUNCTION__, " Sent callback is null");
    }
    ::tel::SendSmsRequest request;
    ::tel::SendSmsReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->SendSms(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int noofsegments = static_cast<int>(response.noofsegments());
    telux::common::ErrorCode smsResponsecbErrorCode =
    static_cast<telux::common::ErrorCode>(response.smsresponsecb_errorcode());
    int smsResponseCbDelay = static_cast<int>(response.smsresponsecb_callbackdelay());
    std::string ref = static_cast<std::string>(response.sentcallback_msgrefs());
    std::vector<int> refs = SmsHelper::convertStringToVector(ref);
    std::vector<smsDeliveryInfo> infos;

    for (int i = 0; i < response.records_size(); i++) {
        smsDeliveryInfo info;
        info.errorCode  = static_cast<telux::common::ErrorCode>
            (response.mutable_records(i)->ondeliveryreport_errorcode());
        info.cbDelay  =  static_cast<int>
            (response.mutable_records(i)->deliverycallbackdelay());
        info.msgRef = static_cast<int>(response.mutable_records(i)->ondeliveryreportmsgref());
        LOG(DEBUG, __FUNCTION__, "errorCode " ,
            static_cast<int>(info.errorCode), "cbDelay ", info.cbDelay,
            "msgRef " ,info.msgRef);
        infos.emplace_back(info);
    }

    if (status == telux::common::Status::SUCCESS) {
        // Invoking response callback
        auto f1 = std::async(std::launch::async,
        [this, smsResponseCbDelay, smsResponsecbErrorCode, refs, sentCallback]() {
            this->invokeCallback(smsResponseCbDelay,
            smsResponsecbErrorCode, refs, sentCallback);
        }).share();
        taskQ_->add(f1);

        // Notifying listeners about the change event.
        if((deliveryReportNeeded) &&
            (smsResponsecbErrorCode == telux::common::ErrorCode::SUCCESS)) {
            auto f2 = std::async(std::launch::async,
                [this, receiverAddress, noofsegments, infos]() {
                    this->invokeDeliveryReportListener(receiverAddress, noofsegments,
                    infos);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}

void SmsManagerStub::invokeDeliveryReportListener(std::string receiverAddress,
    int noofdeliveryreport, std::vector<smsDeliveryInfo> infos) {
    LOG(DEBUG, __FUNCTION__);
    for (int i = 0; i < noofdeliveryreport ;i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(infos[i].cbDelay));
        for (auto iter=listeners_.begin();iter != listeners_.end();) {
            auto spt = (*iter).lock();
            if (spt) {
                spt->onDeliveryReport(phoneId_, infos[i].msgRef,
                    receiverAddress, infos[i].errorCode);
                ++iter;
            } else {
                iter = listeners_.erase(iter);
            }
        }
    }
}

void SmsManagerStub::invokeDeliveryReportListener(std::string receiverAddress,
    int noofdeliveryreport, std::vector<int> refs, telux::common::ErrorCode error,
    int deliveryCallbackDelay) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(deliveryCallbackDelay));
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt) {
            for (int i =0; i < noofdeliveryreport ;i++) {
                spt->onDeliveryReport(phoneId_, refs[i], receiverAddress, error);
            }
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

telux::common::Status SmsManagerStub::sendRawSms(const std::vector<PduBuffer> rawPdus,
    SmsResponseCb sentCallback) {
    LOG(DEBUG, __FUNCTION__);
    if (rawPdus.empty()) {
        LOG(ERROR, __FUNCTION__, " Raw PDU is empty");
        return telux::common::Status::INVALIDPARAM;
    }
    if (telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    if (!sentCallback) {
        LOG(DEBUG, __FUNCTION__, " Sent callback is null");
    }

    ::tel::SendRawSmsRequest request;
    ::tel::SendRawSmsReply response;
    int size = rawPdus.size();
    ClientContext context;

    request.set_phone_id(phoneId_);
    request.set_size(size);

    grpc::Status reqstatus = stub_->SendRawSms(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int noofsegments = size;
    std::string receiverAddress = static_cast<std::string>(response.reciever_address());
    telux::common::ErrorCode smsResponsecbErrorCode =
    static_cast<telux::common::ErrorCode>(response.smsresponsecb_errorcode());
    int smsResponseCbDelay = static_cast<int>(response.smsresponsecb_callbackdelay());
    std::string ref = static_cast<std::string>(response.sentcallback_msgrefs());
    std::vector<int> refs = SmsHelper::convertStringToVector(ref);
    std::vector<smsDeliveryInfo> infos;

    for (int i = 0; i < response.records_size(); i++) {
        smsDeliveryInfo info;
        info.errorCode  =  static_cast<telux::common::ErrorCode>
            (response.mutable_records(i)->ondeliveryreport_errorcode());
        info.cbDelay  =  static_cast<int>
            (response.mutable_records(i)->deliverycallbackdelay());
        info.msgRef = static_cast<int>
            (response.mutable_records(i)->ondeliveryreportmsgref());
        LOG(DEBUG, __FUNCTION__, "errorCode " ,
            static_cast<int>(info.errorCode), "cbDelay ", info.cbDelay,
            "msgRef " ,info.msgRef);
        infos.emplace_back(info);
    }

    if (status == telux::common::Status::SUCCESS) {
        // Invoking response callback
        auto f1 = std::async(std::launch::async,
        [this, smsResponseCbDelay, smsResponsecbErrorCode, refs, sentCallback]() {
            this->invokeCallback(smsResponseCbDelay,
                smsResponsecbErrorCode, refs, sentCallback);
        }).share();
        taskQ_->add(f1);

        // Notifying listeners about the change event.
        if(smsResponsecbErrorCode == telux::common::ErrorCode::SUCCESS) {
            auto f2 = std::async(std::launch::async,
                [this, receiverAddress, noofsegments, infos]() {
                    this->invokeDeliveryReportListener(receiverAddress, noofsegments,
                    infos);
                }).share();
            taskQ_->add(f2);
        }
    }
    return status;
}
void SmsManagerStub::invokeCallback(int cbDelay, ErrorCode error, std::vector<int> msgRefs,
     SmsResponseCb sentCallback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    if (sentCallback) {
        sentCallback(msgRefs,error);
    }
}

void SmsManagerStub::invokesendSmsCallback(int cbDelay,
    std::shared_ptr<telux::common::ICommandResponseCallback> callback,
    telux::common::ErrorCode error) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));
    auto f1 = std::async(std::launch::async,
        [this, callback, error]() {
            callback->commandResponse(error);
        }).share();
    taskQ_->add(f1);
}

telux::common::Status SmsManagerStub::requestSmscAddress
    (std::shared_ptr<ISmscAddressCallback> callback) {
    LOG(DEBUG, __FUNCTION__);
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::tel::GetSmscAddressRequest request;
    ::tel::GetSmscAddressReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetSmscAddress(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    std::string smscAddress = static_cast<std::string>(response.smsc_address());
    LOG(DEBUG, __FUNCTION__, "smscAddress is ", smscAddress );

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, cbDelay, callback, smscAddress, error]() {
                this->invokeGetSmscCallback(cbDelay, callback, smscAddress, error);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeGetSmscCallback(int cbDelay,
    std::shared_ptr<ISmscAddressCallback> callback, std::string smscAddress,
    telux::common::ErrorCode error) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback->smscAddressResponse(smscAddress, error);
    }
}

telux::common::Status SmsManagerStub::setSmscAddress(const std::string &smscAddress,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " SlotId : ", phoneId_);
    if(smscAddress.empty()) {
        LOG(ERROR, __FUNCTION__, "  smscAddress address is empty");
        return telux::common::Status::INVALIDPARAM;
    }
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::tel::SetSmscAddressRequest request;
    ::tel::SetSmscAddressReply response;
    ClientContext context;

    request.set_phone_id(phoneId_);
    request.set_number(smscAddress);

    grpc::Status reqstatus = stub_->SetSmscAddress(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            this->invokeResponseCallback(cbDelay, error, callback);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeResponseCallback(int cbDelay, telux::common::ErrorCode error,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(error);
    }
}

telux::common::Status SmsManagerStub::requestSmsMessageList(SmsTagType type,
    RequestSmsInfoListCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::tel::RequestSmsMessageListRequest request;
    ::tel::RequestSmsMessageListReply response;
    ClientContext context;
    ::tel::SmsTagType_TagType tag =  static_cast<::tel::SmsTagType_TagType>(type);
    request.set_phone_id(phoneId_);
    request.set_tag_type(tag);
    std::vector<SmsMetaInfo> infos;

    grpc::Status reqstatus = stub_->RequestSmsMessageList(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    for (int i = 0; i < response.meta_info_size(); i++) {
        telux::tel::SmsMetaInfo info;
        info.msgIndex  =  static_cast<int>(response.mutable_meta_info(i)->msg_index());
        info.tagType  =  static_cast<telux::tel::SmsTagType>
            (response.mutable_meta_info(i)->tag_type());
        LOG(DEBUG, __FUNCTION__, "msgIndex " ,info.msgIndex,
            "tagType ", static_cast<int>(info.tagType));
        infos.emplace_back(info);
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, infos ,error, callback, cbDelay]() {
                this->invokeRequestSmsInfoListCb(infos, error, callback, cbDelay);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeRequestSmsInfoListCb(std::vector<SmsMetaInfo> infos,
    telux::common::ErrorCode error, RequestSmsInfoListCb callback, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(infos, error);
    }
}

telux::common::Status SmsManagerStub::readMessage(uint32_t messageIndex,
    ReadSmsMessageCb callback) {
    LOG(DEBUG, __FUNCTION__);
    ::tel::ReadMessageRequest request;
    ::tel::ReadMessageReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_msg_index(messageIndex);

    grpc::Status reqstatus = stub_->ReadMessage(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    std::string text =  static_cast<std::string>((response.sms_message()).text());
    std::string sender  =  static_cast<std::string>((response.sms_message()).sender());
    std::string receiver  =  static_cast<std::string>((response.sms_message()).receiver());
    telux::tel::SmsEncoding encoding  =
        static_cast<telux::tel::SmsEncoding>((response.sms_message()).encoding());
    std::string pdu  =  static_cast<std::string>((response.sms_message()).pdu());
    std::vector <uint8_t> rawPdu;
    std::shared_ptr<MessagePartInfo> Info = nullptr;
    SmsMetaInfo metaInfo;
    const uint8_t* tmp = reinterpret_cast<const uint8_t*>(pdu.c_str());
    while (*tmp !='\0')
    {
        rawPdu.push_back(*tmp);
        tmp++;
    }

    int numberOfSegments  =  static_cast<int>(
            (response.sms_message()).messageinfono_of_segments());
    LOG(DEBUG, __FUNCTION__,"numberOfSegments", numberOfSegments);
    if(numberOfSegments > 1) {
        Info = std::make_shared< MessagePartInfo >();
        Info->refNumber  =  static_cast<int>((response.sms_message()).messageinfosegment_no());
        Info->numberOfSegments  =  numberOfSegments;
        Info->segmentNumber  =  static_cast<int>((response.sms_message()).messageinfosegment_no());
    }
    bool isMetaInfoValid  = static_cast<bool>(response.sms_message().ismetainfo_valid());
    metaInfo.tagType = static_cast<telux::tel::SmsTagType>(response.sms_message().tag_type());
    metaInfo.msgIndex = static_cast<int>((response.sms_message()).msg_index());

    SmsMessage msg
        (text, sender, receiver, encoding, pdu, rawPdu, Info, isMetaInfoValid, metaInfo);

    // Sending the Callback Response.
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f1 = std::async(std::launch::async,
            [this, msg, error, callback, cbDelay]() {
                this->invokeReadSmsMessageCb(msg, error, callback, cbDelay);
            }).share();
        taskQ_->add(f1);
    }
    return status;
}

void SmsManagerStub::invokeReadSmsMessageCb(SmsMessage message, telux::common::ErrorCode error,
    ReadSmsMessageCb callback, int cbDelay ) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(message, error);
    }
}

telux::common::Status SmsManagerStub::deleteMessage(DeleteInfo info,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " PhoneId: ", phoneId_, " MessageIndex: ", info.msgIndex,
        " Delete Type: ", static_cast<int>(info.delType), " SMS Tag Type: ",
        static_cast<int>(info.tagType));
    ::tel::DeleteMessageRequest request;
    ::tel::DeleteMessageRequestReply response;
    ::tel::SmsTagType_TagType tag =  static_cast<::tel::SmsTagType_TagType>(info.tagType);
    ::tel::DelType_DeleteType deltype =  static_cast<::tel::DelType_DeleteType>(info.delType);
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_msg_index(info.msgIndex);
    request.set_tag_type(tag);
    request.set_del_type(deltype);

    grpc::Status reqstatus = stub_->DeleteMessage(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }

    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());

    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            this->invokeResponseCallback(cbDelay, error, callback);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status SmsManagerStub::requestPreferredStorage(RequestPreferredStorageCb callback) {
    LOG(DEBUG, __FUNCTION__);
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::tel::RequestPreferredStorageRequest request;
    ::tel::RequestPreferredStorageReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestPreferredStorage(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    telux::tel::StorageType storageType =
        static_cast<telux::tel::StorageType>(response.storage_type());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
            [this, callback, storageType, cbDelay, error]() {
                this->invokeRequestPreferredStorageCb(storageType,cbDelay, error, callback);
            }).share();
        taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeRequestPreferredStorageCb(telux::tel::StorageType storageType,
    int cbDelay, telux::common::ErrorCode error, RequestPreferredStorageCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(storageType, error);
    }
}

telux::common::Status SmsManagerStub::setPreferredStorage(StorageType storageType,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " PhoneId : ", phoneId_);

    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::tel::SetPreferredStorageRequest request;
    ::tel::SetPreferredStorageReply response;
    ::tel::StorageType_Type type =  static_cast<::tel::StorageType_Type>(storageType);
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_storage_type(type);

    grpc::Status reqstatus = stub_->SetPreferredStorage(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            this->invokeResponseCallback(cbDelay, error, callback);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status SmsManagerStub::setTag(uint32_t msgIndex, SmsTagType tagType,
    telux::common::ResponseCallback callback) {
    LOG(DEBUG, __FUNCTION__, " PhoneId : ", phoneId_);
    if(telux::common::ServiceStatus::SERVICE_AVAILABLE != getServiceStatus()) {
        LOG(ERROR, __FUNCTION__, " SMS Manager is not ready");
        return telux::common::Status::NOTREADY;
    }
    ::tel::SetTagRequest request;
    ::tel::SetTagReply response;
    ::tel::SmsTagType_TagType tag =  static_cast<::tel::SmsTagType_TagType>(tagType);
    ClientContext context;
    request.set_phone_id(phoneId_);
    request.set_msg_index(msgIndex);
    request.set_tag_type(tag);

    grpc::Status reqstatus = stub_->SetTag(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    LOG(DEBUG, __FUNCTION__, "Status is ", static_cast<int>(status));
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
        auto f = std::async(std::launch::async,
        [this, cbDelay, error, callback]() {
            this->invokeResponseCallback(cbDelay, error, callback);
        }).share();
        taskQ_->add(f);
    }
    return status;
}

telux::common::Status SmsManagerStub::requestStorageDetails(RequestStorageDetailsCb callback) {

    LOG(DEBUG, __FUNCTION__);
    ::tel::RequestStorageDetailsRequest request;
    ::tel::RequestStorageDetailsReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->RequestStorageDetails(&context, request, &response);
    if (!reqstatus.ok()) {
        return telux::common::Status::FAILED;
    }
    telux::common::ErrorCode error = static_cast<telux::common::ErrorCode>(response.error());
    telux::common::Status status = static_cast<telux::common::Status>(response.status());
    int maxCount = response.max_count();
    int availableCount = response.available_count();
    bool isCallbackNeeded = static_cast<bool>(response.iscallback());
    int cbDelay = static_cast<int>(response.delay());
    if((status == telux::common::Status::SUCCESS) && (isCallbackNeeded)) {
    auto f = std::async(std::launch::async,
        [this, maxCount, availableCount, cbDelay, error, callback]() {
            this->invokeRequestStorageDetailsCb(maxCount, availableCount,
                cbDelay, error, callback);
        }).share();
    taskQ_->add(f);
    }
    return status;
}

void SmsManagerStub::invokeRequestStorageDetailsCb(int maxCount, int availableCount,
    int cbDelay, telux::common::ErrorCode error, RequestStorageDetailsCb callback) {
    LOG(DEBUG, __FUNCTION__);
    std::this_thread::sleep_for(std::chrono::milliseconds(cbDelay));

    if (callback) {
        callback(maxCount, availableCount, error);
    }
}

int SmsManagerStub::getPhoneId() {
    LOG(DEBUG, __FUNCTION__,"PhoneId is ",phoneId_);
    return phoneId_;
}

MessageAttributes SmsManagerStub::calculateMessageAttributes(const std::string &message) {
    LOG(DEBUG, __FUNCTION__);
    ::tel::GetMessageAttributesRequest request;
    ::tel::GetMessageAttributesReply response;
    ClientContext context;
    request.set_phone_id(phoneId_);

    grpc::Status reqstatus = stub_->GetMessageAttributes(&context, request, &response);
    MessageAttributes msg = {};
    msg.encoding  = static_cast<telux::tel::SmsEncoding>
        ((response.message_attribute()).encoding());
    msg.numberOfSegments = static_cast<int>((response.message_attribute()).number_of_segments());
    msg.segmentSize = static_cast<int>((response.message_attribute()).segment_size());
    msg.numberOfCharsLeftInLastSegment =
        static_cast<int>((response.message_attribute()).number_of_chars_left_in_last_segment());
    return msg;
}

/**
 * SmsMessage class to expose details to user application..
 */
SmsMessage::SmsMessage(std::string text, std::string sender, std::string receiver,
    SmsEncoding encoding, std::string pdu, PduBuffer rawPdu, std::shared_ptr<MessagePartInfo> info)
   : text_(text)
   , sender_(sender)
   , receiver_(receiver)
   , encoding_(encoding)
   , pdu_(pdu)
   , rawPdu_(rawPdu)
   , msgPartInfo_(info) {
}

SmsMessage::SmsMessage(std::string text, std::string sender, std::string receiver,
    SmsEncoding encoding, std::string pdu, PduBuffer rawPdu, std::shared_ptr<MessagePartInfo> info
    , bool isMetaInfoValid, SmsMetaInfo metaInfo)
   : text_(text)
   , sender_(sender)
   , receiver_(receiver)
   , encoding_(encoding)
   , pdu_(pdu)
   , rawPdu_(rawPdu)
   , msgPartInfo_(info)
   , isMetaInfoValid_(isMetaInfoValid)
   , metaInfo_(metaInfo) {
}

const std::string &SmsMessage::getText() const {
   return text_;
}

const std::string &SmsMessage::getSender() const {
   return sender_;
}

const std::string &SmsMessage::getReceiver() const {
   return receiver_;
}

SmsEncoding SmsMessage::getEncoding() const {
   return encoding_;
}

const std::string &SmsMessage::getPdu() const {
   return pdu_;
}

PduBuffer SmsMessage::getRawPdu() const {
    return rawPdu_;
}

std::shared_ptr<MessagePartInfo> SmsMessage::getMessagePartInfo() {
    return msgPartInfo_;
}

telux::common::Status SmsMessage::getMetaInfo(SmsMetaInfo &metaInfo) {

    if (isMetaInfoValid_) {
        metaInfo = metaInfo_;
        return telux::common::Status::SUCCESS;
    }
    return telux::common::Status::NOSUCH;
}

const std::string SmsMessage::toString() const {
   std::stringstream ss;
   ss << "Message: " << text_ << ", From: " << sender_ << ", To: " << receiver_;
   return ss.str();
}

void SmsManagerStub::onEventUpdate(std::string event) {
    std::string token;
    if (EVENT_FLAG == EventParserUtil::getNextToken(event, DEFAULT_DELIMITER)) {
        token = EventParserUtil::getNextToken(event, DEFAULT_DELIMITER);
        handleEvent(token, event);
    } else {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
    }
    return;
}

void SmsManagerStub::handleEvent(std::string token , std::string event) {
    LOG(DEBUG, __FUNCTION__, "The received event is: \"",token,"\"");
    if (token == "") {
        LOG(ERROR, __FUNCTION__, "The event flag is not set!");
        return;
    }
    LOG(DEBUG, __FUNCTION__, "The data event type is: ", token);
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", event);
    if (token == "memoryfull") {
        handleMemoryFullEvent(event);
    } else if (token == "incomingsms") {
        handleIncomingSms(event);
    }
}
void SmsManagerStub::handleIncomingSms(std::string eventParams) {
    LOG(DEBUG, __FUNCTION__);

    int phoneId;
    int numberOfSegments;
    int refNumber;
    int segmentNumber;
    int msgIndex;
    telux::tel::SmsTagType tagType = telux::tel::SmsTagType::UNKNOWN;
    telux::tel::SmsEncoding encoding;
    bool isMetaInfoValid;
    std::string pdu;
    std::string rawPdu;
    std::string receiver;
    std::string sender;
    std::string text;

    /* Fetch the slotId */

    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Fetch the numberOfSegments */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The numberOfSegments is not passed!");
    } else {
        try {
            numberOfSegments = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched numberofsegments is: ", numberOfSegments
        , "The leftover string is: ", eventParams);
    /* Fetch refNumber */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The refNumber not passed!");
    } else {
        try {
            refNumber = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched refNumber is: ", refNumber
        , "The leftover string is: ", eventParams);
    /* Fetch segmentNumber */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The segmentNumber is not passed!");
    } else {
        try {
            segmentNumber = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched segmentNumber is: ", segmentNumber
        , "The leftover string is: ", eventParams);

    /* Fetch msgIndex */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The msgIndex is: ", token);
    if (token == "") {
        LOG(INFO, "The msgIndex is not passed!");
    } else {
        try {
            msgIndex = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched msgIndex is: ", msgIndex
        , "The leftover string is: ", eventParams);

    /* Fetch tagType */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The tagType is not passed!");
    } else {
        tagType = SmsHelper::getTagType(token);
    }
    LOG(DEBUG, __FUNCTION__, "The fetched tagType is: ", static_cast<int>(tagType)
        , "The leftover string is: ", eventParams);

    /* Fetch encoding */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The encoding is not passed!");
    } else {
        encoding = SmsHelper::getencodingMethod(token);
    }
    LOG(DEBUG, __FUNCTION__, "The fetched encoding is: ", static_cast<int>(encoding)
        , "The leftover string is: ", eventParams);

    /* Fetch isMetaInfoValid */

   token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The isMetaInfoValid is not passed!");
    } else {
        try {
            int input = std::stoi(token);
            if (input == 1) {
                isMetaInfoValid = true ;
            } else {
                isMetaInfoValid = false ;
            }
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The fetched isMetaInfoValid is: ", isMetaInfoValid
        , "The leftover string is: ", eventParams);

    /* Fetch pdu */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The pdu is not passed!");
    } else {
        pdu = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched pdu is: ", pdu
        , "The leftover string is: ", token);

    /* Fetch rawPdu */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The rawPdu is not passed!");
    } else {
        rawPdu = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched rawPdu is: ", rawPdu
        , "The leftover string is: ", eventParams);

    const uint8_t* p = reinterpret_cast<const uint8_t*>(rawPdu.c_str());
    std::vector <uint8_t> pduBuffer;
    while (*p !='\0')
    {
        pduBuffer.push_back(*p);
        p++;
    }

    /* Fetch receiver */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The receiver is not passed!");
    } else {
        receiver = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched receiver is: ", receiver
        , "The leftover string is: ", eventParams);

   /* Fetch sender */

    token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    if (token == "") {
        LOG(INFO, "The sender is not passed!");
    } else {
        sender = token;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched sender is: ", sender
        , "The leftover string is: ", eventParams);

    /* Fetch text */
    LOG(DEBUG, __FUNCTION__, "The text is: ", eventParams);
    if (token == "") {
        LOG(INFO, "The text is not passed!");
    } else {
        text = eventParams;
    }
    LOG(DEBUG, __FUNCTION__, "The fetched text is: ", text );

    /* Construct sendMessage based on the inputs */
    std::shared_ptr<MessagePartInfo> info = std::make_shared< MessagePartInfo >();
    SmsMetaInfo metaInfo = {};
    info->refNumber = refNumber;
    info->numberOfSegments = numberOfSegments;
    info->segmentNumber = segmentNumber;

    metaInfo.msgIndex = msgIndex;
    metaInfo.tagType = tagType;
    SmsMessage msg(text, sender, receiver, encoding, pdu, pduBuffer,
        info, isMetaInfoValid, metaInfo);
    auto sharedPtr = std::make_shared<SmsMessage> (msg);
    // Invoke incomingSms notification to clients
    invokeIncomingSmslisteners(phoneId, sharedPtr);
    isMemoryFull(phoneId);

    // Consolidated message for all incomingSms segments and send the notification to clients
    if ((msg.getMessagePartInfo())->numberOfSegments > 1 ){
        parseAndConcatenateSmsMessage (phoneId, msg);
    }
}

void SmsManagerStub::isMemoryFull(int phoneId) {
    LOG(DEBUG, __FUNCTION__);

    ::tel::IsMemoryFullRequest request;
    ::tel::IsMemoryFullReply response;
    ClientContext context;
    request.set_phone_id(phoneId);

    grpc::Status reqstatus = stub_->IsMemoryFull(&context, request, &response);

    bool isMemoryFull = response.ismemoryfull();

    if(isMemoryFull) {
        invokeMemoryFulllisteners(phoneId, telux::tel::StorageType::SIM);
    }
}

void SmsManagerStub::invokeIncomingSmslisteners (int phoneId, std::shared_ptr<SmsMessage> message) {
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt) {
            spt->onIncomingSms(phoneId, message);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

void SmsManagerStub::invokeIncomingSmslisteners(int phoneId,
    std::shared_ptr<std::vector<SmsMessage>> messages) {
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt) {
            spt->onIncomingSms(phoneId_, messages);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}

void SmsManagerStub::parseAndConcatenateSmsMessage (int phoneId, SmsMessage& message) {
    LOG(DEBUG, __FUNCTION__);
    bool found = false;
    for ( auto &smsInfo : smsMessageMap_ ) {
        if (smsInfo.first.refNumber == ((message.getMessagePartInfo())->refNumber) &&
            smsInfo.first.senderAddress == message.getSender()) {
            LOG(DEBUG, __FUNCTION__, " Key with refNumber: ", (smsInfo.first).refNumber,
                " and senderAddress: ", smsInfo.first.senderAddress, " exists.");

            MessageMetaData metaData = smsInfo.first;
            std::vector<SmsMessage> smsInfos = smsMessageMap_[metaData];
            bool isPartAlreadyExist = false;
            int index = 0;
            for ( auto &smsInfoElement : smsInfos ) {
                if ((smsInfoElement.getMessagePartInfo())->segmentNumber ==
                    (message.getMessagePartInfo())->segmentNumber) {
                    LOG(DEBUG, __FUNCTION__, " Already part exist with segment no: ",
                        static_cast<int>((message.getMessagePartInfo())->segmentNumber));
                    isPartAlreadyExist = true;
                    break;
                }
                index++;
            }
            if (isPartAlreadyExist) {
                LOG(ERROR, __FUNCTION__,
                    " Duplicate or latest updated SMS info received at index: ", index);
                smsInfos[index] = message;
            } else {
                LOG(DEBUG, __FUNCTION__, " Add SMS info");
                smsInfos.emplace_back(message);
            }

            auto comp = [] ( SmsMessage& lhs,
                 SmsMessage& rhs )
                    {return (lhs.getMessagePartInfo())->segmentNumber <
                            (rhs.getMessagePartInfo())->segmentNumber;};
            sort(smsInfos.begin(), smsInfos.end(), comp);
            smsMessageMap_[metaData] = smsInfos;
            found = true;
            if (static_cast<int>(smsInfos.size())
                == (message.getMessagePartInfo())->numberOfSegments)  {
                LOG(DEBUG, __FUNCTION__, " All the parts of SMS is received");
                std::vector<SmsMessage> messages;
                for ( auto &smsInfoElement : smsInfos ) {
                    messages.emplace_back(smsInfoElement);
                }

                auto sharedPtr = std::make_shared<std::vector<SmsMessage>> (messages);
                invokeIncomingSmslisteners(phoneId, sharedPtr);
                smsMessageMap_.erase(metaData);
                return;
            }
            break;
        }
    }
        if(!found) {
            LOG(DEBUG, __FUNCTION__,
                " Key with refNumber: ", (message.getMessagePartInfo())->segmentNumber,
                " and senderAddress: ", message.getSender(), " does not exists.");
            MessageMetaData metaData;
            metaData.refNumber = (message.getMessagePartInfo())->segmentNumber;
            metaData.senderAddress = message.getSender();
            std::vector<SmsMessage> smsInfos;
            smsInfos.emplace_back(message);
            smsMessageMap_[metaData] = smsInfos;
        }
}
void SmsManagerStub::handleMemoryFullEvent(std::string eventParams) {

    LOG(DEBUG, __FUNCTION__);
    /* Fetch the slotId */
    int phoneId;
    std::string token = EventParserUtil::getNextToken(eventParams, DEFAULT_DELIMITER);
    LOG(DEBUG, __FUNCTION__, "The Slot id is: ", token);
    if(token == "") {
        LOG(INFO, __FUNCTION__, "The Slot id is not passed! Assuming default Slot Id");
        phoneId = 1;
    } else {
        try {
            phoneId = std::stoi(token);
        } catch(exception const & ex) {
            LOG(ERROR, __FUNCTION__, "Exception Occured: ", ex.what());
        }
    }
    LOG(DEBUG, __FUNCTION__, "The leftover string is: ", eventParams);

    /* Fetch Storage type */
    telux::tel::StorageType type;
    if (eventParams == "UNKNOWN") {
        type = telux::tel::StorageType::UNKNOWN;
    } else if (eventParams == "SIM") {
        type = telux::tel::StorageType::SIM;
    } else {
        type = telux::tel::StorageType::NONE;
    }
    LOG(DEBUG, __FUNCTION__, "The Storage type is : ", static_cast<int>(type));
    invokeMemoryFulllisteners(phoneId, type);
}

void SmsManagerStub::invokeMemoryFulllisteners(int phoneId, telux::tel::StorageType type) {
    LOG(DEBUG, __FUNCTION__);
    for (auto iter=listeners_.begin();iter != listeners_.end();) {
        auto spt = (*iter).lock();
        if (spt) {
            spt->onMemoryFull(phoneId, type);
            ++iter;
        } else {
            iter = listeners_.erase(iter);
        }
    }
}