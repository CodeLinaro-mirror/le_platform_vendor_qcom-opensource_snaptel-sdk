/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef FILESYSTEMCOMMANDMGR_HPP
#define FILESYSTEMCOMMANDMGR_HPP

#include <memory>
#include <string>
#include <sstream>

#include <telux/platform/PlatformFactory.hpp>

class FileSystemListener;

class FileSystemCommandMgr : public std::enable_shared_from_this<FileSystemCommandMgr> {
 public:
    FileSystemCommandMgr();
    ~FileSystemCommandMgr();

    int init();
    void registerForUpdates();
    void deregisterFromUpdates();

    void startEfsBackup();
    void prepareForEcall();
    void eCallCompleted();
    void prepareForOtaStart();
    void otaCompleted();
    void prepareForOtaResume();
    void startAbSync();

 private:
    std::shared_ptr<telux::platform::IFsManager> fsMgr_;
    std::shared_ptr<FileSystemListener> fsListener_;
    template <typename T>
    static void getInput(std::string prompt, T &input) {
        std::cout << prompt;
        std::string line;
        std::getline(std::cin, line);
        std::stringstream ss(line);
        ss >> input;
        bool valid = false;
        do {
            if (!ss.bad() && ss.eof() && !ss.fail()) {
                valid = true;
            } else {
                // If an error occurs then an error flag is set and future attempts to get
                // input will fail. Clear the error flag on cin.
                std::cin.clear();
                // Clear the string stream's states and buffer
                ss.clear();
                ss.str("");
                std::cout << "Invalid input, please re-enter" << std::endl;
                std::cout << prompt;
                std::getline(std::cin, line);
                ss << line;
                ss >> input;
            }
        } while (!valid);
    }
};
#endif  // FILESYSTEMCOMMANDMGR_HPP
