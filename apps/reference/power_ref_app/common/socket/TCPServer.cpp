/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TCPSERVER_HPP
#define TCPSERVER_HPP

#include "Server.hpp"

class TCPServer : public IServer {
 public:
    TCPServer() {
        LOGFD();
    }
    ~TCPServer() {
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
        connectionConfig_     = connectionConfig;
        isReceivedStopServer_ = false;
        int domain
            = connectionConfig->ipFamily == telux::data::IpFamilyType::IPV4 ? AF_INET : AF_INET6;
        serverSocket_ = socket(domain, SOCK_STREAM, 0);
        if (serverSocket_ == -1) {
            LOGFE("socket : %s", strerror(errno));
            this->isConnected_ = false;
            return false;
        }
        // Bind and listen
        if (!bindSocket()) {
            return false;
        }
        std::thread([this]() { listenTCPSync(); }).detach();
        return true;
    }

    bool sendMessage(IPMessage &msg) override {
        LOGFD();
        if (send(clientSocket_, static_cast<const void *>(&msg), sizeof(IPMessage), 0)
            != sizeof(IPMessage)) {
            LOGFE("send : %s", strerror(errno));
            this->isConnected_ = false;
            for (auto listener : listeners_) {
                listener->onDisconnect(connectionConfig_);
            }
            close(clientSocket_);
            return false;
        }
        return true;
    }

    bool ensureAllPacketsAcknowledged() override {
        LOGFD();
        if (connectionConfig_->protocol == Protocol::TCP) {
            while (true) {
                struct tcp_info info;
                socklen_t len = sizeof(info);
                if (getsockopt(clientSocket_, IPPROTO_TCP, TCP_INFO, &info, &len) == 0) {
                    if (info.tcpi_unacked == 0) {
                        break;  // All ACKs received
                    }
                } else {
                    LOGFE("getsockopt : %s", strerror(errno));
                    return false;
                }
                usleep(100000);  // Sleep 100ms to avoid busy-waiting
            }
        }
        return true;
    }

    void cleanup() override {
        LOGFD("Stopping TCP Server");
        this->isReceivedStopServer_ = true;
        this->isConnected_          = false;
        if (serverSocket_ != -1) {
            if (shutdown(serverSocket_, SHUT_RDWR) == -1) {
                LOGFE("shutdown %s", strerror(errno));
            }
            if (close(serverSocket_) == -1) {
                LOGFE("close %s", strerror(errno));
            }
        }
        if (clientSocket_ != -1) {
            if (shutdown(clientSocket_, SHUT_RDWR) == -1) {
                LOGFE("client shutdown %s", strerror(errno));
            }
            if (close(clientSocket_) == -1) {
                LOGFE("client close %s", strerror(errno));
            }
        }
        serverSocket_ = -1;
        clientSocket_ = -1;
    }

 private:
    int serverSocket_ = -1;
    int clientSocket_ = -1;

