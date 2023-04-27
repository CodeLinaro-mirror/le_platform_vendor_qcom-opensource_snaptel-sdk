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

#include "EventManager.hpp"

EventManager *EventManager::instance = nullptr;

EventManager *EventManager::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    if (!instance) {
        instance = new EventManager();
    }
    return instance;
}

bool EventManager::init() {
    LOG(DEBUG, __FUNCTION__);
    bool initSucceed = true;
    //  Get the ConnectionFactory instances.
    auto &powerFactory = telux::power::PowerFactory::getInstance();

    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
    tcuActivityStateMgr_ =
        powerFactory.getTcuActivityManager(ClientType::MASTER, ProcType::LOCAL_PROC,
                                           [&](telux::common::ServiceStatus status) {
                                               LOG(DEBUG, __FUNCTION__, " Init Callback called ");
                                               prom.set_value(status);
                                           });
    if (tcuActivityStateMgr_ == nullptr) {
        LOG(ERROR, __FUNCTION__, " ERROR - Failed to get manager instance");
        initSucceed = false;
    }

    // Wait for TCU-activity manager to be ready
    LOG(DEBUG, __FUNCTION__, "  Waiting for TCU Activity Manager to be ready ");
    telux::common::ServiceStatus serviceStatus = prom.get_future().get();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOG(DEBUG, __FUNCTION__, " TCU-activity manager is ready");

        // considering during boot up system state will be resume
        tcuActivityStateMgr_->setActivityState(
            TcuActivityState::RESUME, [this](ErrorCode errorCode) {
            if (errorCode == telux::common::ErrorCode::SUCCESS) {
                LOG(DEBUG, " Setting resume in beginning Command initiated successfully " );
            } else {
                LOG(ERROR,  " Setting resume in beginning Command failed !!!"  );
            }
        });
    } else {
        LOG(ERROR, __FUNCTION__, " Failed to initialize TCU-activity manager");
        initSucceed = false;
    }
    if (initSucceed) {
        initSucceed = registerTcuActivityManager();
    }
    return initSucceed;
}

EventManager::EventManager() {
    LOG(DEBUG, __FUNCTION__);
    // hold wake lock to avoid device getting to suspend
    holdWakeLock();
}
EventManager::~EventManager() {
    LOG(DEBUG, __FUNCTION__);
    tcuActivityStateMgr_.reset();

    for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin();
         it != eventQueue_.end();) {
            eventQueue_.erase(it);
    }
    // service should release the resource that is being held
    releaseWakeLock();
}

void EventManager::notifyOnEventRejected(shared_ptr<Event> event, EventStatus status) {
    LOG(DEBUG, __FUNCTION__, " status  = ", (int)status, " event = ", event->toString());

    event->setEventStatus(status);
    // TriggerType
    for (std::vector<weak_ptr<IEventListener>>::iterator it =
             eventListeners_[TriggerType::UNKNOWN].begin();
         it != eventListeners_[TriggerType::UNKNOWN].end(); ++it) {
        if (std::shared_ptr<IEventListener> eventListener = (*it).lock()) {
            eventListener->onEventRejected(event, status);
        }
    }

    for (std::vector<weak_ptr<IEventListener>>::iterator it =
             eventListeners_[event->getTriggerType()].begin();
         it != eventListeners_[event->getTriggerType()].end(); ++it) {
        if (std::shared_ptr<IEventListener> eventListener = (*it).lock()) {
            eventListener->onEventRejected(event, status);
        }
    }
}

void EventManager::notifyAndEraseEventProcessed(TriggerType triggerType,
                                                TcuActivityState triggeredState, bool success,
                                                EventStatus status) {
    LOG(DEBUG, __FUNCTION__);
    // notify  and erase duplicate event
    for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin();
         it != eventQueue_.end();) {
        if ((*it)->getTriggeredState() == triggeredState) {
            shared_ptr<Event> sameEventInQueue = *it;

            sameEventInQueue->setEventStatus(status);
            LOG(DEBUG, __FUNCTION__, "  removing event id = ", (int)sameEventInQueue->getId());
            for (std::vector<weak_ptr<IEventListener>>::iterator itl =
                     eventListeners_[TriggerType::UNKNOWN].begin();
                 itl != eventListeners_[TriggerType::UNKNOWN].end(); ++itl) {
                if (std::shared_ptr<IEventListener> eventListener = (*itl).lock()) {
                    eventListener->onEventProcessed(sameEventInQueue, success);
                }
            }
            for (std::vector<weak_ptr<IEventListener>>::iterator itl =
                     eventListeners_[triggerType].begin();
                 itl != eventListeners_[triggerType].end(); ++itl) {
                if (std::shared_ptr<IEventListener> eventListener = (*itl).lock()) {
                    eventListener->onEventProcessed(sameEventInQueue, success);
                }
            }
            eventQueue_.erase(it);
        } else {
            break;
        }
    }

    LOG(DEBUG, __FUNCTION__, " end");
}

