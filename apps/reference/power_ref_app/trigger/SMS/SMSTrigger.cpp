/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "SMSTrigger.hpp"
#include <telux/common/Log.hpp>
#include <future>
#include <telux/common/DeviceConfig.hpp>
#include <telux/tel/PhoneFactory.hpp>
#include "common/RefAppUtils.hpp"

SMSTrigger::SMSTrigger(std::shared_ptr<EventManager> eventManager) {
    LOGFD();
    eventManager_ = eventManager;
}

SMSTrigger::~SMSTrigger() {
    LOGFD();
    if (smsManager_) {
        smsManager_->removeListener(myself_);
        smsManager_ = nullptr;
    }
}

bool SMSTrigger::init() {
    LOGFD();

    config_ = ConfigParser::getInstance();
    loadConfig();
    auto &phoneFactory = telux::tel::PhoneFactory::getInstance();

    std::string configSlot = config_->getValue("SMS_TRIGGER", "SLOT_ID");
    int slotId             = stoi(configSlot);

    if (slotId == MAX_SLOT_ID && !telux::common::DeviceConfig::isMultiSimSupported()) {
        LOGFE("ERROR - multi sim support not available. slotId = %s", configSlot.c_str());
        return false;
    }

    std::promise<telux::common::ServiceStatus> prom;
    auto smsMgr = phoneFactory.getSmsManager(
        slotId, [&](telux::common::ServiceStatus status) { prom.set_value(status); });
    if (!smsMgr) {
        LOGFE("ERROR - Failed to get SMS Manager instance slotId = %d", slotId);
        return false;
    }
    myself_ = shared_from_this();
    LOGFD("Waiting for SMS Manager to be ready slotId = %d", slotId);
    telux::common::ServiceStatus smsMgrStatus = prom.get_future().get();
    if (smsMgrStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGFD("SMS Manager is ready slotId = %d", slotId);
        auto status = smsMgr->registerListener(myself_);
        if (status != telux::common::Status::SUCCESS) {
            LOGFE("ERROR - Failed to register listener slotId = %d", slotId);
            return false;
        }
        smsManager_ = smsMgr;
    } else {
        LOGFE("ERROR - Unable to initialize SMS Manager slotId = %d", slotId);
        return false;
    }

    return true;
}

void SMSTrigger::onIncomingSms(
    int phoneId, std::shared_ptr<std::vector<telux::tel::SmsMessage>> msgs) {
    eventManager_->holdWakeLock("SMSReceived");
    LOGFD("Consolidated Multipart Message:");
    std::string text                             = "";
    std::vector<telux::tel::SmsMessage> messages = *(msgs.get());
    LOGFD("Count: %zu", messages.size());

    for (telux::tel::SmsMessage smsMsg : messages) {
        text = text + smsMsg.getText();

        std::shared_ptr<telux::tel::MessagePartInfo> partInfo = smsMsg.getMessagePartInfo();
        if (partInfo) {
            std::string tmpLog = " mSegment: " + std::to_string(partInfo->segmentNumber)
                                 + "\n SMS Part on phone ID " + std::to_string(phoneId)
                                 + " from: " + smsMsg.getSender() + " to: " + smsMsg.getReceiver()
                                 + "\n Message Part: " + smsMsg.getText()
                                 + "\n PDU: " + smsMsg.getPdu()
                                 + "\n RefNumber:" + std::to_string(partInfo->refNumber)
                                 + " NumberOfSegments:" + std::to_string(partInfo->numberOfSegments)
                                 + " SegmentNumber: " + std::to_string(partInfo->segmentNumber);
            LOGFD("%s", tmpLog.c_str());
        }
    }
    LOGFD("Complete Message: %s", text.c_str());

#ifdef TELSDK_FEATURE_SATCOM_ENABLED
    // Send NTN Data
    auto sp = ntnClient_.lock();
    if (sp) {
        telux::common::Status ret = sp->sendDataString(text);
        LOGFD("sendData status = %d", static_cast<int>(ret));
    }
#endif

    std::async(std::launch::async, [this, text] {
        TcuActivityState tcuActivityState = TcuActivityState::UNKNOWN;
        std::string machineName           = ALL_MACHINES;
        if (validateTrigger(text, tcuActivityState, machineName)) {
            this->triggerEvent(tcuActivityState, machineName);
        }
    });
    eventManager_->releaseWakeLock("SMSReceived");
}

void SMSTrigger::onEventRejected(shared_ptr<Event> event, EventStatus reason) {
    LOGFD("%s", event->toString().c_str());
}

void SMSTrigger::onEventProcessed(shared_ptr<Event> event, bool success) {
    LOGFD("%s", event->toString().c_str());
}

void SMSTrigger::triggerEvent(TcuActivityState eventState, std::string machineName) {
    LOGFD();

    std::shared_ptr<Event> event
        = std::make_shared<Event>(eventState, machineName, TriggerType::SMS_TRIGGER);
    if (event) {
        if (eventManager_) {
            RefAppUtils::logKpiFile(event);
            eventManager_->pushEvent(event);
        } else {
            LOGFE("event manager is not available");
        }
    } else {
        LOGFE("unable to create event");
    }
}

bool SMSTrigger::validateTrigger(
    std::string text, TcuActivityState &tcuActivityState, std::string &machineName) {
    LOGFD("%s", text.c_str());
    // to avoid \n and \ in a string which might lead to not matching trigger text
    text.erase(std::remove(text.begin(), text.end(), '\n'), text.cend());
    text.erase(std::remove(text.begin(), text.end(), '\\'), text.cend());
    size_t deliminatorPosition = 0;
    if ((deliminatorPosition = text.find(MACHINE_NAME_DELIMINATOR)) != std::string::npos) {
        machineName
            = text.substr(deliminatorPosition + sizeof(MACHINE_NAME_DELIMINATOR), text.length());
        text = text.substr(0, deliminatorPosition);
    }

    if (triggerText_.find(text) == triggerText_.end()) {
        LOGFE("invalid trigger text, text = %s", text.c_str());
    } else {
        LOGFI(
            "valid trigger text, text = %s, machine name = %s", text.c_str(), machineName.c_str());
        tcuActivityState = triggerText_[text];
        return true;
    }
    return false;
}

bool SMSTrigger::loadConfig() {
    LOGFD();
    std::map<std::string, TcuActivityState> expectedTrigger{
        {TRIGGER_SUSPEND, TcuActivityState::SUSPEND}, {TRIGGER_RESUME, TcuActivityState::RESUME},
        {TRIGGER_SHUTDOWN, TcuActivityState::SHUTDOWN}};
    try {
        std::string configTriggerText = "";
        for (auto itr = expectedTrigger.begin(); itr != expectedTrigger.end(); ++itr) {
            configTriggerText = config_->getValue("SMS_TRIGGER", itr->first);
            if (!configTriggerText.empty()) {
                if (triggerText_.find(configTriggerText) != triggerText_.end()) {
                    LOGFE("Error : same trigger for multiple state");
                    return false;
                }
                triggerText_.insert({configTriggerText, itr->second});
            }
        }
    } catch (const std::invalid_argument &ia) {
        LOGFE("Error : invalid argument");
        return false;
    }
    return true;
}