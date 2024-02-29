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
#include <set>
#include <algorithm>
#include <unistd.h>
#include <grpcpp/grpcpp.h>

#include "EventParserUtil.hpp"
#include "AsyncTaskQueue.hpp"
#include "protos/proto-src/event.grpc.pb.h"
#include "Logger.hpp"
#include "CommonUtils.hpp"

using grpc::Channel;
using grpc::ClientContext;
using grpc::ClientReader;
using grpc::Status;

#define UNSOLICITED_COMMON_EVENT "all"
#define DEFAULT_DELAY 100

namespace telux {
namespace common {

class IEventListener {
public:
    /**
     * @brief This API is to receive the events, broadcasted by EventManager
     * locally to all the managers on libs side.
     * The events triggered from simulation_server are of type google::protobuf::Any.
     *
     * @param event - A google::protobuf::Any parameter depicting the event.
     */
    virtual void onEventUpdate(google::protobuf::Any event) {}

    virtual ~IEventListener() {}
};

template<typename T>
class EventManager {
    /**
     * @brief This class defines APIs and manages the unsolicited events that can be notified to
     *        the SDK.
     *
     */
protected:
    EventManager(std::launch policy = std::launch::async)
    : policy_(policy) {
        LOG(DEBUG, __FUNCTION__);
        LOG(DEBUG, " Initializing the EventManager");
        taskQ_ = std::make_shared<AsyncTaskQueue<void>>();
        stub_ = CommonUtils::getGrpcStub<T>();
        connectToSimulationServer();
    }

    virtual ~EventManager() {
        LOG(DEBUG, __FUNCTION__);
        {
            std::lock_guard<std::mutex> lock(exitingMutex_);
            exiting_ = true;
        }
        cleanup();
        getClientContext()->TryCancel();
        taskQ_ = nullptr;
        listeners_.clear();
        clearClientContext();
    }

    void connectToSimulationServer() {
        LOG(DEBUG, __FUNCTION__);

        if (!connectedToSimulationServer_) {
            auto f = std::async(std::launch::async, [this]() {
                isEventServiceAvailable();
            }).share();
            taskQ_->add(f);
        }
    }

    std::unique_ptr<typename T::Stub> stub_;
public:
    /**
    * This overloaded method filters the incoming events from simualtion server. Based on the
    * filtering results, it either notifies that listener or ignores the notification.
    */
    void handleEventNotifications(::eventService::EventResponse message) {
        LOG(DEBUG, __FUNCTION__);

        std::string filter = message.filter();
        std::lock_guard<std::mutex> lk(listenerMutex_);
        if (filter == UNSOLICITED_COMMON_EVENT) {
            LOG(DEBUG, __FUNCTION__, " passing common event");
            //passing the unsolicited common event to all the listeners
            for (auto it = listeners_.begin(); it != listeners_.end(); ++it) {
                for (auto &listener: it->second) {
                    auto sp = listener.lock();
                    if (sp) {
                        sp->onEventUpdate(message.any());
                    }
                }
            }
        } else {
            LOG(DEBUG, __FUNCTION__, " passing unsolicited event::", message.filter());
            //passing the unsolicited event to the listener who subscribed for it
            auto &eventListeners = listeners_[filter];
            for (auto it = eventListeners.begin(); it != eventListeners.end();) {
                auto sp = (*it).lock();
                if (sp) {
                    sp->onEventUpdate(message.any());
                } else {
                    LOG(DEBUG, "erased obsolete weak pointer from EventManager listeners");
                    it = eventListeners.erase(it);
                    continue;
                }
                ++it;
            }
        }
    }

    telux::common::Status registerListener(std::weak_ptr<IEventListener> listener,
        std::vector<std::string> filters) {

        LOG(DEBUG, __FUNCTION__);
        telux::common::Status status = telux::common::Status::SUCCESS;
        for (auto &x: filters) {
            status = registerListener(listener, x);

            if (status != telux::common::Status::SUCCESS)
                break;
        }

        return status;
    }

    telux::common::Status deregisterListener(std::weak_ptr<IEventListener> listener,
        std::vector<std::string> filters) {
        LOG(DEBUG, __FUNCTION__);
        telux::common::Status status = telux::common::Status::SUCCESS;
        for (auto &x: filters) {
            status = deregisterListener(listener, x);

            if (status != telux::common::Status::SUCCESS)
                break;
        }

        return status;
    }

    telux::common::Status registerListener(
        std::weak_ptr<IEventListener> listener, std::string filter) {

        LOG(DEBUG, __FUNCTION__);
        std::lock_guard<std::mutex> listenerLock(listenerMutex_);
        auto spt = listener.lock();
        bool updateFilter = false;
        if (spt != nullptr) {
            if (listeners_.find(filter) == listeners_.end()) {
                updateFilter = true;
            }

            const auto listenersItr =
                find_if(listeners_[filter].begin(), listeners_[filter].end(),
                        [spt](const std::weak_ptr<IEventListener>& wp){
                        return (spt == wp.lock());});
            if (listenersItr != listeners_[filter].end()) {
                return telux::common::Status::ALREADY;
            }

            listeners_[filter].insert(listener);
            if (updateFilter) {
                LOG(DEBUG, "Registering Listener for filter: ", filter);
                this->updateFilters();
            }
        }
        else {
            LOG(ERROR, "Failed to register");
            return telux::common::Status::FAILED;
        }
        return telux::common::Status::SUCCESS;
    }