void EventManager::updateEventStatus(shared_ptr<Event> event,
                                     bool processed, bool succeed, EventStatus status) {
    LOG(DEBUG, __FUNCTION__, "  event = ", (int)event->getId(), " ,status = ", (int)status,
        ", processed = ", (int)processed, " succeed = ", (int)succeed);
    if (processed) {
        notifyAndEraseEventProcessed(event->getTriggerType(), event->getTriggeredState(), succeed,
                                     status);
    } else {
        // will update failure cases
        if (status == EventStatus::REJECTED_INVALID_STATE_TRANSITION ||
            status == EventStatus::REJECTED_EVENT_OVERRIDDEN) {
            notifyOnEventRejected(event, status);
        }
    }
}

void EventManager::printQueue() {
    LOG(DEBUG, __FUNCTION__);
    for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin(); it != eventQueue_.end();
         ++it) {
        LOG(DEBUG, __FUNCTION__, " event = ", (*it)->toString());
    }
}

// event management
void EventManager::pushEvent(shared_ptr<Event> event) {
    LOG(DEBUG, __FUNCTION__, " event = ", event->toString());
    std::lock_guard<std::mutex> lk(eventQueueUpdate_);
    printQueue();
    TcuActivityState newState = event->getTriggeredState();
    do {
        if (!eventQueue_.empty()) {
            // consider 1st event is in progress if eventQueue_ size more then 1
            TcuActivityState inProgressTrigger = eventQueue_[0]->getTriggeredState();
            // check event in progress for event overriden
            for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin();
                it != eventQueue_.end();) {
                if ((*it)->getTriggeredState() != newState &&
                    (*it)->getTriggeredState() != inProgressTrigger) {

                    shared_ptr<Event> overridenEvent = *it;
                    LOG(ERROR, __FUNCTION__,
                        " REJECTED_EVENT_OVERRIDDEN  event = ", overridenEvent->toString());

                    eventQueue_.erase(it);
                    updateEventStatus(overridenEvent, false, false,
                                    EventStatus::REJECTED_EVENT_OVERRIDDEN);
                } else {
                    ++it;
                }
            }
            updateEventStatus(event, false, false, EventStatus::IN_QUEUE);
            eventQueue_.push_back(event);
        } else {
            TcuActivityState currentState = tcuActivityStateMgr_->getActivityState();
            if (currentState == TcuActivityState::UNKNOWN &&
                tcuActivityStateMgr_->getServiceStatus() !=
                    telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                // tcu activity manager down
                LOG(ERROR, __FUNCTION__, " tcu activity state manager down ");
                updateEventStatus(event, false, false, EventStatus::FAILED_TCU_ACTIVITY);
                break;
            }
            LOG(DEBUG, __FUNCTION__, " currentState = ",
                RefAppUtils::tcuActivityStateToString(currentState), " triggered state = ",
                RefAppUtils::tcuActivityStateToString(newState));
            // check the existing state of the device to avoid an invalid state transition
            if (currentState == newState) {
                LOG(ERROR, __FUNCTION__, " REJECTED_INVALID_STATE_TRANSITION ");
                updateEventStatus(event, false, false,
                                        EventStatus::REJECTED_INVALID_STATE_TRANSITION);
            } else {
                // hold wake lock to avoid the device getting suspended before processing a new
                // event
                holdWakeLock();
                LOG(DEBUG, __FUNCTION__, " setActivityState ");
                eventQueue_.push_back(event);
                setActivityState(event);
            }
        }
    }while (0);
}

