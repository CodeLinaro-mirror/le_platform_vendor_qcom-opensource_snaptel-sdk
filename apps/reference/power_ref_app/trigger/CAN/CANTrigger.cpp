/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
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

#include "CANTrigger.hpp"

std::shared_ptr<CANTrigger>  CANTrigger::canTrigger_ = nullptr;

CANTrigger::CANTrigger(std::shared_ptr<EventManager> eventManager) {
    LOG(DEBUG, __FUNCTION__);
    eventManager_ = eventManager;
}

CANTrigger::~CANTrigger() {
    LOG(DEBUG, __FUNCTION__);
    deRegisterCanListener();
}

bool CANTrigger::init() {
    LOG(DEBUG, __FUNCTION__);
    config_ =  ConfigParser::getInstance();
    loadTrigger();
    canWrapper_ = CanWrapper::getInstance();
    return registerCanListener();
}

void CANTrigger::deRegisterCanListener() {
    LOG(DEBUG, __FUNCTION__);
    for (auto trigger : triggers_) {
        if (trigger.second.second) {
            canWrapper_->unregisterListener(trigger.second.second);
        }
    }
}

bool CANTrigger::registerCanListener() {
    LOG(DEBUG, __FUNCTION__);
    bool status = true;
    for (auto trigger : triggers_) {
        if (trigger.first) {
            LOG(INFO, __FUNCTION__, " trigger id ", trigger.first);
            // registering for CAN trigger to receive notifications when new CAN messages arrive 
            RegistrationToken token = canWrapper_->registerListener(trigger.first, CwBase::MASK29,
                                            CANTrigger::triggerEvent, this, 0, CwBase::IFACE_ANY);
            if (token) {
                trigger.second.second = token;
                LOG(DEBUG, __FUNCTION__, " registered for  id ", trigger.first);
            } else {
                status = false;
                LOG(ERROR, __FUNCTION__, " unable to register for ", trigger.first);
            }
        } else {
            LOG(ERROR, __FUNCTION__, " could not register for ", trigger.first);
        }
    }
    return status;
}

void CANTrigger::onEventRejected(shared_ptr<Event> event, EventStatus reason) {
   LOG(DEBUG, __FUNCTION__, " ", event->toString());
}

void CANTrigger::onEventProcessed(shared_ptr<Event> event, bool success) {
   LOG(DEBUG, __FUNCTION__, " ", event->toString());
}

void CANTrigger::triggerEvent(CwFrame * pf, void* userData, int ifNo) {
    LOG(DEBUG, __FUNCTION__);
    CANTrigger* canTriggerPtr =  (CANTrigger*)userData;
    if(!canTriggerPtr) {
        LOG(ERROR, __FUNCTION__, " no can trigger instance available ");
        return;
    }
    LOG(DEBUG, __FUNCTION__, " received frame id = ", pf->getId());
    std::shared_ptr<Event> eventPtr;
    for (auto trigger : canTriggerPtr->triggers_) {
        // ignore identifier extension (IDE) bit of CAN frame ID
        LOG(DEBUG, __FUNCTION__, " compare with trigger id = ", trigger.first);
        if (trigger.first<<1 ==  pf->getId()<<1) {
            eventPtr = std::make_shared<Event>(trigger.second.first, TriggerType::CAN_TRIGGER);
            break;
        }
    }

    if (eventPtr) {
        if (canTriggerPtr->eventManager_) {
            canTriggerPtr->eventManager_->pushEvent(eventPtr);
        } else {
            LOG(ERROR, __FUNCTION__, "  event manager is not available ");
        }
    } else {
        LOG(ERROR, __FUNCTION__, " unable to create event");
    }
}


std::shared_ptr<CANTrigger> CANTrigger::getInstance(std::shared_ptr<EventManager> eventManager) {
    LOG(DEBUG, __FUNCTION__);
    if (!canTrigger_ && eventManager) {
        CANTrigger::canTrigger_ = *(new std::shared_ptr<CANTrigger>(new CANTrigger(eventManager)));
    }

    if (!CANTrigger::canTrigger_) {
        LOG(ERROR, __FUNCTION__, " failed to create CANTrigger instance");
    }
    return CANTrigger::canTrigger_;
}

bool CANTrigger::loadTrigger() {
   LOG(DEBUG, __FUNCTION__);
   uint32_t triggerSuspend, triggerResume, triggerShutdown;
   triggerSuspend = std::stoul(config_->getValue("CAN_TRIGGER", TRIGGER_SUSPEND), nullptr, 16);
   triggerResume = std::stoul(config_->getValue("CAN_TRIGGER", TRIGGER_RESUME), nullptr, 16);
   triggerShutdown = std::stoul(config_->getValue("CAN_TRIGGER", TRIGGER_SHUTDOWN), nullptr, 16);

   if (triggerSuspend == triggerResume || triggerSuspend == triggerShutdown ||
       triggerResume == triggerShutdown) {
      LOG(ERROR, __FUNCTION__, " Error : same trigger for multiple state");
      return false;
   }


   triggers_.insert({triggerSuspend, {TcuActivityState::SUSPEND, 0}});
   triggers_.insert({triggerResume, {TcuActivityState::RESUME, 0}});
   triggers_.insert({triggerShutdown, {TcuActivityState::SHUTDOWN, 0}});

   return true;
}