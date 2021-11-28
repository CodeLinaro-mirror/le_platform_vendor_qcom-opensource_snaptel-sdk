/*
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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


#include <iostream>
#include <memory>
#include <cstdlib>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <algorithm>
#include <errno.h>
#include <net/if.h>
#include <glib.h>

#include <telux/data/DataFactory.hpp>

/**
 * @file: DataOnDemandPdnApp.cpp
 *
 * @brief: Sample application to demonstrate
 * - Bringing up data calls on requested profile and slot
 * - DNS resolution using dig
 * - Data communication with remote host after binding to an interface
 */

#define SIZE_IP_ADDR_BUF 40
#define RESPONSE_BUF 4096

int connect(std::string ipAddress, std::string outBoundIf, std::string portNumber) {
    std::cout << "Connecting to " << ipAddress << " on port " << portNumber << " via " << outBoundIf
              << std::endl;
    int sockfd = 0;
    sockaddr_in serverIpAddress;
    serverIpAddress.sin_family = AF_INET;
    serverIpAddress.sin_port = htons(stoi(portNumber));

    if (inet_pton(AF_INET, ipAddress.c_str(), &serverIpAddress.sin_addr) <= 0) {
        std::cerr << "Cannot parse IP address" << std::endl;
        return -1;
    }
    // Create the socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return -1;
    }

    // Bind the socket to the interface
    ifreq ifr;
    g_strlcpy(ifr.ifr_name, outBoundIf.c_str(), outBoundIf.length());
    if (setsockopt(sockfd, SOL_SOCKET, SO_BINDTODEVICE, (void *)&ifr, sizeof(ifr)) < 0) {
        std::cerr << "Socket bind failed with " << strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }

    // Connect to the remote host
    if (connect(sockfd, (sockaddr *)&serverIpAddress, sizeof(serverIpAddress)) < 0) {
        std::cerr << "Connect failed: " << strerror(errno) << std::endl;
        close(sockfd);
        return -1;
    }
    return sockfd;
}

std::string resolve(std::string domain, std::string dnsAddress) {
    std::cout << "Resolving " << domain << " using DNS server at " << dnsAddress << std::endl;
    FILE *cmd;
    std::string ipAddress;
    char cipAddress[SIZE_IP_ADDR_BUF] = {0};

    // Use the provided DNS address to request dig for name resolution
    std::string command = "/usr/bin/dig @" + dnsAddress + " " + domain + " +short";
    std::cout << "Command: " << command << std::endl;
    cmd = popen(command.c_str(), "r");
    if (cmd) {
        sockaddr_in address;
        // Get all the answers from the DNS server
        while (NULL != fgets(cipAddress, SIZE_IP_ADDR_BUF, cmd)) {
            cipAddress[strlen(cipAddress) - 1] = '\0';

            // If the received answer is verified as a valid IP address, return the address
            if (inet_pton(AF_INET, cipAddress, &address.sin_addr) == 1) {
                ipAddress = cipAddress;
                std::cout << ipAddress;
                ipAddress.erase(
                    std::remove(ipAddress.begin(), ipAddress.end(), '\n'), ipAddress.end());
                break;
            }
        }
    }
    std::cout << "\n\n";  // Declutters output from dig
    return ipAddress;
}

class DataConnectionListener : public telux::data::IDataConnectionListener {
 public:
    DataConnectionListener(std::promise<std::shared_ptr<telux::data::IDataCall>> p) {
        p_ = std::move(p);
    }
    void onDataCallInfoChanged(const std::shared_ptr<telux::data::IDataCall> &dataCall) override {
        std::cout << "\n onDataCallInfoChanged";
        logDataCallDetails(dataCall);
        if (dataCall->getDataCallStatus() == telux::data::DataCallStatus::NET_CONNECTED) {
            p_.set_value(dataCall);
        }
    }

 private:
    void logDataCallDetails(const std::shared_ptr<telux::data::IDataCall> &dataCall) {
        std::cout << " ** DataCall Details **\n";
        std::cout << " SlotID: " << dataCall->getSlotId() << std::endl;
        std::cout << " ProfileID: " << dataCall->getProfileId() << std::endl;
        std::cout << " interfaceName: " << dataCall->getInterfaceName() << std::endl;
        std::cout << " DataCallStatus: " << (int)dataCall->getDataCallStatus() << std::endl;
        std::cout << " DataCallEndReason: Type = "
                  << static_cast<int>(dataCall->getDataCallEndReason().type) << std::endl;
        std::list<telux::data::IpAddrInfo> ipAddrList = dataCall->getIpAddressInfo();
        for (auto &it : ipAddrList) {
            std::cout << "\n ifAddress: " << it.ifAddress
                      << "\n primaryDnsAddress: " << it.primaryDnsAddress
                      << "\n secondaryDnsAddress: " << it.secondaryDnsAddress << '\n';
        }
        std::cout << " IpFamilyType: " << static_cast<int>(dataCall->getIpFamilyType()) << '\n';
        std::cout << " TechPreference: " << static_cast<int>(dataCall->getTechPreference()) << '\n';
        std::cout << " DataBearerTechnology: " << static_cast<int>(dataCall->getCurrentBearerTech())
                  << '\n';
    }
    std::promise<std::shared_ptr<telux::data::IDataCall>> p_;
};