void EventManager::setActivityState(shared_ptr<Event> event) {
    LOG(DEBUG, __FUNCTION__);
    TcuActivityState trigger = event->getTriggeredState();

    tcuActivityStateMgr_->setActivityState(trigger, [trigger, event, this](ErrorCode errorCode) {
        if (errorCode == telux::common::ErrorCode::SUCCESS) {
            LOG(DEBUG, __FUNCTION__,  " Command initiated successfully " );
            updateEventStatus(event, false, false, EventStatus::IN_PROGRESS_TCU_ACTIVITY );
        } else {
            LOG(ERROR, __FUNCTION__,  " Command failed !!!"  );
            executeEvent(EventStatus::FAILED_TCU_ACTIVITY);
        }
    });
}

// Event listener
void EventManager::registerListener(weak_ptr<IEventListener> eventListener,
                                    TriggerType triggerType) {
    LOG(DEBUG, __FUNCTION__, " TriggerType = ", (int)triggerType);
    if (eventListeners_.find(triggerType) == eventListeners_.end()) {
        std::vector<weak_ptr<IEventListener>> temp;
        temp.push_back(eventListener);
        eventListeners_.insert({triggerType, temp});
    } else {
        eventListeners_[triggerType].push_back(eventListener);
    }
}

void EventManager::writeToSystemNode(char *nodepath, char *value, int length) {
    LOG(DEBUG, __FUNCTION__);
    int fd = -1;
    LOG(DEBUG, __FUNCTION__, " About to open node ", string(nodepath));
    string logTmp;
    fd = open(nodepath, O_WRONLY | O_APPEND | O_NONBLOCK);
    if (fd < 0) {
        logTmp = " Opening of  node failed!!! err " + string(nodepath) +
                 " errno = " + string(strerror(errno));
        LOG(ERROR, __FUNCTION__, logTmp);
    } else {
        LOG(DEBUG, __FUNCTION__, " Opening of  node success ", string(nodepath));
        int returnValueWrite = write(fd, value, length);
        if (returnValueWrite == -1) {
            logTmp = " Writing of  to  node failed err "+ string(value) +
                     " "+string(nodepath) + " errno = " + string(strerror(errno));
            LOG(ERROR, __FUNCTION__, logTmp);
        } else {
            logTmp = " Writing of  to  node success " + string(value) +
                     " " + string(nodepath);
            LOG(DEBUG, __FUNCTION__, logTmp);
        }
    }
    close(fd);
}

void EventManager::holdWakeLock() {
    LOG(DEBUG, __FUNCTION__);
    writeToSystemNode((char *)WAKELOCK_PATH, (char *)DAEMON_NAME, strlen(DAEMON_NAME));
}

void EventManager::releaseWakeLock() {
    LOG(DEBUG, __FUNCTION__);
    writeToSystemNode((char *)AUTOSLEEP_NODE_PATH, (char *)AUTOSLEEP_NODE_MEM,
                            strlen(AUTOSLEEP_NODE_MEM));
    writeToSystemNode((char *)WAKEUNLOCK_PATH, (char *)DAEMON_NAME, strlen(DAEMON_NAME));
}

