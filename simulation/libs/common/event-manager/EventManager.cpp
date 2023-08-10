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

/**
 * @file       EventManager.cpp
 *
 * @brief      Implements the @ref EventManger class.
 *
 */

#include <algorithm>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

#include "EventManager.hpp"
#include "SimulationConfigParser.hpp"
#include "Logger.hpp"

#define UNSOLICITED_COMMON_EVENT "all"
#define LOCAL_HOST "127.0.0.1"
#define DEFAULT_PORT 8080
#define RETRY_TIMER 500
#define BUFFER_SIZE 100

namespace telux {
namespace common {

/* Event manager member function definitions */
EventManager::EventManager() {
    LOG(DEBUG, __FUNCTION__);
    LOG(DEBUG, " Initializing the EventManager");
    taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
    init();
}

EventManager::~EventManager() {
    LOG(DEBUG, __FUNCTION__);
    shutdown(clientSocket_, SHUT_RDWR);
    {
        std::lock_guard<std::mutex> lock(exitingMutex_);
        exiting_ = true;
    }
    config_ = nullptr;
    taskQ_ = nullptr;
    listeners_.clear();
    close(clientSocket_);
}

EventManager &EventManager::getInstance() {
    LOG(DEBUG, __FUNCTION__);
    static EventManager instance;
    return instance;
}

/**
 * This method filters the incoming events based on the subsystem type and manager. Based on the
 * filtering results, it either notifies that listener or ignores the notification.
 */
void EventManager::handleEventNotifications(std::string message) {
    LOG(DEBUG, __FUNCTION__);
    EventMessage parsedMessage;
    EventParserUtil::parseEventHeaderOptions(message, parsedMessage);

    std::string filter = parsedMessage.filter;
    std::lock_guard<std::mutex> lk(listenerMutex_);
    if (filter == UNSOLICITED_COMMON_EVENT) {
       LOG(DEBUG, __FUNCTION__, " passing common event::", parsedMessage.event);
       //passing the unsolicited common event to all the listeners
       for (auto it = listeners_.begin(); it != listeners_.end(); ++it) {
            for (auto &listener: it->second) {
                auto sp = listener.lock();
            if (sp) {
                sp->onEventUpdate(parsedMessage.event);
                }
            }
        }
    } else {
        LOG(DEBUG, __FUNCTION__, " passing event::", parsedMessage.event);
        //passing the unsolicited event to the listener who subscribed for it
        auto &eventListeners = listeners_[filter];
        for (const auto& listener: eventListeners) {
            auto sp = listener.lock();
        if (sp !=  nullptr) {
            sp->onEventUpdate(parsedMessage.event);
        } else {
            LOG(DEBUG, "erased obsolete weak pointer from EventManager listeners");
                const auto eventItr =
                std::find_if(eventListeners.begin(), eventListeners.end(),
                    [sp](const std::weak_ptr<IEventListener>& obj) {
                    return sp == obj.lock();
                    });

                if (eventItr != eventListeners.end()) {
                    eventListeners.erase(eventItr);
                }
            }
        }
    }
}

void EventManager::makeConnection() {
    LOG(DEBUG, __FUNCTION__);
    struct sockaddr_in address = {0};
    char buffer[BUFFER_SIZE] = {0};

    string portString = config_->getValue("PORT");
    int port = DEFAULT_PORT;
    if (portString != "") {
        port = stoi(portString);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    address.sin_port = htons(port);

    while(true) {
        {
            std::lock_guard<std::mutex> lock(exitingMutex_);
            if (exiting_) {
                break;
            }
        }

        if ((clientSocket_ = socket(AF_INET, SOCK_STREAM,0)) <= 0) {
            LOG(DEBUG, "error Creating socket");
        }
        LOG(INFO, "socket created::", clientSocket_);
        //Attempting to reconnect with EventInjector
        while(connect(clientSocket_, (struct sockaddr *)
                            &address, sizeof(address)) != 0) {
            {
                std::lock_guard<std::mutex> lock(exitingMutex_);
                if (exiting_) {
                    break;
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_TIMER));
        }

        LOG(DEBUG, "connected to SimulationServer");
        while (read(clientSocket_, buffer, BUFFER_SIZE) > 0) {
            std::string readStr(buffer);
            LOG(DEBUG, "received event::", readStr);

            auto f = std::async(std::launch::async, [this, readStr]() {
                this->handleEventNotifications(readStr);
            }).share();
            taskQ_->add(f);

            memset(buffer, 0, BUFFER_SIZE * (sizeof buffer[0]));
            {
                std::lock_guard<std::mutex> lock(exitingMutex_);
                if (exiting_) {
                    break;
                }
            }
        }
        LOG(DEBUG, "SimulationServer disconnected");
        close(clientSocket_);
    }
}

void EventManager::init() {
    LOG(DEBUG, __FUNCTION__);
    config_ = std::make_shared<SimulationConfigParser>();
    auto f = std::async(std::launch::async, [this]() {
        this->makeConnection();
    }).share();
    taskQ_->add(f);
}

telux::common::Status EventManager::registerListener(
        std::weak_ptr<IEventListener> listener, std::string filter) {
    LOG(DEBUG, __FUNCTION__);
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    auto spt = listener.lock();
    if (spt != nullptr) {
        if (listeners_.find(filter) == listeners_.end()) {
            listeners_[filter].push_back(listener);
            LOG(DEBUG, "Registering Listener");
        }
    }
    else {
        LOG(ERROR, "Failed to register");
        return telux::common::Status::FAILED;
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status EventManager::deregisterListener(
        std::weak_ptr<IEventListener> listener) {
    LOG(DEBUG, __FUNCTION__);
    telux::common::Status retVal = telux::common::Status::FAILED;
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    auto spt = listener.lock();

    if (spt != nullptr) {
        for (auto it = listeners_.begin(); it != listeners_.end(); it++) {
            auto &eventListeners = it->second;
            const auto eventItr =
            std::find_if(eventListeners.begin(), eventListeners.end(),
                [spt](const std::weak_ptr<IEventListener>& obj) {
                return spt == obj.lock();
        });

            if (eventItr != eventListeners.end()) {
                eventListeners.erase(eventItr);
            }
        }

        LOG(DEBUG, "In deRegister removed listener");
        retVal = telux::common::Status::SUCCESS;
    }
    return retVal;
}

} // end of namespace common

} // end of namespace telux
