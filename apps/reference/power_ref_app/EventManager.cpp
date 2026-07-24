/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "EventManager.hpp"
#include <algorithm>
#include "common/RefAppUtils.hpp"

EventManager *EventManager::instance = nullptr;

EventManager *EventManager::getInstance() {
    LOGFD();
    if (!instance) {
        instance = new EventManager();
    }
    return instance;
}

bool EventManager::init() {
    LOGFD();
    bool initSucceed = true;
    //  Get the ConnectionFactory instances.
    auto &powerFactory = telux::power::PowerFactory::getInstance();

    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();

    ClientInstanceConfig config;
    config.clientName  = DAEMON_NAME + std::to_string(getpid());
    config.clientType  = ClientType::MASTER;
    config.machineName = ALL_MACHINES;
    tcuActivityStateMgr_
        = powerFactory.getTcuActivityManager(config, [&](telux::common::ServiceStatus status) {
              LOGFD("Init Callback called");
              prom.set_value(status);
          });
    if (tcuActivityStateMgr_ == nullptr) {
        LOGFE("ERROR - Failed to get manager instance");
        initSucceed = false;
    }

    // Wait for TCU-activity manager to be ready
    LOGFD("Waiting for TCU Activity Manager to be ready");
    telux::common::ServiceStatus serviceStatus = prom.get_future().get();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        LOGFD("TCU-activity manager is ready");

        // considering during boot up system state will be resume
        tcuActivityStateMgr_->setActivityState(
            TcuActivityState::RESUME, ALL_MACHINES, [this](ErrorCode errorCode) {
                if (errorCode == telux::common::ErrorCode::SUCCESS) {
                    LOGD("Setting resume in beginning Command initiated successfully");
                } else {
                    LOGE("Setting resume in beginning Command failed !!!");
                }
            });

        tcuActivityStateMgr_->getMachineName(localMachineName_);
    } else {
        LOGFE("Failed to initialize TCU-activity manager");
        initSucceed = false;
    }
    if (initSucceed) {
        initSucceed = registerTcuActivityManager();
    }
    return initSucceed;
}

EventManager::EventManager() {
    LOGFD();
}

EventManager::~EventManager() {
    LOGFD();
}

void EventManager::cleanup() {
    LOGFD();
    std::lock_guard<std::mutex> lk(cleanup_);
    if (tcuActivityStateMgr_)
        tcuActivityStateMgr_.reset();
    eventQueue_.clear();
}

void EventManager::notifyPreProcessEvent(shared_ptr<Event> event) {
    LOGFD(" event = %s", event->toString().c_str());

    // TriggerType
    for (std::vector<weak_ptr<IEventListener>>::iterator it
         = eventListeners_[TriggerType::UNKNOWN].begin();
         it != eventListeners_[TriggerType::UNKNOWN].end(); ++it) {
        if (std::shared_ptr<IEventListener> eventListener = (*it).lock()) {
            eventListener->preProcessEvent(event);
        }
    }

    for (std::vector<weak_ptr<IEventListener>>::iterator it
         = eventListeners_[event->getTriggerType()].begin();
         it != eventListeners_[event->getTriggerType()].end(); ++it) {
        if (std::shared_ptr<IEventListener> eventListener = (*it).lock()) {
            eventListener->preProcessEvent(event);
        }
    }
}

void EventManager::notifyOnEventRejected(shared_ptr<Event> event, EventStatus status) {
    LOGFD("status = %d event = %s", (int)status, event->toString().c_str());

    event->setEventStatus(status);
    // TriggerType
    for (std::vector<weak_ptr<IEventListener>>::iterator it
         = eventListeners_[TriggerType::UNKNOWN].begin();
         it != eventListeners_[TriggerType::UNKNOWN].end(); ++it) {
        if (std::shared_ptr<IEventListener> eventListener = (*it).lock()) {
            eventListener->onEventRejected(event, status);
        }
    }

    for (std::vector<weak_ptr<IEventListener>>::iterator it
         = eventListeners_[event->getTriggerType()].begin();
         it != eventListeners_[event->getTriggerType()].end(); ++it) {
        if (std::shared_ptr<IEventListener> eventListener = (*it).lock()) {
            eventListener->onEventRejected(event, status);
        }
    }
}

