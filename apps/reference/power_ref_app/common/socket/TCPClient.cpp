/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef TCP_CLIENT_CPP
#define TCP_CLIENT_CPP

#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <iostream>
#include <memory>
#include <mutex>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <telux/data/DataDefines.hpp>
#include <unistd.h>
#include <vector>
#include "ISocketConnectionListener.hpp"
#include <telux/common/Log.hpp>

class TCPClient : public IIPConnection {
 public:
    TCPClient() {
        LOGFD();
    }
    ~TCPClient() {
        LOGFD();
        cleanup();
    }

    bool isStarted() override {
        LOGFD();
        return !receivedStopClient_;
    }

    bool isConnected() override {
        LOGFD();
        return isConnected_;
    }

    std::shared_ptr<Connection> getConnectionParams() override {
        LOGFD();
        std::unique_lock<std::mutex> lock(mtx_);
        return connectionConfig_;
    }

    bool updateConnectionParams() {
        LOGFD();
        char ip_str[INET6_ADDRSTRLEN];
        uint16_t port;

        // Get the local (source) address and port
        struct sockaddr_storage local_addr;
        socklen_t addr_len = sizeof(local_addr);
        if (getsockname(clientSocket_, (struct sockaddr *)&local_addr, &addr_len) < 0) {
            LOGFE("Error getting local address");
            close(clientSocket_);
            isConnected_ = false;
            return false;
        }
        if (local_addr.ss_family == AF_INET) {  // IPv4
            struct sockaddr_in *addr_in = (struct sockaddr_in *)&local_addr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str));
            port                            = ntohs(addr_in->sin_port);
            connectionConfig_->clientIpAddr = ip_str;
            connectionConfig_->clientPort   = port;
            connectionConfig_->ipFamily     = telux::data::IpFamilyType::IPV4;
        } else if (local_addr.ss_family == AF_INET6) {  // IPv6
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&local_addr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ip_str, sizeof(ip_str));
            port                            = ntohs(addr_in6->sin6_port);
            connectionConfig_->clientIpAddr = ip_str;
            connectionConfig_->clientPort   = port;
            connectionConfig_->ipFamily     = telux::data::IpFamilyType::IPV6;
        }

        // Get the remote (destination) address and port
        struct sockaddr_storage remote_addr;
        addr_len = sizeof(remote_addr);
        if (getpeername(clientSocket_, (struct sockaddr *)&remote_addr, &addr_len) < 0) {
            LOGFE("Error getting remote address");
            close(clientSocket_);
            isConnected_ = false;
            return false;
        }
        if (remote_addr.ss_family == AF_INET) {  // IPv4
            struct sockaddr_in *addr_in = (struct sockaddr_in *)&remote_addr;
            inet_ntop(AF_INET, &(addr_in->sin_addr), ip_str, sizeof(ip_str));
            port                            = ntohs(addr_in->sin_port);
            connectionConfig_->serverIpAddr = ip_str;
            connectionConfig_->serverPort   = port;
        } else if (remote_addr.ss_family == AF_INET6) {  // IPv6
            struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)&remote_addr;
            inet_ntop(AF_INET6, &(addr_in6->sin6_addr), ip_str, sizeof(ip_str));
            port                            = ntohs(addr_in6->sin6_port);
            connectionConfig_->serverIpAddr = ip_str;
            connectionConfig_->serverPort   = port;
        }
        // Call the worker onConnect method with the updated connection info
        for (auto listener : listeners_) {
            listener->onConnect(connectionConfig_);
        }

        LOGFD("%s", (connectionConfig_->toString()).c_str());

        IPMessage msg{};
        std::string messageStr
            = "\n\n [Connected client] config: " + connectionConfig_->toString() + "\n\n";
        std::snprintf(msg.msg, sizeof(msg.msg), "%s", messageStr.c_str());
        sendMessage(msg);
        return true;
    }

    bool readLoop() {
        LOGFD();
        if (clientSocket_ < 0) {
            LOGFE("Invalid socket descriptor");
            return false;
        }

        try {
            do {
                IPMessage msg;
                memset(&msg, 0, sizeof(msg));
                ssize_t n = 0;
                n         = recv(clientSocket_, static_cast<void *>(&msg), sizeof(msg), 0);

                if (n <= 0) {
                    LOGFE("connection interrupted or closed");
                    isConnected_ = false;
                    break;
                }

                LOGFD("length = %zd current listeners: %zu", n, listeners_.size());
                for (auto listener : listeners_) {
                    listener->messageReceived(msg, n, connectionConfig_);
                }

            } while (true);

            for (auto listener : listeners_) {
                listener->onDisconnect(connectionConfig_);
            }

            if (connectionConfig_->protocol == Protocol::TCP) {
                if (shutdown(clientSocket_, SHUT_RDWR) == -1) {
                    LOGFE("shutdown failed errno = %s", strerror(errno));
                }
            }

            if (close(clientSocket_) == -1) {
                LOGFE("close failed errno = %s", strerror(errno));
            }

        } catch (const std::exception &e) {
            isConnected_ = false;
            LOGFE("exception: %s", e.what());
        }

        LOGFD("exit");
        return true;
    }

    bool start(std::shared_ptr<Connection> connectionConfig) override {
        LOGFD();
        receivedStopClient_ = false;
        connectionConfig_   = connectionConfig;
        std::lock_guard<std::mutex> lk(mtx_);
        if (isConnected_) {
            return false;
        }
        startTcp();
        return true;
    }

    void startTcp() {
        LOGFD();
        std::thread([=]() {
            do {
                isConnected_ = false;
                LOGFD("TCP client starting...");

                if (!setupSocketAndBind()) {
                    usleep(4000000);  // Retry delay
                    continue;
                };

                if (!connectToServer()) {
                    usleep(4000000);  // Retry delay
                    continue;
                }

                isConnected_ = true;
                int no       = 1;
                if (setsockopt(clientSocket_, SOL_SOCKET, SO_KEEPALIVE, &no, sizeof(int)) != 0) {
                    LOGFE(, "Failed to bind to device: %s", strerror(errno));
                    usleep(4000000);  // Retry delay
                    continue;
                }
                updateConnectionParams();
                readLoop();  // TCP-specific listener
            } while (!receivedStopClient_);
        }).detach();
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

    bool setupSocketAndBind() {
        LOGFD();
        int domain
            = (connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV6) ? AF_INET6 : AF_INET;

        clientSocket_ = socket(domain, SOCK_STREAM, 0);
        if (clientSocket_ < 0) {
            LOGFE("socket : %s", strerror(errno));
            return false;
        }

        struct sockaddr *sockAddrBind = nullptr;
        socklen_t sockSize            = 0;
        int reuse                     = 1;

        if (!connectionConfig_->configuredInterfaceName.empty()) {
            if (!bindToDevice(clientSocket_, connectionConfig_->configuredInterfaceName)) {
                LOGFE(" bind %s , %s", connectionConfig_->configuredInterfaceName, strerror(errno));
            }
        } else {
            // Prepare bind address
            if (!prepareBindAddress(sockAddrBind, sockSize)) {
                LOGFE(" prepareBindAddress bind : ", strerror(errno));
            }

            if (!bindToDevice(clientSocket_, connectionConfig_->dataCall->getInterfaceName())) {
                LOGFE(" bind : %s %s", connectionConfig_->dataCall->getInterfaceName().c_str(),
                    strerror(errno));
            }
            if (sockAddrBind != nullptr) {
                setsockopt(clientSocket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
                setsockopt(clientSocket_, SOL_SOCKET, SO_REUSEPORT, &reuse, sizeof(reuse));
                if (bind(clientSocket_, sockAddrBind, sockSize) < 0) {
                    LOGFE(" bind : %s", strerror(errno));
                }
            }
        }
        return true;
    }

    bool connectToServer() {
        LOGFD();
        struct sockaddr *sockAddrConnect = nullptr;
        socklen_t sockSize               = 0;

        if (connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV4) {
            struct sockaddr_in v4ServerAddr = {};
            if (!inet_pton(
                    AF_INET, connectionConfig_->serverIpAddr.c_str(), &(v4ServerAddr.sin_addr))) {
                LOGFE("failed destination IPv4 parsing");
                return false;
            }
            v4ServerAddr.sin_family = AF_INET;
            v4ServerAddr.sin_port   = htons(connectionConfig_->serverPort);
            sockAddrConnect         = reinterpret_cast<struct sockaddr *>(&v4ServerAddr);
            sockSize                = sizeof(sockaddr_in);
            if (connect(clientSocket_, sockAddrConnect, sockSize) == -1) {
                LOGFE(" connect %s ; Connection config %s", strerror(errno),
                    connectionConfig_->toString().c_str());
                return false;
            }
        } else {
            struct sockaddr_in6 v6ServerAddr = {};
            if (!inet_pton(
                    AF_INET6, connectionConfig_->serverIpAddr.c_str(), &(v6ServerAddr.sin6_addr))) {
                LOGFE("failed destination IPv6 parsing");
                return false;
            }
            v6ServerAddr.sin6_family = AF_INET6;
            v6ServerAddr.sin6_port   = htons(connectionConfig_->serverPort);
            sockAddrConnect          = reinterpret_cast<struct sockaddr *>(&v6ServerAddr);
            sockSize                 = sizeof(sockaddr_in6);
            if (connect(clientSocket_, sockAddrConnect, sockSize) == -1) {
                LOGFE(" connect %s ; Connection config %s", strerror(errno),
                    connectionConfig_->toString().c_str());
                return false;
            }
        }
        return true;
    }

    bool prepareBindAddress(struct sockaddr *&sockAddrBind, socklen_t &sockSize) {
        LOGFD();
        if (connectionConfig_->ipFamily == telux::data::IpFamilyType::IPV4) {
            static struct sockaddr_in v4ClientAddr = {};
            if (!connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.empty()) {
                if (!inet_pton(AF_INET,
                        connectionConfig_->dataCall->getIpv4Info().addr.ifAddress.c_str(),
                        &(v4ClientAddr.sin_addr))) {
                    LOGFE("failed source IPv4 parsing %s", strerror(errno));
                    return false;
                }
            }
            v4ClientAddr.sin_family = AF_INET;
            if (connectionConfig_->clientPort != 0) {
                v4ClientAddr.sin_port = htons(connectionConfig_->clientPort);
            }
            sockAddrBind = reinterpret_cast<struct sockaddr *>(&v4ClientAddr);
            sockSize     = sizeof(sockaddr_in);
        } else {
            static struct sockaddr_in6 v6ClientAddr = {};
            if (!connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.empty()) {
                if (!inet_pton(AF_INET6,
                        connectionConfig_->dataCall->getIpv6Info().addr.ifAddress.c_str(),
                        &(v6ClientAddr.sin6_addr))) {
                    LOGFE("failed source IPv6 parsing %s", strerror(errno));
                    return false;
                }
            }
            v6ClientAddr.sin6_family = AF_INET6;
            if (connectionConfig_->clientPort != 0) {
                v6ClientAddr.sin6_port = htons(connectionConfig_->clientPort);
            }
            sockAddrBind = reinterpret_cast<struct sockaddr *>(&v6ClientAddr);
            sockSize     = sizeof(sockaddr_in6);
        }
        return true;
    }

    void cleanup() override {
        LOGFE("Stopping client");
        receivedStopClient_ = true;
        isConnected_        = false;
        if (clientSocket_ != -1) {
            if (shutdown(clientSocket_, SHUT_RDWR) == -1) {
                LOGFE("shutdown : %s", strerror(errno));
            }
            if (close(clientSocket_) == -1) {
                LOGFE("close : %s", strerror(errno));
            }
        }
        clientSocket_ = -1;
    }

    bool sendMessage(IPMessage &msg) override {
        LOGFD();
        size_t expected   = strnlen(msg.msg, sizeof(msg.msg)) + 1;
        ssize_t sentBytes = send(clientSocket_, msg.msg, expected, 0);
        LOGFD(" sentBytes : %zd", sentBytes);
        if (sentBytes != (ssize_t)expected) {
            LOGFE(" send : %s", strerror(errno));
            for (auto listener : listeners_) {
                listener->onDisconnect(connectionConfig_);
            }
            close(clientSocket_);
            isConnected_ = false;
            return false;
        }
        return true;
    }

    bool ensureAllPacketsAcknowledged() {
        LOGFD();
        if (connectionConfig_->protocol == Protocol::TCP) {
            // Wait until all unacknowledged packets are acknowledged to avoid acks
            // disrupting TCP KA or other configuration
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

    void registerListener(std::shared_ptr<ISocketConnectionListener> listener) override {
        LOGFD();
        listeners_.push_back(listener);
    }

 private:
    std::vector<std::shared_ptr<ISocketConnectionListener>> listeners_;
    std::shared_ptr<Connection> connectionConfig_;
    std::thread clientThread_;
    int clientSocket_                     = -1;
    std::atomic<bool> receivedStopClient_ = {false};
    std::mutex mtx_;
    std::atomic<bool> isConnected_ = {false};
};
#endif  // TCP_CLIENT_CPP
