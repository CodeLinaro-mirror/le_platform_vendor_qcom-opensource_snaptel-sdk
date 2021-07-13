/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are
 *  met:
 *    * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *    * Redistributions in binary form must reproduce the above
 *      copyright notice, this list of conditions and the following
 *      disclaimer in the documentation and/or other materials provided
 *      with the distribution.
 *    * Neither the name of The Linux Foundation nor the names of its
 *      contributors may be used to endorse or promote products derived
 *      from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 *  ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 *  BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 *  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 *  WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 *  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * This is a test application to register and receive EFS related events
 * The application can run in
 * (1) Listen mode, where the registration to the notifications are automatically done
 * (2) Console mode, where the registration and deregistration to the notifications can be
 * controlled via console
 */

#include <getopt.h>
#include <iostream>
#include <vector>
#include <csignal>

extern "C" {
#include "unistd.h"
}

#include "FileSystemCommandMgr.hpp"
#include "FileSystemTestApp.hpp"
#include "Utils.hpp"

void FileSystemTestApp::printHelp() {

    std::cout << "Usage: " << APP_NAME << " options" << std::endl;
    std::cout << "   -l --listen            : listen to EFS restore operation updates" << std::endl;
    std::cout << "   -c --console-mode      : open interactive console" << std::endl;
    std::cout << "   -h --help              : print the help menu" << std::endl;
}

Status FileSystemTestApp::parseArguments(int argc, char **argv) {
    int arg;
    while (1) {
        static struct option long_options[] = {{"listen", no_argument, 0, 'l'},
            {"console-mode", no_argument, 0, 'c'}, {"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};
        int opt_index = 0;
        arg = getopt_long(argc, argv, "lch", long_options, &opt_index);
        if (arg == -1) {
            break;
        }
        switch (arg) {
            case 'l':
                command_ = "listen";
                break;
            case 'c':
                command_ = "console";
                break;
            case 'h':
                printHelp();
                break;
            default:
                printHelp();
                return Status::INVALIDPARAM;
        }
    }
    return Status::SUCCESS;
}

void FileSystemTestApp::handleArguments() {
    if (command_ == "listen") {
        myFsCmdMgr_->registerForUpdates();
        std::unique_lock<std::mutex> lock(mtx_);
        std::cout << APP_NAME << " Press CTRL+C to exit" << std::endl;
        cv_.wait(lock, [this]() { return exiting_; });
        myFsCmdMgr_->deregisterForUpdates();
    }
    if (command_ == "console") {
        consoleinit();
        mainLoop();
    }
    return;
}

FileSystemTestApp::FileSystemTestApp()
   : ConsoleApp("FileSystem Management Menu", "fs-mgmt> ")
   , myFsCmdMgr_(nullptr)
   , command_("") {
}

FileSystemTestApp::~FileSystemTestApp() {
}

FileSystemTestApp &FileSystemTestApp::getInstance() {
    static FileSystemTestApp instance;
    return instance;
}

void signalHandler(int signum) {
    FileSystemTestApp::getInstance().signalHandler(signum);
}

void FileSystemTestApp::signalHandler(int signum) {
    std::unique_lock<std::mutex> lock(mtx_);
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    exiting_ = true;
    cv_.notify_all();
}

int FileSystemTestApp::init() {
    myFsCmdMgr_ = std::make_shared<FileSystemCommandMgr>();
    int rc = myFsCmdMgr_->init();
    if (rc) {
        return -1;
    }
    return 0;
}

void FileSystemTestApp::consoleinit() {
    std::shared_ptr<ConsoleAppCommand> registerForUpdatesCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Register restore indications",
            {}, std::bind(&FileSystemCommandMgr::registerForUpdates, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> deRegisterForUpdatesCmd
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Disable restore indications",
            {}, std::bind(&FileSystemCommandMgr::deregisterForUpdates, myFsCmdMgr_)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> commandsListFilesMenu
        = {registerForUpdatesCmd, deRegisterForUpdatesCmd};
    ConsoleApp::addCommands(commandsListFilesMenu);
    ConsoleApp::displayMenu();
}

/**
 * Main routine
 */
int main(int argc, char **argv) {
    Status ret = Status::FAILED;
    // Setting required secondary groups for SDK file/diag logging
    std::vector<std::string> supplementaryGrps{"system", "diag"};
    int rc = Utils::setSupplementaryGroups(supplementaryGrps);
    if (rc == -1) {
        std::cout << APP_NAME << "Adding supplementary groups failed!" << std::endl;
    }
    auto &FileSystemTest = FileSystemTestApp::getInstance();
    if (0 != FileSystemTest.init()) {
        std::cout << APP_NAME << " Failed to initialize the File system management service"
                  << std::endl;
        return -1;
    }
    signal(SIGINT, signalHandler);
    ret = FileSystemTest.parseArguments(argc, argv);
    if (ret != Status::SUCCESS) {
        return -1;
    }
    FileSystemTest.handleArguments();
    std::cout << "Exiting application..." << std::endl;
    return 0;
}