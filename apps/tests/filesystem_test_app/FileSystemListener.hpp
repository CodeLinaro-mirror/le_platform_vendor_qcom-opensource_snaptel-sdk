/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef FILESYSTEMLISTENER_HPP
#define FILESYSTEMLISTENER_HPP

#include <telux/platform/FsListener.hpp>

using namespace telux::platform;
using namespace telux::common;

class FileSystemListener : public telux::platform::IFsListener {
 public:
    void OnEfsRestoreEvent(EfsEventInfo event) override;
    void OnEfsBackupEvent(EfsEventInfo event) override;
    void OnFsOperationImminentEvent(uint32_t timeLeftToStart) override;
    void onServiceStatusChange(ServiceStatus status) override;

    FileSystemListener();
    ~FileSystemListener();

 private:
    void printEfsEvent(std::string type, EfsEventInfo eventInfo);
};

#endif  // FILESYSTEMLISTENER_HPP
