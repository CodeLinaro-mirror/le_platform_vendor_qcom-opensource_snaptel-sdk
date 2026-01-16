/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef ANTENNATESTAPP_HPP
#define ANTENNATESTAPP_HPP

#include <condition_variable>
#include <mutex>
#include <memory>
#include <string>
#include <future>

#include <telux/platform/PlatformFactory.hpp>
#include "MyAntennaListener.hpp"
#include "ConsoleApp.hpp"

using namespace telux::platform;
using namespace telux::common;

class AntennaTestApp : public hardware::IAntennaListener, public ConsoleApp {
 public:
    AntennaTestApp();
    ~AntennaTestApp();

    int init();
    void cleanup();

    void signalHandler(int signum);
    void consoleinit();

 private:
    std::shared_ptr<telux::platform::hardware::IAntennaManager> antMgr_;
    std::shared_ptr<MyAntennaListener> antListener_;
    void setAntConfig(std::vector<std::string> userInput);
    void getAntConfig(std::vector<std::string> userInput);
};

#endif  // ANTENNATESTAPP_HPP