void EventManager::notifyAndEraseEventProcessed(
    TriggerType triggerType, TcuActivityState triggeredState, bool succeed, EventStatus status) {
    LOGFD();
    // notify  and erase duplicate event
    for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin();
         it != eventQueue_.end();) {
        if ((*it)->getTriggeredState() == triggeredState) {
            shared_ptr<Event> sameEventInQueue = *it;

            sameEventInQueue->setEventStatus(status);
            LOGFD("removing event id = %d", (int)sameEventInQueue->getId());
            for (std::vector<weak_ptr<IEventListener>>::iterator itl
                 = eventListeners_[TriggerType::UNKNOWN].begin();
                 itl != eventListeners_[TriggerType::UNKNOWN].end(); ++itl) {
                if (std::shared_ptr<IEventListener> eventListener = (*itl).lock()) {
                    eventListener->onEventProcessed(sameEventInQueue, succeed);
                }
            }
            for (std::vector<weak_ptr<IEventListener>>::iterator itl
                 = eventListeners_[triggerType].begin();
                 itl != eventListeners_[triggerType].end(); ++itl) {
                if (std::shared_ptr<IEventListener> eventListener = (*itl).lock()) {
                    eventListener->onEventProcessed(sameEventInQueue, succeed);
                }
            }
            it = eventQueue_.erase(it);
        } else {
            break;
        }
    }

    LOGFD("end");
}

void EventManager::updateEventStatus(
    shared_ptr<Event> event, bool removeFromQueue, bool succeed, EventStatus status) {
    LOGFD("event = %d, status = %d, remove from queue = %d succeed = %d", (int)event->getId(),
        (int)status, (int)removeFromQueue, (int)succeed);
    if (removeFromQueue) {
        notifyAndEraseEventProcessed(
            event->getTriggerType(), event->getTriggeredState(), succeed, status);
    } else {
        // will update failure cases
        if (!succeed) {
            notifyOnEventRejected(event, status);
        }
    }
}

void EventManager::printQueue() {
    LOGFD();
    for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin(); it != eventQueue_.end();
         ++it) {
        LOGFD("event = %s", (*it)->toString().c_str());
    }
}

// event management
void EventManager::pushEvent(shared_ptr<Event> event) {
    LOGFD("event = %s", event->toString().c_str());

    LOGFD("local machine name: %s local state: %s incoming machine name: %s incoming state: %s",
        localMachineName_.c_str(), RefAppUtils::tcuActivityStateToString(localState_).c_str(),
        event->getMachineName().c_str(),
        RefAppUtils::tcuActivityStateToString(event->getTriggeredState()).c_str());
    if ((localMachineName_.compare(event->getMachineName()) == 0) || event->getMachineName().empty()
        || (event->getMachineName().compare(ALL_MACHINES) == 0)
        || (event->getMachineName().compare(LOCAL_MACHINE))) {
        localState_ = event->getTriggeredState();
    }

    std::lock_guard<std::mutex> lk(eventQueueUpdate_);
    printQueue();
    TcuActivityState newState = event->getTriggeredState();
    do {
        std::vector<std::string> machineNames;
        // check if provided valid machine name
        if (tcuActivityStateMgr_->getAllMachineNames(machineNames)
            == telux::common::Status::SUCCESS) {
            auto it = std::find(machineNames.begin(), machineNames.end(), event->getMachineName());
            if (it == machineNames.end() && event->getMachineName() != ALL_MACHINES) {
                LOGFE("unable to find given machine name");
                updateEventStatus(event, false, false, EventStatus::REJECTED_INVALID_MACHINE_NAME);

                break;
            }
        } else {
            LOGFE("unable to get available machine names");
        }

        if (!eventQueue_.empty()) {
            // consider 1st event is in progress if eventQueue_ size more then 1
            TcuActivityState inProgressTrigger = eventQueue_[0]->getTriggeredState();
            // check event in progress for event overriden
            for (std::deque<shared_ptr<Event>>::iterator it = eventQueue_.begin();
                 it != eventQueue_.end();) {
                if ((*it)->getTriggeredState() != newState
                    && (*it)->getTriggeredState() != inProgressTrigger) {

                    shared_ptr<Event> overridenEvent = *it;
                    LOGFE(
                        "REJECTED_EVENT_OVERRIDDEN event = %s", overridenEvent->toString().c_str());

                    it = eventQueue_.erase(it);
                    updateEventStatus(
                        overridenEvent, false, false, EventStatus::REJECTED_EVENT_OVERRIDDEN);
                } else {
                    ++it;
                }
            }
            event->setEventStatus(EventStatus::IN_QUEUE);
            eventQueue_.push_back(event);
        } else {

            if (tcuActivityStateMgr_->getServiceStatus()
                != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
                // tcu activity manager down
                LOGFE("tcu activity state manager down");
                updateEventStatus(event, false, false, EventStatus::FAILED_TCU_ACTIVITY);
                break;
            }
            // Hold the wake lock temporarily to avoid the device getting suspended automatically
            // while processing the event
            holdWakeLock();
            eventQueue_.push_back(event);
            setActivityState(event);
        }
    } while (0);
}

