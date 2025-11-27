/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef FILESYSTEMTESTAPP_HPP
#define FILESYSTEMTESTAPP_HPP

#include <condition_variable>
#include <mutex>
#include <memory>
#include <string>

#include <telux/platform/PlatformFactory.hpp>
#include "ConsoleApp.hpp"

using namespace telux::platform;
using namespace telux::common;

class FileSystemCommandMgr;

class FileSystemTestApp : public IFsListener, public ConsoleApp {
 public:
    FileSystemTestApp();
    ~FileSystemTestApp();

    int init();
    void cleanup();

    void printHelp();
    void signalHandler(int signum);
    Status parseArguments(int argc, char **argv);
    void consoleinit();

 private:
    // Member variable to keep the command manager object alive till application ends.
    std::shared_ptr<FileSystemCommandMgr> myFsCmdMgr_;
};

#endif  // FILESYSTEMTESTAPP_HPP
