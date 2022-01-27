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

std::shared_ptr<FileSystemTestApp> fileSystemTestApp_ = nullptr;

void FileSystemTestApp::printHelp() {

    std::cout << "Usage: " << APP_NAME << " options" << std::endl;
    std::cout << "   -h --help              : print the help menu" << std::endl;
}

Status FileSystemTestApp::parseArguments(int argc, char **argv) {
    int arg;
    while (1) {
        static struct option long_options[] = {{"help", no_argument, 0, 'h'}, {0, 0, 0, 0}};
        int opt_index = 0;
        arg = getopt_long(argc, argv, "h", long_options, &opt_index);
        if (arg == -1) {
            break;
        }
        switch (arg) {
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

FileSystemTestApp::FileSystemTestApp()
   : ConsoleApp("FileSystem Management Menu", "fs-mgmt> ")
   , myFsCmdMgr_(nullptr) {
}

FileSystemTestApp::~FileSystemTestApp() {
    if (myFsCmdMgr_) {
        myFsCmdMgr_->deregisterFromUpdates();
    }
    myFsCmdMgr_ = nullptr;
}

void signalHandler(int signum) {
    fileSystemTestApp_->signalHandler(signum);
}

void FileSystemTestApp::signalHandler(int signum) {
    std::cout << APP_NAME << " Interrupt signal (" << signum << ") received.." << std::endl;
    cleanup();
    exit(1);
}

int FileSystemTestApp::init() {
    myFsCmdMgr_ = std::make_shared<FileSystemCommandMgr>();
    int rc = myFsCmdMgr_->init();
    if (rc) {
        return -1;
    }
    return 0;
}

void FileSystemTestApp::cleanup() {
    if (myFsCmdMgr_) {
        myFsCmdMgr_->deregisterFromUpdates();
    }
    myFsCmdMgr_ = nullptr;
}

void FileSystemTestApp::consoleinit() {
    std::shared_ptr<ConsoleAppCommand> startEfsBackupCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("1", "Start_Efs_Backup", {},
            std::bind(&FileSystemCommandMgr::startEfsBackup, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> prepareForEcallCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("2", "Prepare_For_Ecall", {},
            std::bind(&FileSystemCommandMgr::prepareForEcall, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> eCallCompletedCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("3", "ECall_Completed", {},
            std::bind(&FileSystemCommandMgr::eCallCompleted, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> prepareForOtaStartCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("4", "Prepare_For_Ota_Start", {},
            std::bind(&FileSystemCommandMgr::prepareForOtaStart, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> otaCompletedCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "5", "Ota_Completed", {}, std::bind(&FileSystemCommandMgr::otaCompleted, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> prepareForOtaResumeCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand("6", "Prepare_For_Ota_Resume", {},
            std::bind(&FileSystemCommandMgr::prepareForOtaResume, myFsCmdMgr_)));

    std::shared_ptr<ConsoleAppCommand> startAbSyncCommand
        = std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(
            "7", "Start_AbSync", {}, std::bind(&FileSystemCommandMgr::startAbSync, myFsCmdMgr_)));

    std::vector<std::shared_ptr<ConsoleAppCommand>> fileSystemTestAppCommands
        = {startEfsBackupCommand, prepareForEcallCommand, eCallCompletedCommand,
            prepareForOtaStartCommand, otaCompletedCommand, prepareForOtaResumeCommand,
            startAbSyncCommand};

    ConsoleApp::addCommands(fileSystemTestAppCommands);
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
    fileSystemTestApp_ = std::make_shared<FileSystemTestApp>();
    if (0 != fileSystemTestApp_->init()) {
        std::cout << APP_NAME << " Failed to initialize the File system management service"
                  << std::endl;
        return -1;
    }
    signal(SIGINT, signalHandler);
    ret = fileSystemTestApp_->parseArguments(argc, argv);
    if (ret != Status::SUCCESS) {
        return -1;
    }
    fileSystemTestApp_->consoleinit();
    fileSystemTestApp_->mainLoop();
    std::cout << "Exiting application..." << std::endl;
    fileSystemTestApp_->cleanup();
    fileSystemTestApp_ = nullptr;
    return 0;
}
