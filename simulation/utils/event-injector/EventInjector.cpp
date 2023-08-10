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
 * @file       EventInjector.cpp
 *
 * @brief      Implements the @ref EventInjector class.
 *
 */

#include <iostream>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include "../../libs/common/SimulationConfigParser.hpp"
#include "../../libs/common/JsonParser.hpp"
#include "../../libs/common/Logger.hpp"

extern "C" {
#include <getopt.h>
}

#include "EventInjector.hpp"

#define LOCAL_HOST "127.0.0.1"
#define DEFAULT_PORT 8080
#define RETRY_TIMER 500
#define EVENT_JSON "Events.json"

constexpr bool isOptionalArgumentPresent(char * optarg, int optind, int argc, char** argv) {
    return (optarg == NULL && optind < argc && argv[optind] != NULL && argv[optind][0] != '-');
}

const std::string FILTER_FLAG = "-f";
const std::string EVENT_FLAG = "-e";

void EventInjector::printHelp(std::string subsystem, std::string event) {
    std::cout << "\n-------------------------------------------------" << std::endl;

    if((subsystem.empty()) && (event.empty())) {
        std::cout << "\nUse the following command to get namespace specific help" << std::endl;
        std::cout << "\nUsage: telsdk_event_injector -h <subsystem>" << std::endl;
        std::cout << "\nSupported Subsystems : " << std::endl;
        for (const auto& subsystem : eventObj_.getMemberNames())
        {
            std::cout << subsystem << std::endl;
        }
    } else if(event.empty()) {
        std::cout << "\nUse the following command to get event specific help" << std::endl;
        std::cout << "\nUsage: telsdk_event_injector -h <subsystem> <event>" << std::endl;
        std::cout << "\nSupported events : " << std::endl;
        for(const auto& event : eventObj_[subsystem].getMemberNames())
        {
             std::cout << event << std::endl;
        }
    } else {
        std::cout << "\nUse the following command to inject event" << std::endl;
        std::cout << "\nUsage: " << std::endl;
        for(const auto& command : eventObj_[subsystem][event])
        {
             std::cout << command << std::endl;
        }
    }
}

EventInjector::EventInjector() {
}

EventInjector::~EventInjector(){
    if (config_ ) {
        config_ = nullptr;
    }
    close(clientSocket_);
}

Status EventInjector::makeConnectionAndSendMessage(std::string filter, std::string event) {
    struct sockaddr_in address = {0};

    if ((clientSocket_ = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        LOG(ERROR, "failed to create socket");
        return Status::FAILED;
    }
    LOG(INFO, "socket created::", clientSocket_);
    std::string portString = config_->getValue("PORT");
    int port = DEFAULT_PORT;
    if (portString != "") {
        port = std::stoi(portString);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    address.sin_port = htons(port);

    while(connect(clientSocket_, (struct sockaddr *)
            &address, sizeof(address)) != 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_TIMER));
    }
    LOG(DEBUG, "connected to SimulationServer");
    sendMessage(filter, event);
    return Status::SUCCESS;
}

Status EventInjector::init() {
    config_ = std::make_shared<SimulationConfigParser>();

    telux::common::ErrorCode readError =
        JsonParser::readFromJsonFile(eventObj_, EVENT_JSON);
    if (readError != telux::common::ErrorCode::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " Reading JSON File failed!");
    }

    return Status::SUCCESS;
}

Status EventInjector::parseAndHandleArguments(int argc, char **argv) {
    int arg;
    std::string filter;
    std::string event;

    static struct option long_options[] = {
        {"help",          optional_argument, 0, 'h'},
        {"filter",        required_argument, 0, 'f'},
        {"event",         required_argument, 0, 'e'},
        {NULL, 0, 0, '\0'}
    };

    while((arg = getopt_long(argc, argv, "h::f:e:", long_options, NULL)) != -1) {
        switch(arg) {
            case 'h':
                {
                    if (isOptionalArgumentPresent(optarg, optind, argc, argv)) {
                        optarg = argv[optind++];
                        std::string subsystemArg(optarg);
                        optarg = NULL; // resetting the optarg back to null.
                        if (isOptionalArgumentPresent(optarg, optind, argc, argv)) {
                            optarg = argv[optind++];
                            std::string eventArg(optarg);
                            optarg = NULL; // resetting the optarg back to null.
                            printHelp(subsystemArg, eventArg);
                        } else {
                            printHelp(subsystemArg);
                        }
                    } else {
                        printHelp();
                    }
                }
                break;
            case 'f':
                {
                    filter = optarg;
                }
                break;
            case 'e':
                {
                    event = optarg; // fetched event name;
                    optarg = NULL;
                    while(isOptionalArgumentPresent(optarg, optind, argc, argv)) {
                        optarg = argv[optind++];
                        std::string eventArg(optarg);
                        event = event + " " + eventArg;
                        optarg = NULL; // resetting the optarg back to null.
                    }
                    LOG(DEBUG, __FUNCTION__, " Final Event string is ", event);
                }
                break;
            default:
                LOG(ERROR, __FUNCTION__, " Entered options is not valid!");
                return Status::FAILED;
        }
        if ((!filter.empty()) && (!event.empty())) {
            makeConnectionAndSendMessage(filter, event);
        }
    }
    return Status::SUCCESS;
}

Status EventInjector::sendMessage(std::string filter, std::string event) {
    Status ret = Status::SUCCESS;
    std::string reformattedString = FILTER_FLAG + " " + filter + " " + EVENT_FLAG  + " " + event;

    LOG(DEBUG, __FUNCTION__, " String being sent to SDK is: ", reformattedString);
    write(clientSocket_, reformattedString.c_str(), reformattedString.length());

    LOG(DEBUG, __FUNCTION__, " event injected!");
    return ret;
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {
    Status ret = Status::SUCCESS;
    EventInjector eventInjectorObj;
    if (eventInjectorObj.init() != Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed to initialize ", APP_NAME);
        return -1;
    }

    ret = eventInjectorObj.parseAndHandleArguments(argc, argv);
    if (ret != Status::SUCCESS) {
        return -1;
    }

    std::cout << "\nInfo: Exiting application..." << std::endl;
    return 0;
}