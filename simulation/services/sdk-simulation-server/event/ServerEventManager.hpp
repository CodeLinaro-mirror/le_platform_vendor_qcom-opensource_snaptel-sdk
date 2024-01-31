/*
 *  Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SERVER_EVENT_MANAGER_HPP
#define SERVER_EVENT_MANAGER_HPP

#include <iostream>
#include <string>
#include <set>
#include <vector>
#include <unordered_map>
#include <telux/common/CommonDefines.hpp>

#include "protos/proto-src/event_simulation.grpc.pb.h"

class IServerEventListener {
public:
    /**
     * @brief This API is to receive the events, broadcasted by EventManager
     * locally to all the managers on server side.
     * The events triggered from event_injector are in string format &
     * has to converted to google::protobuf::Any type by vertical specific server Impl.
     *
     * @param event - A string depicting the event.
     */
    virtual void onEventUpdate(::eventService::UnsolicitedEvent event) {}
    virtual ~IServerEventListener() {}
};

class ServerEventManager {
    /**
    * @brief This class acts as the event manager on the server side.
    * It is responsible for broadcasting the event locally to vertical specific
    * services.
    */
public:
    static ServerEventManager &getInstance();

    telux::common::Status registerListener(
        std::weak_ptr<IServerEventListener> listener,
        std::vector<std::string> filter);

    telux::common::Status deregisterListener(
        std::weak_ptr<IServerEventListener> listener,
        std::vector<std::string> filter);

    telux::common::Status registerListener(
        std::weak_ptr<IServerEventListener> listener, std::string filter);

    telux::common::Status deregisterListener(
        std::weak_ptr<IServerEventListener> listener, std::string filter);

    void handleEventNotifications(::eventService::UnsolicitedEvent message);

private:
    ServerEventManager();
    ~ServerEventManager();

    void updateApiResponse(std::string message);

    std::unordered_map<std::string,
        std::set<std::weak_ptr<IServerEventListener>,
        std::owner_less<std::weak_ptr<IServerEventListener>>>> listeners_;
    std::mutex listenerMutex_;
};

#endif // SERVER_EVENT_MANAGER_HPP