    bool bindSocket() {
        LOGFD();
        struct sockaddr *sockAddr = nullptr;
        socklen_t sockSize        = 0;
        int reuse                 = 1;

        // Call bindToDevice after accept
        if (!connectionConfig_->configuredInterfaceName.empty()) {
            if (!bindToDevice(clientSocket_, connectionConfig_->configuredInterfaceName)) {
                LOGFE(" bind : %s %s", connectionConfig_->configuredInterfaceName.c_str(),
                    std::string(strerror(errno)).c_str());
            }
        } else {
            if (!bindToDevice(
                    this->clientSocket_, connectionConfig_->dataCall->getInterfaceName())) {
                LOGFE(" Failed to bind to device");
            }
        }

        // Set socket options BEFORE bind
        setsockopt(this->serverSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
        setsockopt(this->serverSocket_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
        if (this->connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV4) {
            struct sockaddr_in v4ServerAddr {};
            memset(&v4ServerAddr, 0, sizeof(v4ServerAddr));
            if (!this->connectionConfig_->serverIpAddr.empty()) {
                LOGFD(" IPv4 address format: %s", this->connectionConfig_->serverIpAddr.c_str());

                int ret = inet_pton(AF_INET, this->connectionConfig_->serverIpAddr.c_str(),
                    &(v4ServerAddr.sin_addr));
                if (ret <= 0) {
                    if (ret == 0) {
                        LOGFE(" Invalid IPv4 address format: %s",
                            this->connectionConfig_->serverIpAddr.c_str());
                    } else {
                        LOGFE(" inet_pton failed: ", std::string(strerror(errno)).c_str());
                    }
                    return false;
                }

            } else if (!this->connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.empty()) {
                LOGFE(" IPv4 address format: %s",
                    (this->connectionConfig_->dataCall->getIpv4Info().addr.ifAddress).c_str());
                int ret = inet_pton(AF_INET,
                    this->connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.c_str(),
                    &(v4ServerAddr.sin_addr));
                if (ret <= 0) {
                    if (ret == 0) {
                        LOGFE("Invalid IPv4 address format: %s",
                            (this->connectionConfig_->dataCall->getIpv4Info().addr.ifAddress)
                                .c_str());
                    } else {
                        LOGFE(" inet_pton failed: %s", std::string(strerror(errno)).c_str());
                    }
                    return false;
                }
            }
            v4ServerAddr.sin_family = AF_INET;
            v4ServerAddr.sin_port   = htons(this->connectionConfig_->serverPort);
            sockAddr                = reinterpret_cast<struct sockaddr *>(&v4ServerAddr);
            sockSize                = sizeof(sockaddr_in);

            if (bind(this->serverSocket_, sockAddr, sockSize) < 0) {
                LOGFE(" bind : %s", std::string(strerror(errno)).c_str());
                this->isConnected_ = false;
                if (this->serverSocket_ != -1) {
                    close(this->serverSocket_);
                    this->serverSocket_ = -1;
                }
                return false;
            }

            return true;
        } else {
            struct sockaddr_in6 v6ServerAddr = {};
            memset(&v6ServerAddr, 0, sizeof(v6ServerAddr));
            if (!this->connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.empty()) {
                LOGFE(" IPv6 address format: %s",
                    (this->connectionConfig_->dataCall->getIpv6Info().addr.ifAddress).c_str());
                int ret = inet_pton(AF_INET6,
                    this->connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.c_str(),
                    &(v6ServerAddr.sin6_addr));
                if (ret <= 0) {
                    if (ret == 0) {
                        LOGFE(" Invalid IPv6 address format: %s",
                            (this->connectionConfig_->dataCall->getIpv6Info().addr.ifAddress)
                                .c_str());
                    } else {
                        LOGFE(" inet_pton failed: %s", strerror(errno));
                    }
                    return false;
                }
            }
            v6ServerAddr.sin6_family = AF_INET6;
            v6ServerAddr.sin6_port   = htons(this->connectionConfig_->serverPort);
            sockAddr                 = reinterpret_cast<struct sockaddr *>(&v6ServerAddr);
            sockSize                 = sizeof(sockaddr_in6);

            if (bind(this->serverSocket_, sockAddr, sockSize) < 0) {
                LOGFE(" bind : %s", strerror(errno));
                this->isConnected_ = false;
                if (this->serverSocket_ != -1) {
                    close(this->serverSocket_);
                    this->serverSocket_ = -1;
                }
                return false;
            }
            return true;
        }
    }

    void listenTCPSync() {
        LOGFD();

        if (listen(this->serverSocket_, 3) < 0) {
            LOGFE("listen failed: %s", strerror(errno));
            return;
        }

        do {
            struct sockaddr_storage clientAddr;
            socklen_t clientAddrLen = sizeof(clientAddr);

            this->clientSocket_ = accept(this->serverSocket_,
                reinterpret_cast<struct sockaddr *>(&clientAddr), &clientAddrLen);
            if (this->clientSocket_ < 0) {
                LOGFE("accept failed: %s", strerror(errno));
                continue;
            }

            // Call bindToDevice after accept
            if (!connectionConfig_->configuredInterfaceName.empty()) {
                if (!bindToDevice(clientSocket_, connectionConfig_->configuredInterfaceName)) {
                    LOGFE(" bind : %s %s", connectionConfig_->configuredInterfaceName.c_str(),
                        strerror(errno));
                }
            } else {
                if (!bindToDevice(
                        this->clientSocket_, connectionConfig_->dataCall->getInterfaceName())) {
                    LOGFE(" Failed to bind to device");
                }
            }

            char clientIp[INET6_ADDRSTRLEN] = {};
            uint16_t clientPort             = 0;

            if (clientAddr.ss_family == AF_INET) {
                struct sockaddr_in *addr = reinterpret_cast<struct sockaddr_in *>(&clientAddr);
                inet_ntop(AF_INET, &addr->sin_addr, clientIp, sizeof(clientIp));
                clientPort = ntohs(addr->sin_port);
            } else if (clientAddr.ss_family == AF_INET6) {
                struct sockaddr_in6 *addr = reinterpret_cast<struct sockaddr_in6 *>(&clientAddr);
                inet_ntop(AF_INET6, &addr->sin6_addr, clientIp, sizeof(clientIp));
                clientPort = ntohs(addr->sin6_port);
            }

            // Filter by IP and port if configured
            if (!this->connectionConfig_->clientIpAddr.empty()
                && strcmp(clientIp, this->connectionConfig_->clientIpAddr.c_str()) != 0) {
                LOGFW(" rejected client IP: %s", clientIp);
                // close(this->clientSocket_);
                // continue;
            }

            if (this->connectionConfig_->clientPort != 0
                && this->connectionConfig_->clientPort != clientPort) {
                LOGFW(" rejected client port: %u", clientPort);
                // close(this->clientSocket_);
                // continue;
            }

            this->isConnected_ = true;
            updateConnectionParams();

            try {
                do {
                    IPMessage msg;
                    memset(&msg, 0, sizeof(msg));
                    ssize_t n
                        = recv(this->clientSocket_, static_cast<void *>(&msg), sizeof(msg), 0);
                    if (n <= 0) {
                        LOGFE("trigger connection interrupted");
                        this->isConnected_ = false;
                        break;
                    }
                    for (auto listener : listeners_) {
                        listener->messageReceived(msg, n, connectionConfig_);
                    }
                    LOGFD("length = %zd", n);
                } while (true);
            } catch (const std::exception &e) {
                this->isConnected_ = false;
                LOGFE("exception: %s", e.what());
            }

            // Always clean up socket
            if (shutdown(this->clientSocket_, SHUT_RDWR) == -1) {
                LOGFE("shutdown failed: %s", strerror(errno));
            }
            if (close(this->clientSocket_) == -1) {
                LOGFE("close failed: %s", strerror(errno));
            }
            for (auto listener : listeners_) {
                listener->onDisconnect(this->connectionConfig_);
            }

        } while (!this->isReceivedStopServer_);

        close(this->serverSocket_);
        LOGFD("exit");
    }

    bool updateConnectionParams() {
        LOGFD();
        int soc = clientSocket_;

        char ipStr[INET6_ADDRSTRLEN];
        uint16_t port;
        struct sockaddr_storage localAddr;
        socklen_t addrLen = sizeof(localAddr);

        if (getsockname(soc, (struct sockaddr *)&localAddr, &addrLen) == -1) {
            LOGFE("getsockname failed: %s", strerror(errno));
            return false;
        }

        if (localAddr.ss_family == AF_INET) {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)&localAddr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), ipStr, sizeof(ipStr));
            port                        = ntohs(addr_in->sin_port);
            connectionConfig_->ipFamily = telux::data::IpFamilyType::IPV4;
        } else if (localAddr.ss_family == AF_INET6) {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&localAddr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ipStr, sizeof(ipStr));
            port                        = ntohs(addr_in6->sin6_port);
            connectionConfig_->ipFamily = telux::data::IpFamilyType::IPV6;
        } else {
            LOGFE("Unknown address family");
            return false;
        }

        connectionConfig_->serverIpAddr = std::string(ipStr);
        connectionConfig_->serverPort   = port;

        // Get client info
        struct sockaddr_storage peerAddr;
        addrLen = sizeof(peerAddr);
        if (getpeername(soc, (struct sockaddr *)&peerAddr, &addrLen) == -1) {
            LOGFE("getpeername failed: %s", strerror(errno));
            return false;
        }

        if (peerAddr.ss_family == AF_INET) {
            struct sockaddr_in *addr_in = (struct sockaddr_in *)&peerAddr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), ipStr, sizeof(ipStr));
            port = ntohs(addr_in->sin_port);
        } else if (peerAddr.ss_family == AF_INET6) {
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&peerAddr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ipStr, sizeof(ipStr));
            port = ntohs(addr_in6->sin6_port);
        } else {
            LOGFE("Unknown peer address family");
            return false;
        }

        connectionConfig_->clientIpAddr = std::string(ipStr);
        connectionConfig_->clientPort   = port;
        return true;
    }
};
#endif  // TCPSERVER_HPP