void EventManager::executeEvent(EventStatus status) {
    LOG(DEBUG, __FUNCTION__, " status = ", RefAppUtils::eventStatusToString(status));
    std::lock_guard<std::mutex> lk(eventQueueUpdate_);
    printQueue();

    bool isEventExecutionSucceed = false;
    // Note: even in case of timeout we considering to suspend
    if (status == EventStatus::SUCCEED || status == EventStatus::FAILED_TCU_ACTIVITY_TIMEOUT) {
        isEventExecutionSucceed = true;
    }
    if (!eventQueue_.empty()) {
        // check event in progress
        std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin();
        shared_ptr<Event> processedEvent = *it;
        TcuActivityState processedState = processedEvent->getTriggeredState();

        //check for latest event
        if (eventQueue_.back()->getTriggeredState() != processedEvent->getTriggeredState()) {
            LOG(ERROR, __FUNCTION__, " found conflict with latest event");
            updateEventStatus(processedEvent, true, false, EventStatus::REJECTED_EVENT_OVERRIDDEN);
        } else {
            updateEventStatus(processedEvent, true, isEventExecutionSucceed, status);
        }

        // keep processing next event
        LOG(DEBUG, __FUNCTION__, " check next event ");
        if (!eventQueue_.empty()) {
            std::deque<shared_ptr<Event>>::iterator itl = eventQueue_.begin();
            shared_ptr<Event> nextEvent = *itl;
            LOG(DEBUG, __FUNCTION__, " execute next event. event = ", nextEvent->toString());
            setActivityState(nextEvent);
        } else {
            if (isEventExecutionSucceed) {
                if (processedState == TcuActivityState::SUSPEND) {
                    releaseWakeLock();
                } else if (processedState == TcuActivityState::SHUTDOWN) {
                    int returnValue = system("/sbin/shutdown -hP now");
                    if ( returnValue !=0 ) {
                        LOG(ERROR, __FUNCTION__, " failed to execute shutdown returnValue = "
                            , returnValue);
                    }
                }
            } else {
                if (processedState == TcuActivityState::RESUME) {
                    // While system is in SUSPEND state, if a RESUME trigger is received, we need to
                    // hold the wake-lock to prevent system from entering SUSPEND before RESUME is
                    // processed
                    releaseWakeLock();
                }
            }
        }
    } else {
        LOG(ERROR, __FUNCTION__, "  eventQueue is empty");
    }
}

void EventManager::onSlaveAckStatusUpdate(telux::common::Status status) {
    LOG(DEBUG, __FUNCTION__);
    if (status == telux::common::Status::SUCCESS) {
        LOG(DEBUG, __FUNCTION__, " Slave applications successfully acknowledged the state",
            " transition");
        executeEvent(EventStatus::SUCCEED);
    } else if (status == telux::common::Status::EXPIRED) {
        LOG(ERROR, __FUNCTION__, " Timeout occured while waiting for acknowledgements from slave",
            " applications");
        executeEvent(EventStatus::FAILED_TCU_ACTIVITY_TIMEOUT);
    } else {
        LOG(ERROR, __FUNCTION__, " Failed to receive acknowledgements from slave applications");
        executeEvent(EventStatus::FAILED_TCU_ACTIVITY);
    }
}

void EventManager::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOG(DEBUG, __FUNCTION__, " Service Status : ", RefAppUtils::serviceStatusToString(status));
}

bool EventManager::registerTcuActivityManager() {
    LOG(DEBUG, __FUNCTION__);
    // Registering a listener for TCU-activity state updates
    // weak_ptr<ITcuActivityListener> ptr(instance);
    telux::common::Status status = tcuActivityStateMgr_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " ERROR - Failed to register for TCU-activity state updates");
        return false;
    } else {
        LOG(DEBUG, __FUNCTION__, " Registered Listener for TCU-activity state updates");
    }
    // Registering a listener for TCU-activity management service status updates
    status = tcuActivityStateMgr_->registerServiceStateListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " ERROR - Failed to register for Service status updates");
        return false;
    }
    return true;
}

void EventManager::onTcuActivityStateUpdate(TcuActivityState state) {
    LOG(DEBUG, __FUNCTION__, " ", RefAppUtils::tcuActivityStateToString(state));
    if (state == TcuActivityState::SUSPEND) {
        Status ackStatus = tcuActivityStateMgr_->sendActivityStateAck(
            TcuActivityStateAck::SUSPEND_ACK);
        if (ackStatus == Status::SUCCESS) {
            LOG(DEBUG, __FUNCTION__, " Sent SUSPEND acknowledgement");
        } else {
            LOG(ERROR, __FUNCTION__, " Failed to send SUSPEND acknowledgement !");
        }
    } else if (state == TcuActivityState::SHUTDOWN) {
        Status ackStatus = tcuActivityStateMgr_->sendActivityStateAck(
            TcuActivityStateAck::SHUTDOWN_ACK);
        if (ackStatus == Status::SUCCESS) {
            LOG(DEBUG, __FUNCTION__, " Sent SHUTDOWN acknowledgement");
        } else {
            LOG(ERROR, __FUNCTION__, " Failed to send SHUTDOWN acknowledgement !");
        }
    } else if (state == TcuActivityState::RESUME) {
        LOG(DEBUG, __FUNCTION__, "  RESUME Success !");
        executeEvent(EventStatus::SUCCEED);
    }

}
