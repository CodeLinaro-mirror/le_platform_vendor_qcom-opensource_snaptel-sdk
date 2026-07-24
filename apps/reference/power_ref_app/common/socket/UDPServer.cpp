/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */
#ifndef UDPSERVER_HPP
#define UDPSERVER_HPP

#include "Server.hpp"

class UDPServer : public IServer {
 public:
    UDPServer() {
        LOGFD();
    }
    ~UDPServer() {
        LOGFD();
        cleanup();
    }

    bool isStarted() override {
        LOGFD();
        return !this->isReceivedStopServer_;
    }

    bool isConnected() override {
        LOGFD();
        return this->isConnected_;
    }

    std::shared_ptr<Connection> getConnectionParams() override {
        LOGFD();
        return connectionConfig_;
    }

    bool bindToDevice(int clientSocket, std::string deviceName) {
        LOGFD();
        if (setsockopt(
                clientSocket, SOL_SOCKET, SO_BINDTODEVICE, deviceName.c_str(), deviceName.size())
            != 0) {
            LOGFE("Failed to bind to device: %s", strerror(errno));
            return false;
        }
        return true;
    }

    bool start(std::shared_ptr<Connection> connectionConfig) override {
        LOGFD();
        this->connectionConfig_     = connectionConfig;
        this->isReceivedStopServer_ = false;
        int domain
            = connectionConfig->ipFamily == telux::data::IpFamilyType::IPV4 ? AF_INET : AF_INET6;
        serverSocket_ = socket(domain, SOCK_DGRAM, 0);
        if (serverSocket_ == -1) {
            LOGFE("socket: %s", strerror(errno));
            this->isConnected_ = false;
            return false;
        }
        // Bindthesocket
        if (!bindSocket()) {
            return false;
        }
        std::thread([this]() { listenUDPSync(); }).detach();
        return true;
    }

    bool sendMessage(IPMessage &msg) override {
        LOGFD();
        if (send(serverSocket_, static_cast<const void *>(&msg), sizeof(IPMessage), 0)
            != sizeof(IPMessage)) {
            LOGFE("send: %s", strerror(errno));
            this->isConnected_ = false;

            for (auto listener : listeners_) {
                listener->onDisconnect(connectionConfig_);
            }
            close(serverSocket_);
            return false;
        }
        return true;
    }

    bool ensureAllPacketsAcknowledged() override {
        LOGFD();
        return true;
    }

    void cleanup() override {
        LOGFD("StoppingUDPServer");
        this->isReceivedStopServer_ = true;
        this->isConnected_          = false;
        if (serverSocket_) {
            if (shutdown(serverSocket_, SHUT_RDWR) == -1) {
                LOGFE("shutdown %s", strerror(errno));
            }
            if (close(serverSocket_) == -1) {
                LOGFE("close %s", strerror(errno));
            }
        }
        serverSocket_ = 0;
    }

 private:
    int serverSocket_;

    bool bindSocket() {
        LOGFD();
        struct sockaddr *sockAddr = nullptr;
        socklen_t sockSize        = 0;
        int reuse                 = 1;

        if (!bindToDevice(serverSocket_, connectionConfig_->dataCall->getInterfaceName())) {
            return false;
        }

        if (this->connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV4) {
            struct sockaddr_in v4ServerAddr = {};
            if (!connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.empty()) {
                inet_pton(AF_INET,
                    connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.c_str(),
                    &(v4ServerAddr.sin_addr));
            }
            v4ServerAddr.sin_family = AF_INET;
            v4ServerAddr.sin_port   = htons(this->connectionConfig_->serverPort);
            sockAddr                = reinterpret_cast<struct sockaddr *>(&v4ServerAddr);
            sockSize                = sizeof(sockaddr_in);
            this->connectionConfig_->serverIpAddr
                = connectionConfig_->dataCall->getIpv4Info().addr.ifAddress;
        } else {
            struct sockaddr_in6 v6ServerAddr = {};
            if (!connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.empty()) {
                inet_pton(AF_INET6,
                    connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.c_str(),
                    &(v6ServerAddr.sin6_addr));
            }
            v6ServerAddr.sin6_family = AF_INET6;
            v6ServerAddr.sin6_port   = htons(this->connectionConfig_->serverPort);
            sockAddr                 = reinterpret_cast<struct sockaddr *>(&v6ServerAddr);
            sockSize                 = sizeof(sockaddr_in6);
            this->connectionConfig_->serverIpAddr
                = connectionConfig_->dataCall->getIpv6Info().addr.ifAddress;
        }
        if (bind(serverSocket_, sockAddr, sockSize) < 0) {
            LOGFE("bind: %s", strerror(errno));
            this->isConnected_ = false;
            return false;
        }
        setsockopt(serverSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        return true;
    }

    void listenUDPSync() {
        LOGFD();
        do {
            IPMessage msg;
            memset(&msg, 0, sizeof(msg));
            struct sockaddr_in clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);
            ssize_t bytesReceived = recvfrom(serverSocket_, static_cast<void *>(&msg), sizeof(msg),
                0, (struct sockaddr *)&clientAddr, &clientAddrLen);
            if (bytesReceived < 0) {
                LOGFE("Error receiving data: %s", strerror(errno));
                close(serverSocket_);
                return;
            }
            if (bytesReceived < static_cast<ssize_t>(sizeof(msg))) {
                reinterpret_cast<char *>(&msg)[bytesReceived] = '\0';
            }
            char clientIp[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &clientAddr.sin_addr, clientIp, sizeof(clientIp));
            uint16_t clientPort = ntohs(clientAddr.sin_port);
            if (this->connectionConfig_->clientIpAddr.empty()) {
                this->connectionConfig_->clientIpAddr = std::string(clientIp);
            } else {
                if (strcmp(clientIp, this->connectionConfig_->clientIpAddr.c_str()) != 0) {
                    continue;
                }
            }
            if (this->connectionConfig_->clientPort != 0
                && this->connectionConfig_->clientPort != clientPort) {
                continue;
            }
            for (auto listener : listeners_) {
                listener->messageReceived(msg, bytesReceived, connectionConfig_);
            }
        } while (!this->isReceivedStopServer_);
        close(serverSocket_);
    }
};

#endif  // UDPSERVER_HPP
