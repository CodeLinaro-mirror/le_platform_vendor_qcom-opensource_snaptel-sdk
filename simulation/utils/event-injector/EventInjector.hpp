/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * @file       EventInjector.hpp
 *
 * @brief      Declares the EventInjector class that sends notifications of Asynchronous Device
 *             events.
 *
 */

#ifndef EVENT_INJECTOR_HPP
#define EVENT_INJECTOR_HPP

#include <string>
#include <memory>

#include "protos/proto-src/event_simulation.grpc.pb.h"

#define APP_NAME "EventInjector"

class SimulationConfigParser;

class EventInjector {
 public:
    EventInjector();
    ~EventInjector();

    Status init();
    Status parseAndHandleArguments(int argc, char **argv);

    bool sendMessage_ = false;

 private:
    void printHelp(std::string subsystem = "", std::string event = "");
    Status sendMessage(std::string filter, std::string event);
    Json::Value eventObj_;
    std::unique_ptr<::eventService::EventDispatcherService::Stub> stub_;
};

#endif  // EVENT_INJECTOR_HPP