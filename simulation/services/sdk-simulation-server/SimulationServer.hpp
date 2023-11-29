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
 * @file       SimulationServer.hpp
 *
 * @brief      Declares the SimulationServer class
 *
 */

#ifndef SIMULATION_SERVER_HPP
#define SIMULATION_SERVER_HPP

#include <string>
#include <memory>
#include <vector>
#include "../../libs/common/AsyncTaskQueue.hpp"

#define APP_NAME "SimulationServer"
#define BUFFER_SIZE 600

/*
* For the events injected from telsdk_event_injector, we want the event flow to be
*
* telsdk_event_injector -> simulation_server -> vertical_server_impl -> lib
*
* to achieve this event flow we have introduced enum ClientType.
* ClientType = SERVER : when simulation_server is sending the events to vertical_server_impl
* ClientType = LIB    : when vertical_server_impl forwards the events to libs.
* ClientType = ALL    : when the event shall be forwarded to both vertical_server_impl & lib.
*                       Also the order of processing the event at server side or lib side does't
*                       matter.
*/
enum ClientType {
    ALL,
    SERVER,
    LIB,
};

class SimulationServer {
public:
    static SimulationServer &getInstance();
    telux::common::Status start();
    telux::common::Status writeMessage(std::string msg, int length,
        ClientType type = ClientType::ALL);

private:
    SimulationServer();
    ~SimulationServer();

    telux::common::Status readMessage(int socketFd);
    std::string createServerAddress(std::string ipAddress,
        std::string portNo);
    void startGrpcServer();
    void updateJsonValue(std::string message);

    std::shared_ptr<telux::common::AsyncTaskQueue<void>> taskQ_;
    bool exiting_ = false;
    std::mutex exitingMutex_;
    std::mutex writeMutex_;
    std::vector<int> clientSockets_;
};

#endif // SIMULATION_SERVER_HPP
