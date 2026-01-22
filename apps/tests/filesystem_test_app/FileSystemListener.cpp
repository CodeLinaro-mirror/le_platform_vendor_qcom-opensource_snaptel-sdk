/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

/**
 * This file hosts the implemenation of the FileSystemListener class, which is notified of
 * file system events in the platform
 */

#include <iostream>

#include "FileSystemListener.hpp"
#include "Utils.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

FileSystemListener::FileSystemListener() {
}

FileSystemListener::~FileSystemListener() {
}

void FileSystemListener::printEfsEvent(std::string type, EfsEventInfo eventInfo) {
    EfsEvent event                 = eventInfo.event;
    telux::common::ErrorCode error = eventInfo.error;
    PRINT_NOTIFICATION << type;
    if (event == EfsEvent::START) {
        std::cout << ": START" << std::endl;
    } else if (event == EfsEvent::END) {
        std::cout << ": END with ErrorCode: " << Utils::getErrorCodeAsString(error) << std::endl;
    } else {
        std::cout << APP_NAME << " ERROR: Invalid EFS restore event notified" << std::endl;
    }
}

void FileSystemListener::onServiceStatusChange(ServiceStatus status) {
    std::cout << std::endl;
    if (status == ServiceStatus::SERVICE_UNAVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : UNAVAILABLE" << std::endl;
    } else if (status == ServiceStatus::SERVICE_AVAILABLE) {
        PRINT_NOTIFICATION << "Service Status : AVAILABLE" << std::endl;
    }
}

void FileSystemListener::OnEfsRestoreEvent(EfsEventInfo event) {
    std::cout << std::endl;
    printEfsEvent("Restore EFS", event);
}

void FileSystemListener::OnEfsBackupEvent(EfsEventInfo event) {
    std::cout << std::endl;
    printEfsEvent("Backup EFS", event);
}

void FileSystemListener::OnFsOperationImminentEvent(uint32_t timeLeftToStart) {
    std::cout << "Filesystem operation shall re-enable in seconds " << timeLeftToStart << std::endl;
}
