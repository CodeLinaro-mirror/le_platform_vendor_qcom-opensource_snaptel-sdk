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
 * This sample application demonstrates the use of the filesystem manager (IFsManager) to start
 * EFS backup and receive filesystem events like start or end of EFS restore and backup events
 */

#include <future>
#include <getopt.h>
#include <iostream>
#include <condition_variable>

#include "../../../common/utils/Utils.hpp"
#include <telux/platform/PlatformFactory.hpp>

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m"

class EfsEventListener : public telux::platform::IFsListener {
 public:
    // [6] Receive service status notifications
    virtual void onServiceStatusChange(telux::common::ServiceStatus serviceStatus) override {
        PRINT_NOTIFICATION << "Filesystem service status: ";
        std::string status;
        switch (serviceStatus) {
            case telux::common::ServiceStatus::SERVICE_AVAILABLE: {
                status = "Available";
                break;
            }
            case telux::common::ServiceStatus::SERVICE_UNAVAILABLE: {
                status = "Unavailable";
                break;
            }
            case telux::common::ServiceStatus::SERVICE_FAILED: {
                status = "Failed";
                break;
            }
            default: {
                status = "Unknown";
                break;
            }
        }
        std::cout << status << std::endl;
    }

    // [8] Receive EFS restore and backup notifications
    virtual void OnEfsRestoreEvent(telux::platform::EfsEventInfo event) override {
        PRINT_NOTIFICATION
            << ": Received efs event: Restore"
            << ((event.event == telux::platform::EfsEvent::START) ? " started" : "ended");
        if (event.event == telux::platform::EfsEvent::END) {
            std::cout << " with result: " << Utils::getErrorCodeAsString(event.error) << std::endl;
        }
    }

    virtual void OnEfsBackupEvent(telux::platform::EfsEventInfo event) override {
        PRINT_NOTIFICATION
            << ": Received efs event: Backup"
            << ((event.event == telux::platform::EfsEvent::START) ? " started" : "ended");
        if (event.event == telux::platform::EfsEvent::END) {
            std::cout << " with result: " << Utils::getErrorCodeAsString(event.error) << std::endl;
        }
    }
};

int main(int argc, char **argv) {
    std::cout << "********* efs backup restore sample app *********" << std::endl;

    // [1] Get platform factory
    auto &platformFactory = telux::platform::PlatformFactory::getInstance();

    // [2] Prepare a callback that is invoked when the filesystem sub-system initialization is
    // complete
    std::promise<telux::common::ServiceStatus> p;
    auto initCb = [&p](telux::common::ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
    };

    // [3] Get the filesystem manager
    std::shared_ptr<telux::platform::IFsManager> fsManager = platformFactory.getFsManager(initCb);
    if (fsManager == nullptr) {
        std::cout << "filesystem manager is nullptr" << std::endl;
        exit(1);
    }
    std::cout << "Obtained filesystem manager" << std::endl;

    // [4] Wait until initialization is complete
    p.get_future().get();
    if (fsManager->getServiceStatus() != telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Filesystem service not available" << std::endl;
        exit(1);
    }
    std::cout << "Filesystem service is now available" << std::endl;

    // [5] Create the listener object and register as a listener
    std::shared_ptr<EfsEventListener> efsEventListener = std::make_shared<EfsEventListener>();
    fsManager->registerListener(efsEventListener);

    // [7] Start EFS backup whenever necessary
    telux::common::Status status = fsManager->startEfsBackup();
    if (status != telux::common::Status::SUCCESS) {
        std::cout << "Unable to start EFS backup: ";
        Utils::printStatus(status);
    } else {
        std::cout << "Request to start EFS backup successful";
    }

    std::cout << "\n\nPress ENTER to exit!!! \n\n";
    std::cin.ignore();

    // [9] Clean-up
    fsManager->deregisterListener(efsEventListener);
    efsEventListener = nullptr;
    fsManager = nullptr;

    return 0;
}
