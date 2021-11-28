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
 * The file hosts the implementation of FileSystemCommandMgr class, whose responsibility is to
 * instantiate and interact with the file system manager
 */

#include <iostream>
#include <future>

#include "Utils.hpp"
#include "FileSystemCommandMgr.hpp"
#include "FileSystemListener.hpp"

FileSystemCommandMgr::FileSystemCommandMgr() {
}

FileSystemCommandMgr::~FileSystemCommandMgr() {
    fsMgr_ = nullptr;
}

int FileSystemCommandMgr::init() {
    // Get platform factory instance
    auto &platFormFactory = PlatformFactory::getInstance();
    // Get File System manager object
    std::promise<telux::common::ServiceStatus> prom = std::promise<telux::common::ServiceStatus>();
    fsMgr_ = platFormFactory.getFsManager(
        [&](telux::common::ServiceStatus status) { prom.set_value(status); });
    if (fsMgr_ == NULL) {
        std::cout << APP_NAME << " *** ERROR - Failed to get FileSystem manager" << std::endl;
        return -1;
    }
    // Check file system management service status
    std::cout << " Waiting for FileSystem manager to be ready " << std::endl;
    telux::common::ServiceStatus serviceStatus = prom.get_future().get();
    if (serviceStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << APP_NAME << " FileSystem manager is ready !" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR - Unable to initialize FileSystem manager"
                  << std::endl;
        return -1;
    }
    fsListener_ = std::make_shared<FileSystemListener>();
    registerForUpdates();
    return 0;
}

void FileSystemCommandMgr::registerForUpdates() {
    // Registering a listener for EFS operation updates
    telux::common::Status status = fsMgr_->registerListener(fsListener_);
    if ((status == telux::common::Status::SUCCESS) || (status == telux::common::Status::ALREADY)) {
        std::cout << APP_NAME << " Registered for EFS events" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR - Failed to register for EFS events: ";
        Utils::printStatus(status);
    }
}

void FileSystemCommandMgr::deregisterFromUpdates() {
    // De-registering a listener from EFS operation updates
    telux::common::Status status = fsMgr_->deregisterListener(fsListener_);
    if ((status == telux::common::Status::SUCCESS) || (status == telux::common::Status::NOSUCH)) {
        std::cout << APP_NAME << " Deregistered listener successfully" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR - Failed to deregister: " << std::endl;
        Utils::printStatus(status);
    }
}

void FileSystemCommandMgr::startEfsBackup() {
    std::cout << APP_NAME << ": Sending request to start EFS backup" << std::endl;
    telux::common::Status status = fsMgr_->startEfsBackup();
    if (status == telux::common::Status::SUCCESS) {
        std::cout << APP_NAME << " Backup request successful" << std::endl;
    } else {
        std::cout << APP_NAME << " *** ERROR - Backup request failed: ";
        Utils::printStatus(status);
    }
}