void EventManager::setActivityState(shared_ptr<Event> event) {
    LOGFD();
    notifyPreProcessEvent(event);
    tcuActivityStateMgr_->setActivityState(
        event->getTriggeredState(), event->getMachineName(), [event, this](ErrorCode errorCode) {
            if (errorCode != telux::common::ErrorCode::SUCCESS) {
                LOGFE("Command failed !!!");
                processedEventHandler(EventStatus::FAILED_TCU_ACTIVITY);
            } else {
                LOGFD("Command initiated successfully");
                if (event->getTriggeredState() == TcuActivityState::RESUME) {
                    // Acknowledgment message (onSlaveAckStatusUpdate) is not expected for resume.
                    processedEventHandler(EventStatus::SUCCEED);
                } else {
                    event->setEventStatus(EventStatus::IN_PROGRESS_TCU_ACTIVITY);
                }
            }
        });
}

// Event listener
void EventManager::registerListener(
    weak_ptr<IEventListener> eventListener, TriggerType triggerType) {
    LOGFD("TriggerType = %d", (int)triggerType);
    if (eventListeners_.find(triggerType) == eventListeners_.end()) {
        std::vector<weak_ptr<IEventListener>> temp;
        temp.push_back(eventListener);
        eventListeners_.insert({triggerType, temp});
    } else {
        eventListeners_[triggerType].push_back(eventListener);
    }
}

void EventManager::writeToSystemNode(char *nodepath, char *value, int length) {
    int fd = open(nodepath, O_WRONLY | O_APPEND | O_NONBLOCK);
    if (fd < 0) {
        LOGFE("Opening of node failed!!! nodepath: %s errno: %s", nodepath, strerror(errno));
    } else {
        if (write(fd, value, length) == -1) {
            LOGFE("Writing to node failed value: %s nodepath: %s errno: %s", value, nodepath,
                strerror(errno));
        } else {
            LOGFD("Writing to node success value: %s nodepath: %s", value, nodepath);
        }
        close(fd);
    }
}

void EventManager::holdWakeLock() {
    writeToSystemNode((char *)WAKELOCK_PATH, (char *)WAKE_LOCK, strlen(WAKE_LOCK));
    LOGFD();
}

void EventManager::holdWakeLock(const std::string &wakeLockValue) {
    writeToSystemNode((char *)WAKELOCK_PATH, (char *)wakeLockValue.c_str(), wakeLockValue.length());
    LOGFD();
}

void EventManager::releaseWakeLock() {
    LOGFD();
    writeToSystemNode((char *)WAKEUNLOCK_PATH, (char *)WAKE_LOCK, strlen(WAKE_LOCK));
}
void EventManager::releaseWakeLock(const std::string &wakeLockValue) {
    LOGFD();
    writeToSystemNode(
        (char *)WAKEUNLOCK_PATH, (char *)wakeLockValue.c_str(), wakeLockValue.length());
}

