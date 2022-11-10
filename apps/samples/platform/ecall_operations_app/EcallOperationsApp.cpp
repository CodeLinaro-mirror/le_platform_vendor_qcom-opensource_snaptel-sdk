/*
 *  Copyright (c) 2022 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted (subject to the limitations in the
 *  disclaimer below) provided that the following conditions are met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials provided
 *        with the distribution.
 *
 *      * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *        contributors may be used to endorse or promote products derived
 *        from this software without specific prior written permission.
 *
 *  NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE
 *  GRANTED BY THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT
 *  HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
 *  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 *  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 *  ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 *  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 *  GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 *  IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 *  OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 *  IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/**
 * This sample application demonstrates the use of the filesystem manager (IFsManager)
 * to control file system operation for ecall client.
 */

#include <future>
#include <getopt.h>
#include <iostream>
#include <condition_variable>

#include "../../../common/utils/Utils.hpp"
#include <telux/platform/PlatformFactory.hpp>

#define PRINT_NOTIFICATION std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::common;
using namespace telux::platform;

class EcallOperationListener : public IFsListener {
 public:
    // [6] Receive service status notifications
    virtual void onServiceStatusChange(ServiceStatus serviceStatus) override {
        PRINT_NOTIFICATION << "Ecall operation service status: ";
        std::string status;
        switch (serviceStatus) {
            case ServiceStatus::SERVICE_AVAILABLE: {
                status = "Available";
                break;
            }
            case ServiceStatus::SERVICE_UNAVAILABLE: {
                status = "Unavailable";
                break;
            }
            case ServiceStatus::SERVICE_FAILED: {
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
};

int main(int argc, char **argv) {
    std::cout << "********* Ecall operation sample app *********" << std::endl;

    // [1] Get platform factory
    auto &platformFactory = PlatformFactory::getInstance();

    // [2] Prepare a callback that is invoked when the filesystem sub-system
    // initialization is complete
    std::promise<ServiceStatus> p;
    auto initCb = [&p](ServiceStatus status) {
        std::cout << "Received service status: " << static_cast<int>(status) << std::endl;
        p.set_value(status);
    };

    // [3] Get the filesystem manager
    std::shared_ptr<IFsManager> fsManager = platformFactory.getFsManager(initCb);
    if (fsManager == nullptr) {
        std::cout << "filesystem manager is nullptr" << std::endl;
        exit(1);
    }
    std::cout << "Obtained filesystem manager" << std::endl;

    // [4] Wait until initialization is complete
    p.get_future().get();
    if (fsManager->getServiceStatus() != ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "Filesystem service not available" << std::endl;
        exit(1);
    }
    std::cout << "Filesystem service is now available" << std::endl;

    // [5] Create the listener object and register as a listener
    std::shared_ptr<EcallOperationListener> ecallOperationListener
        = std::make_shared<EcallOperationListener>();
    fsManager->registerListener(ecallOperationListener);

    // [7] Before starting the eCall, prepare the filesystem manager for the eCall
    // Note: It is recommended to start the eCall even if the request to prepare
    // for the eCall fails.this API can re-invoke while eCall is ongoing.
    {
        Status ecallStartStatus = Status::FAILED;
        std::cout << "Request to prepare for eCall start invoked " << std::endl;

        ecallStartStatus = fsManager->prepareForEcall();

        if (ecallStartStatus == Status::SUCCESS) {
            std::cout << "Request to prepare for eCall start successful";
        } else {
            std::cout << "Request to prepare for eCall start failed : ";
            Utils::printStatus(ecallStartStatus);
        }
    }

    // [8] Initiate the eCall

    // [9] Once the eCall is completed, indicate to the filesystem manager
    {
        Status ecallEndStatus = Status::FAILED;
        std::cout << "eCall completion request being invoked " << std::endl;

        ecallEndStatus = fsManager->eCallCompleted();

        if (ecallEndStatus == Status::SUCCESS) {
            std::cout << "eCall completion request successful";
        } else {
            std::cout << "eCall completion request failed : ";
            Utils::printStatus(ecallEndStatus);
        }
    }

    std::cout << "\n\nPress ENTER to exit!!! \n\n";
    std::cin.ignore();

    // [11] Clean-up
    fsManager->deregisterListener(ecallOperationListener);
    ecallOperationListener = nullptr;
    fsManager = nullptr;

    return 0;
}
