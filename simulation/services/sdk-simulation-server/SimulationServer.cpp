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
 * @file       SimulationServer.cpp
 *
 * @brief      Implements the @ref SimulationServer class.
 *
 */

#include <iostream>
#include <sys/un.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <thread>
#include <telux/common/CommonDefines.hpp>

#include <grpcpp/grpcpp.h>

#include "../../libs/common/SimulationConfigParser.hpp"
#include "../../libs/common/Logger.hpp"

#include "SimulationServer.hpp"
#include "tel/CardManagerServerImpl.hpp"
#include "tel/SubscriptionManagerServerImpl.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

#define LOCAL_HOST "127.0.0.1"
#define DEFAULT_PORT 8080
#define DEFUALT_GRPC_PORT "8089"

/* Defining the SimulationServer app instance */
SimulationServer::SimulationServer() {
    taskQ_ = std::make_shared<telux::common::AsyncTaskQueue<void>>();
}

SimulationServer::~SimulationServer(){
    {
        std::lock_guard<std::mutex> lock(exitingMutex_);
        exiting_ = true;
    }

    taskQ_ = nullptr;
}

telux::common::Status SimulationServer::start() {
    struct sockaddr_in address = {0};
    int socketFd;
    int addrlen = sizeof(address);
    int opt = 1;
    int serverSocket;
    std::vector<int> clientSockets;

    std::thread grpc_sim_server([this] {
            startGrpcServer();
        }
    );

    std::shared_ptr<SimulationConfigParser> config =
        std::make_shared<SimulationConfigParser>();

    if ((serverSocket = socket(AF_INET,SOCK_STREAM,0)) < 0) {
        LOG(ERROR, "failed to create socket");
        return telux::common::Status::FAILED;
    }

    LOG(INFO, "socket created::", serverSocket);
    std::string portString = config->getValue("PORT");
    int port = DEFAULT_PORT;
    if (portString != "") {
        port = std::stoi(portString);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr(LOCAL_HOST);
    address.sin_port = htons(port);

    if (setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
        sizeof(opt)) < 0) {
        LOG(ERROR, "setsockopt failed");
        return telux::common::Status::FAILED;
    }

    if (bind(serverSocket, (struct sockaddr *)&address,
        sizeof(address)) < 0) {
        LOG(ERROR, "binding socket failed");
        return telux::common::Status::FAILED;
    }

    if (listen(serverSocket,5) < 0) {
        LOG(ERROR, "listen failed");
        return telux::common::Status::FAILED;
    }

    while(true) {
        {
            std::lock_guard<std::mutex> lock(exitingMutex_);
            if (exiting_) {
                break;
            }
        }

        LOG(INFO, "waiting to accept connection");
        socketFd = accept(serverSocket, (struct sockaddr *)&address, (socklen_t *)&addrlen);
        if (socketFd < 0)
        {
            LOG(ERROR, "accept failed");
            return telux::common::Status::FAILED;
        }

        LOG(INFO, "client connected::", socketFd);
        clientSockets.push_back(socketFd);

        auto f = std::async(std::launch::async, [this, socketFd, &clientSockets]() {
            this->readMessage(socketFd, clientSockets);
        }).share();
        taskQ_->add(f);
    }

    grpc_sim_server.join();
    close(serverSocket);
    for(auto &socket_: clientSockets) {
        close(socket_);
    }
    return telux::common::Status::SUCCESS;
}

telux::common::Status SimulationServer::readMessage(int socketFd,
    std::vector<int> &clientSockets) {
    char buffer[BUFFER_SIZE];

    while(true) {
        int length = read(socketFd,buffer, BUFFER_SIZE);
        if (length <= 0)
        {
            close(socketFd);
            clientSockets.erase(find(clientSockets.begin(),clientSockets.end(), socketFd));
            LOG(INFO, __FUNCTION__, "socket disconnected::", socketFd);
            return telux::common::Status::SUCCESS;
        }
        else {
            LOG(DEBUG, __FUNCTION__, "received data::", buffer);
            writeMessage(buffer, length, clientSockets);
            memset(buffer, 0, BUFFER_SIZE * (sizeof buffer[0]));
        }

        {
            std::lock_guard<std::mutex> lock(exitingMutex_);
            if (exiting_) {
                break;
            }
        }
    }
    return telux::common::Status::SUCCESS;
}


telux::common::Status SimulationServer::writeMessage(char* buffer, int length,
    const std::vector<int> &clientSockets) {
    LOG(DEBUG, __FUNCTION__);
    for(auto socket: clientSockets)
    {
        write(socket,buffer,length);
    }
    return telux::common::Status::SUCCESS;
}

std::string SimulationServer::createServerAddress(std::string ipAddress, std::string portNo) {
    return ipAddress+ ":" + portNo;
}

void SimulationServer::startGrpcServer() {
    std::string serverIpAddress = LOCAL_HOST;
    std::string serverAddress = createServerAddress(serverIpAddress, DEFUALT_GRPC_PORT);
    std::string server_address(serverAddress);

    grpc::EnableDefaultHealthCheckService(true);
    ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());

    std::shared_ptr<CardManagerServerImpl> cardService = std::make_shared<CardManagerServerImpl>();
    builder.RegisterService(cardService.get());

    std::shared_ptr<SubscriptionManagerServerImpl> subscriptionService =
        std::make_shared<SubscriptionManagerServerImpl>();
    builder.RegisterService(subscriptionService.get());

    std::unique_ptr<Server> server(builder.BuildAndStart());
    LOG(DEBUG, __FUNCTION__, " Server listening on ", server_address);
    server->Wait();
}

/**
 * Main routine
 */
int main(int argc, char ** argv) {
    SimulationServer simulationServer;

    if (simulationServer.start() != telux::common::Status::SUCCESS) {
        LOG(ERROR, __FUNCTION__, " failed to start ", APP_NAME);
        return -1;
    }

    std::cout << "\nInfo: Exiting application..." << std::endl;
    return 0;
}