void EventManager::processedEventHandler(EventStatus status) {
    LOGFD("status = %s", RefAppUtils::eventStatusToString(status).c_str());
    std::lock_guard<std::mutex> lk(eventQueueUpdate_);
    printQueue();

    // Note: even in case of timeout or other error, the master is proceeding with state change
    bool isEventExecutionSucceed = true;
    if (!eventQueue_.empty()) {
        // check event in progress
        shared_ptr<Event> processedEvent
            = *((std::deque<shared_ptr<Event>>::iterator)eventQueue_.begin());

        // check for the latest event
        if (eventQueue_.back()->getTriggeredState() != processedEvent->getTriggeredState()) {
            LOGFE("found conflict with latest event");
            updateEventStatus(processedEvent, true, false, EventStatus::REJECTED_EVENT_OVERRIDDEN);
            isEventExecutionSucceed = false;
        } else {
            updateEventStatus(processedEvent, true, isEventExecutionSucceed, status);
        }

        // keep processing the next event
        LOGFD("check next event");
        if (!eventQueue_.empty()) {
            shared_ptr<Event> nextEvent
                = *((std::deque<shared_ptr<Event>>::iterator)eventQueue_.begin());
            LOGFD("execute next event. event = %s", nextEvent->toString().c_str());
            setActivityState(nextEvent);
        } else {
            // after processing all event in queue remove temporary wake lock
            releaseWakeLock();
        }
    } else {
        LOGFE("eventQueue is empty");
    }
}

void EventManager::onSlaveAckStatusUpdate(const telux::common::Status status,
    const std::string machineName, const std::vector<ClientInfo> unresponsiveClients,
    const std::vector<ClientInfo> nackResponseClients) {
    LOGFD();
    EventStatus eventStatus = EventStatus::FAILED_TCU_ACTIVITY;
    if (status == telux::common::Status::SUCCESS) {
        LOGFD("Slave applications successfully acknowledged the state transition");
        eventStatus = EventStatus::SUCCEED;
    } else if (status == telux::common::Status::EXPIRED) {
        LOGFE("Timeout occured while waiting for acknowledgements from slave applications");
        eventStatus = EventStatus::FAILED_TCU_ACTIVITY_TIMEOUT;
    } else {
        LOGFE("Failed to receive acknowledgements from slave applications");
        eventStatus = EventStatus::FAILED_TCU_ACTIVITY;
    }

    if (unresponsiveClients.size() > 0) {
        LOGFE("Number of unresponsive clients : %zu", unresponsiveClients.size());
        for (size_t i = 0; i < unresponsiveClients.size(); i++) {
            LOGFE("client name : %s, machine name : %s", unresponsiveClients[i].first.c_str(),
                unresponsiveClients[i].second.c_str());
        }
    }

    if (nackResponseClients.size() > 0) {
        LOGFE("Number of clients responded with nack : %zu", nackResponseClients.size());
        for (size_t i = 0; i < nackResponseClients.size(); i++) {
            LOGFE("client name : %s, machine name : %s", nackResponseClients[i].first.c_str(),
                nackResponseClients[i].second.c_str());
        }
    }

    processedEventHandler(eventStatus);
}

void EventManager::onServiceStatusChange(telux::common::ServiceStatus status) {
    LOGFD("Service Status : %s", RefAppUtils::serviceStatusToString(status).c_str());
}

bool EventManager::registerTcuActivityManager() {
    LOGFD();
    // Registering a listener for TCU-activity state updates
    // weak_ptr<ITcuActivityListener> ptr(instance);
    telux::common::Status status = tcuActivityStateMgr_->registerListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        LOGFE("ERROR - Failed to register for TCU-activity state updates");
        return false;
    } else {
        LOGFD("Registered Listener for TCU-activity state updates");
    }
    // Registering a listener for TCU-activity management service status updates
    status = tcuActivityStateMgr_->registerServiceStateListener(shared_from_this());
    if (status != telux::common::Status::SUCCESS) {
        LOGFE("ERROR - Failed to register for Service status updates");
        return false;
    }
    return true;
}

void EventManager::onTcuActivityStateUpdate(TcuActivityState state, std::string machineName) {
    // The master is not expected to get this indication, as the master is the one who triggers the
    // state change. Other concerned slave clients will get this indication, and it is expected that
    // the slave acknowledges this indication via (sendActivityStateAck).
    LOGFE("%s", RefAppUtils::tcuActivityStateToString(state).c_str());
}

TcuActivityState EventManager::getActivityState() {
    LOGFE("%s", RefAppUtils::tcuActivityStateToString(localState_).c_str());
    return localState_;
}