int main(int argc, char *argv[]) {
    std::shared_ptr<telux::data::IDataConnectionManager> dataConnMgr = nullptr;
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;

    std::promise<std::shared_ptr<telux::data::IDataCall>> dataCallPromise;
    std::future<std::shared_ptr<telux::data::IDataCall>> dataCallFuture
        = dataCallPromise.get_future();

    std::shared_ptr<telux::data::IDataConnectionListener> dataListener
        = std::make_shared<DataConnectionListener>(std::move(dataCallPromise));

    if (argc != 6) {
        std::cerr << "\n Invalid argument!!! \n\n";
        std::cerr << "\n Sample command is: \n";
        std::cerr << "\n\t " << argv[0] << " <slotId> <profieId> <operationType> <domain> <port>\n";
        std::cerr << "\n\t\t slot id        Slot id that contains modem profile";
        std::cerr << "\n\t\t profile id     modem profile id to start data call on";
        std::cerr << "\n\t\t operation type (0-LOCAL, 1-REMOTE)";
        std::cerr << "\n\t\t domain: The remote host to connect to";
        std::cerr << "\n\t\t port: The port on remote host to connect to";
        std::cerr << "\n\t ./data_app 1 2 0 www.example.com  --> start data call on slotId 1 and "
                     "profile Id 2 on local and connect to www.example.com on port 80\n";
        exit(1);
    }

    SlotId slotId = static_cast<SlotId>(std::atoi(argv[1]));
    int profileId = std::atoi(argv[2]);
    telux::data::OperationType opType = static_cast<telux::data::OperationType>(std::atoi(argv[3]));
    std::string domain = argv[4];
    std::string portNumber = argv[5];

    // [1] Get Data Connection Manager and wait for service availability
    auto &dataFactory = telux::data::DataFactory::getInstance();
    {
        std::promise<telux::common::ServiceStatus> p;
        dataConnMgr = dataFactory.getDataConnectionManager(
            slotId, [&](telux::common::ServiceStatus status) { p.set_value(status); });
        if (dataConnMgr) {
            std::cout << "\n\nInitializing Data connection manager subsystem on slot " << slotId
                      << ", Please wait ..." << std::endl;
            subSystemStatus = p.get_future().get();
        }
        if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
            std::cout << "Data sub-system is ready" << std::endl;
        } else {
            std::cerr << "Unable to initialize data subsystem. Exiting..." << std::endl;
            exit(1);
        }
    }

    // [2] Register listener
    dataConnMgr->registerListener(dataListener);

    // [3] Start data call on the mentioned slot Id and profile id and operation type
    {
        std::promise<telux::common::ErrorCode> p;
        telux::data::IpFamilyType ipFamilyType = telux::data::IpFamilyType::IPV4;
        dataConnMgr->startDataCall(
            profileId, ipFamilyType,
            [&](const std::shared_ptr<telux::data::IDataCall> &dataCall,
                telux::common::ErrorCode errorCode) {
                std::cout << "startCallResponse: errorCode: " << static_cast<int>(errorCode)
                          << std::endl;
                p.set_value(errorCode);
            },
            opType);
        telux::common::ErrorCode errorCode = p.get_future().get();
        if (errorCode != telux::common::ErrorCode::SUCCESS) {
            std::cerr << "Failed to start data call. Exiting..." << std::endl;
            exit(1);
        }
    }

    // [4] Wait for the data call to get connected
    std::shared_ptr<telux::data::IDataCall> dataCall = dataCallFuture.get();
    if (dataCall == nullptr) {
        std::cerr << "Could not get data call object. Exiting..." << std::endl;
        exit(1);
    }

    // [5] Resolve the remote host using the DNS address provided by the data call
    std::string remoteIp = resolve(domain, dataCall->getIpv4Info().addr.primaryDnsAddress);
    if (remoteIp == "") {
        std::cerr << "Could not resolve " << domain << ". Exiting..." << std::endl;
        exit(1);
    }
    std::cout << "Resolved " << domain << " to " << remoteIp << std::endl;

    // [6] Connect to the remote host
    int sockfd = connect(remoteIp, dataCall->getInterfaceName(), portNumber);
    if (sockfd < 0) {
        std::cerr << "Could not connect to " << domain << ". Exiting..." << std::endl;
        exit(1);
    }
    std::cout << "Connected to " << domain << std::endl;

    std::cout << "Press any key to exit" << std::endl;
    std::cin.ignore();

    // [7] Clean-up
    std::cout << "Cleaning up" << std::endl;
    close(sockfd);
    dataConnMgr->deregisterListener(dataListener);
    dataConnMgr = nullptr;

    return 0;
}
