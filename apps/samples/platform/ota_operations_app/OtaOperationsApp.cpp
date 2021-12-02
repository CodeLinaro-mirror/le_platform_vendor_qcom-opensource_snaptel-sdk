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
 * to control file system operation for ota client. On filesystem operation response,
 * provided callback will execute.
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

class OtaOperationsListener : public IFsListener {
 public:
    // [6] Receive service status notifications
    virtual void onServiceStatusChange(ServiceStatus serviceStatus) override {
        PRINT_NOTIFICATION << "Ota operation service status: ";
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
    std::cout << "********* Ota operations sample app *********" << std::endl;

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
    std::shared_ptr<OtaOperationsListener> otaOperationsListener
        = std::make_shared<OtaOperationsListener>();
    fsManager->registerListener(otaOperationsListener);

    // [6] Download the new package

    // [7] Prepare for ota start
    {
        Status otaStartStatus = Status::FAILED;
        OtaOperation otaOperation = OtaOperation::START;
        std::promise<ErrorCode> p;

        std::cout << "Request to prepare for ota start invoked " << std::endl;
        otaStartStatus = fsManager->prepareForOta(
            otaOperation, [&p, fsManager](ErrorCode error) { p.set_value(error); });

        if (otaStartStatus != Status::SUCCESS) {
            std::cout << "Request to prepare for ota start : ";
            Utils::printStatus(otaStartStatus);
            exit(1);
        } else {
            std::cout << "Request to prepare for ota start successful";
            ErrorCode error = p.get_future().get();
            std::cout << "Prepare for ota start with result: " << Utils::getErrorCodeAsString(error)
                      << std::endl;
        }
    }
    // [8] The client can start the OTA update

    // [9] Once the OTA is complete, indicate the update status to filesystem manager
    {
        Status otaEndStatus = Status::FAILED;
        OperationStatus operationStatus = OperationStatus::SUCCESS;
        std::promise<ErrorCode> p;

        std::cout << "Request to ota completion for update succeed invoked " << std::endl;
        otaEndStatus = fsManager->otaCompleted(
            operationStatus, [&p, fsManager](ErrorCode error) { p.set_value(error); });

        if (otaEndStatus != Status::SUCCESS) {
            std::cout << "Ota completion for update succeed request : ";
            Utils::printStatus(otaEndStatus);
            // Note: If OTA completion result in a failure, the client needs to start
            //      with prepareForOta
        } else {
            std::cout << "Ota completion for update succeed request successful";
            ErrorCode error = p.get_future().get();
            std::cout << " ota completed for update succeed with result: "
                      << Utils::getErrorCodeAsString(error) << std::endl;
        }
    }

    // [10] If user decides to mirror the filesystem, start AB sync
    {
        Status abSyncStatus = Status::FAILED;
        std::promise<ErrorCode> p;

        std::cout << "Request to start ABSYNC invoked " << std::endl;
        abSyncStatus
            = fsManager->startAbSync([&p, fsManager](ErrorCode error) { p.set_value(error); });

        if (abSyncStatus != Status::SUCCESS) {
            std::cout << "Request to start absync : ";
            Utils::printStatus(abSyncStatus);
        } else {
            std::cout << "Request to start absync successful";
            ErrorCode error = p.get_future().get();
            std::cout << "Start absync with result: " << Utils::getErrorCodeAsString(error)
                      << std::endl;
        }
    }

    std::cout << "\n\nPress ENTER to exit!!! \n\n";
    std::cin.ignore();

    // [11] Clean-up
    fsManager->deregisterListener(otaOperationsListener);
    otaOperationsListener = nullptr;
    fsManager = nullptr;

    return 0;
}
