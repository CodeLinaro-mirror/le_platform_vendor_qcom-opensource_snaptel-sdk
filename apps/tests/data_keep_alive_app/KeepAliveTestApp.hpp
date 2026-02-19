/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef KEEPALIVETESTAPP_HPP
#define KEEPALIVETESTAPP_HPP

#include <memory>

#include "ConsoleApp.hpp"
#include "tcp_socket/TCPClient.hpp"
#include "tcp_socket/TCPServer.hpp"
#include <telux/data/DataFactory.hpp>

#define APP_NAME "data_keep_alive_app"

using namespace telux::data;
using namespace telux::common;
using namespace tcp_test;

struct kaproto {
  char msg[1024];
};

void printMessage(kaproto &msg) { std::cout << msg.msg << std::endl; }
namespace tcp_test {
template <> class TCPClientWorker<kaproto> {
public:
  TCPClientWorker() {}
  virtual ~TCPClientWorker() {}
  virtual void messageReceived(kaproto &msg) {
    std::cout << "Received: ";
    printMessage(msg);
  }
  virtual void onDisconnect() { std::cout << "disconnected\n"; }
  virtual void onConnected() { std::cout << "connected\n"; }
};
template <> class TCPServerWorker<kaproto> {

public:
  TCPServerWorker() {}
  virtual ~TCPServerWorker() {
    if (s_) {
      s_->disconnect();
    }
  }
  void setServer(std::shared_ptr<TCPServer<kaproto>> s) { s_ = s; }
  virtual void onAccept(std::string ip, int port) {
    std::cout << "Connected: " << ip << ":" << port << std::endl;
  }
  virtual void messageReceived(kaproto &msg) {
    std::cout << "Received: ";
    printMessage(msg);
  }
  virtual void onDisconnect() { std::cout << "disconnected\n"; }

private:
  std::shared_ptr<TCPServer<kaproto>> s_;
};
} // namespace tcp_test

class KeepAliveTestApp : public IKeepAliveListener,
                         public ConsoleApp,
                         public std::enable_shared_from_this<KeepAliveTestApp> {
public:
  KeepAliveTestApp();
  ~KeepAliveTestApp();

  void startTCPServer(std::vector<std::string> inputCommand);
  void stopTCPServer(std::vector<std::string> inputCommand);
  void startServerThread();

  void startTCPClient(std::vector<std::string> inputCommand);
  void stopTCPClient(std::vector<std::string> inputCommand);
  void startClientThread();

  void sendMessage(std::vector<std::string> inputCommand);
  void enableTCPMonitor(std::vector<std::string> inputCommand);
  void disableTCPMonitor(std::vector<std::string> inputCommand);
  void startTCPKeepAliveOffload(std::vector<std::string> inputCommand);
  void stopTCPKeepAliveOffload(std::vector<std::string> inputCommand);

  void onKeepAliveStatusChange(ErrorCode error,
                               TCPKAOffloadHandle handle) override;
  void onServiceStatusChange(telux::common::ServiceStatus status) override;
  void registerForUpdates();
  void deregisterForUpdates();
  void consoleInit(bool isServer);
  void cleanup();

  std::shared_ptr<telux::data::IKeepAliveManager> keepAliveMgr_;

private:
  KeepAliveTestApp(KeepAliveTestApp const &) = delete;
  KeepAliveTestApp &operator=(KeepAliveTestApp const &) = delete;

  void
  onTFTResponse(const std::vector<std::shared_ptr<TrafficFlowTemplate>> &tft,
                ErrorCode error);
  void logQosDetails(std::shared_ptr<TrafficFlowTemplate> &tft);
  void printFilterDetails(std::shared_ptr<telux::data::IIpFilter> filter);

  int profileId_;
  std::shared_ptr<TCPServer<kaproto>> server_;
  std::shared_ptr<TCPServerWorker<kaproto>> serverWorker_;

  std::shared_ptr<TCPClient<kaproto>> client_;
  std::shared_ptr<TCPClientWorker<kaproto>> clientWorker_;
  std::weak_ptr<KeepAliveTestApp> self_;

  std::thread serverThread_;
  std::thread clientThread_;
  bool isServer_ = false;

  std::condition_variable clientExitCondition_;
  std::mutex clientExitMutex_;
  std::atomic<bool> clientExited_ = {true};

  std::condition_variable serverExitCondition_;
  std::mutex serverExitMutex_;
  std::atomic<bool> serverExited_ = {true};
};

#endif // KEEPALIVETESTAPP_HPP
