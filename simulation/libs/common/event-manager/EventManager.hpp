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
 * @file       EventManager.hpp
 *
 * @brief      Declares the EventManger class that handles notifications of Asynchronous Device
 *             events.
 *
 */

#ifndef EVENT_MANAGER_HPP
#define EVENT_MANAGER_HPP

#include <unordered_map>
#include <string>
#include <memory>
#include <mutex>
#include <vector>
#include "EventParserUtil.hpp"
#include "../AsyncTaskQueue.hpp"

class SimulationConfigParser;

namespace telux {
namespace common {

class IEventListener {
public:
    /**
     * @brief This class encapsulates the base event Listener class
     *
     * @param event - A string parameter depicting the event type.
     */
    virtual void onEventUpdate(std::string event) {}
    virtual ~IEventListener() {}
};

class EventManager {
    /**
     * @brief This class defines APIs and manages the unsolicited events that can be notified to
     *        the SDK.
     *
     */
public:
    static EventManager &getInstance();
    void connectToSimulationServer();
    void handleEventNotifications(std::string msg);

    telux::common::Status registerListener(std::weak_ptr<IEventListener> listener,
        std::string filter);
    telux::common::Status deregisterListener(std::weak_ptr<IEventListener> listener);
private:
    EventManager();
    virtual ~EventManager();

    void makeConnection();

    bool exiting_ = false;
    int clientSocket_;
    std::mutex listenerMutex_;
    std::mutex exitingMutex_;
    std::unordered_map<std::string, std::vector<std::weak_ptr<IEventListener>>> listeners_;
    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    std::shared_ptr<SimulationConfigParser> config_;
};

} // end of namespace common

} // end of namespace telux

#endif  // EVENT_MANAGER_HPP