    telux::common::Status deregisterListener(
        std::weak_ptr<IEventListener> listener, std::string filter) {

        LOG(DEBUG, __FUNCTION__);
        telux::common::Status retVal = telux::common::Status::FAILED;
        std::lock_guard<std::mutex> listenerLock(listenerMutex_);
        auto spt = listener.lock();

        if (spt != nullptr) {
            auto &eventListeners = listeners_[filter];
            auto eventItr = eventListeners.find(listener);
            if (eventItr != eventListeners.end()) {
                eventListeners.erase(eventItr);
                if (eventListeners.size() == 0) {
                    this->updateFilters();
                }
            }

            LOG(DEBUG, "In deRegister removed listener");
            retVal = telux::common::Status::SUCCESS;
        }
        return retVal;
    }

private:
    /**
     * @brief This API make sure that, we request the stream initialization only if
     *  server is available.
     */
    void isEventServiceAvailable() {
        LOG(DEBUG, __FUNCTION__);
        while (!exiting_) {
            const google::protobuf::Empty request;
            google::protobuf::Empty response;
            ClientContext context;
            grpc::Status reqStatus =
                stub_->isServiceAvailable(&context, request, &response);
            if (!reqStatus.ok()) {
                LOG(DEBUG, __FUNCTION__, " Server not available yet");
                std::this_thread::sleep_for(std::chrono::milliseconds(DEFAULT_DELAY));
                continue;
            }
            this->getEvents();
        }
    }

    /**
     * @brief This API is to request events from server. It initializes a stream that
     * handles the events of type ::eventService::EventResponse.
     */
    void getEvents() {
        LOG(DEBUG, __FUNCTION__);

        ::eventService::EventRequest request;
        ::eventService::EventResponse response;

        request.set_client_id(getpid());

        if (!stub_) {
            LOG(DEBUG, __FUNCTION__, " Failed to create channel");
            return;
        }

        std::unique_ptr<grpc::ClientReader<::eventService::EventResponse> > reader(
            stub_->registerForEvents(getClientContext(), request));

        if (!reader) {
            LOG(DEBUG, __FUNCTION__, " Failed to create reader");
            return;
        }

        connectedToSimulationServer_ = true;
        updateFilters();

        std::string readStr;
        while (reader->Read(&response)) {
            LOG(DEBUG, __FUNCTION__, " Received event for::", response.filter());
            if (response.has_any()) {
                auto f = std::async(policy_, [this, response]() {
                        this->handleEventNotifications(response);
                        }).share();
                taskQ_->add(f);
            }
        }
        grpc::Status status = reader->Finish();
        connectedToSimulationServer_ = false;
        clearClientContext();

        if (status.ok()) {
            LOG(DEBUG, __FUNCTION__, " RequestEvent succeeded.");
        } else {
            LOG(DEBUG, __FUNCTION__, " RequestEvent failed.", status.error_message());
        }
    }

    /**
     * @brief To achieve multicast from server, a list of filters is provided by client
     * while connecting to server, that filter list is maintained on server side in RAM.
     * This API is allowing client to update the filters list maintained by server.
     */
    void updateFilters() {
        LOG(DEBUG, __FUNCTION__);

        std::lock_guard<std::mutex> lck(mtx_);

        if (listeners_.size() == 0) {
            return;
        }

        ::eventService::EventRequest request;
        google::protobuf::Empty response;
        ClientContext context;

        request.set_client_id(getpid());
        for (auto& listener: listeners_) {
            LOG(DEBUG, __FUNCTION__, " Updating filter::", listener.first);
            request.add_filters(listener.first);
        }

        if (!stub_) {
            LOG(DEBUG, __FUNCTION__, " Failed to create channel");
            return;
        }

        grpc::Status reqStatus = stub_->updateFilter(&context, request, &response);
        if (!reqStatus.ok()) {
            LOG(DEBUG, __FUNCTION__, " Failed to update filters");
        }
    }

    void cleanup() {
        LOG(DEBUG, __FUNCTION__);

        if (!stub_) {
            LOG(DEBUG, __FUNCTION__, " Failed to create channel");
            return;
        }

        ClientContext context;
        ::eventService::CleanupRequest request;
        ::google::protobuf::Empty response;

        request.set_client_id(getpid());
        grpc::Status reqStatus = stub_->cleanup(&context, request, &response);
        if (!reqStatus.ok()) {
            LOG(DEBUG, __FUNCTION__, " Failed to do cleanup");
        }
    }

    // we are storing client context so that, we can cancel the blocked stream call,
    // while the application is exiting.
    grpc::ClientContext* getClientContext() {
        LOG(DEBUG, __FUNCTION__);
        std::lock_guard<std::mutex> lck(mtx_);

        if(!contextPtr_) {
            contextPtr_ = new grpc::ClientContext();
        }

        return contextPtr_;
    }

    // we need to clear the client context to handle server restart scenarios.
    void clearClientContext() {
        LOG(DEBUG, __FUNCTION__);
        {
            std::lock_guard<std::mutex> lck(mtx_);
            if (contextPtr_) {
                delete contextPtr_;
                contextPtr_ = nullptr;
            }
        }
    }

    bool connectedToSimulationServer_ = false;
    bool exiting_ = false;

    std::launch policy_;

    std::mutex listenerMutex_;
    std::mutex mtx_;
    std::mutex exitingMutex_;

    grpc::ClientContext* contextPtr_;
    /*
    * owner_less performs an owner-based comparison b/w
    * shared_ptr or weak_ptr. Required for cases when container contains
    * shared_ptr/weak_ptr & we need to perform comparison.
    */
    std::unordered_map<std::string,
        std::set<std::weak_ptr<IEventListener>,
        owner_less<std::weak_ptr<IEventListener>>>> listeners_;

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
};

} // end of namespace common

} // end of namespace telux

#endif  // EVENT_MANAGER_HPP
