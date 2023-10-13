/*
 * Copyright (c) 2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *    * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *
 *   * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#include "DiagFileMethodMenu.hpp"

#define print_notification std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

#define PRINT_OPERATION_ERROR(error)                                        \
    if (error == telux::common::ErrorCode::SUCCESS) {                     \
        std::cout << "Operation completed successfully\n" << std::endl;   \
    } else {                                                              \
        std::cout << "Operation returned error "                          \
            << Utils::getErrorCodeAsString(error) << "\n" << std::endl;   \
    }

DiagFileMethodMenu::DiagFileMethodMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DiagFileMethodMenu::~DiagFileMethodMenu() {
    if (diagLogManager_) {
        diagLogManager_ = nullptr;
    }
}

bool DiagFileMethodMenu::init() {
    std::vector<std::pair<std::string, std::function<void(std::vector<std::string> &)>>>
        diagFileCommandPairList = {
        std::make_pair("get_status",
            std::bind(&DiagFileMethodMenu::getStatus, this, std::placeholders::_1)) ,
        std::make_pair("Set_config",
            std::bind(&DiagFileMethodMenu::setConfig, this, std::placeholders::_1)) ,
        std::make_pair("Get_Config",
            std::bind(&DiagFileMethodMenu::getFileMethodConfig, this))          ,
        std::make_pair("Start_Draining_Logs",
            std::bind(&DiagFileMethodMenu::startDrainingLogs, this))                ,
        std::make_pair("Stop_Draining_Logs",
            std::bind(&DiagFileMethodMenu::stopDrainingLogs, this))                 ,
        std::make_pair("Start_Log_Collection",
            std::bind(&DiagFileMethodMenu::startLogCollection, this))               ,
        std::make_pair("Stop_Log_Collection",
            std::bind(&DiagFileMethodMenu::stopLogCollection, this))                ,
    };

    std::vector<std::shared_ptr<ConsoleAppCommand>> diagFileMenuCommandList;
    int commandId = 1;
    for(auto& menuItem: diagFileCommandPairList) {
        diagFileMenuCommandList.push_back(
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandId),
            menuItem.first, {}, menuItem.second)));
        commandId++;
    }
    addCommands(diagFileMenuCommandList);
    if (initDiag()) {
        ConsoleApp::displayMenu();
    }
    return true;
}

bool DiagFileMethodMenu::initDiag() {
    telux::common::ServiceStatus subSystemStatus = telux::common::ServiceStatus::SERVICE_FAILED;
    if (diagLogManager_ == nullptr) {
        std::promise<telux::common::ServiceStatus> prom{};

        // Get the DiagnosticsFactory instance.
        auto &diagnosticsFactory = telux::platform::diag::DiagnosticsFactory::getInstance();

        diagLogManager_ = diagnosticsFactory.getDiagLogManager(
            [&prom](telux::common::ServiceStatus status) { prom.set_value(status); });

        if (diagLogManager_ == nullptr) {
            //Return immediately
            std::cout <<
                "\nError encountered in initializing Diag Log Manager" << std::endl;
            return false;
        }
        subSystemStatus = prom.get_future().get();
    }

    if (subSystemStatus == telux::common::ServiceStatus::SERVICE_AVAILABLE) {
        std::cout << "\nDiag Manager is ready" << std::endl;
        telux::common::Status status = diagLogManager_->registerListener(shared_from_this());
        if (status != telux::common::Status::SUCCESS) {
            std::cout << "Unable to register diag manager listener" << std::endl;
        }
    } else {
        std::cout << "\nDiag Manager is not ready" << std::endl;
        return false;
    }
    return true;
}

void DiagFileMethodMenu::getStatus(std::vector<std::string> userInput) {
    std::cout << "Get Diagnostic Status" << std::endl;
    telux::platform::diag::DiagStatus diagStatus = diagLogManager_->getStatus();
    std::cout << "Logging Method: ";
    if(diagStatus.logMethod == telux::platform::diag::LogMethod::FILE) {
        std::cout << "File";
    } else if(diagStatus.logMethod == telux::platform::diag::LogMethod::CALLBACK) {
        std::cout << "Callback";
    } else {
        std::cout << "None";
    }
    std::cout << std::endl;
    std::cout << "Logging " << ((diagStatus.isLoggingInProgress)?
        "is in Progress":"Not Started") << std::endl;
    std::cout << "Log Drain " << ((diagStatus.isLogDrainInProgress)?
        "is in Progress":"Not Started") << std::endl;
}

void DiagFileMethodMenu::setConfig(std::vector<std::string> userInput) {
    telux::common::ErrorCode error;
    int input = 0;
    std::cout << "Set File Method Config " << std::endl;
    telux::platform::diag::DiagConfig fileMethodCfg {};
    fileMethodCfg.method = telux::platform::diag::LogMethod::FILE;

    int usrIn = 0;
    std::cout << "Select logging level (0-Device, 1-Peripheral: ";
    std::cin >> usrIn;
    DiagUtils::validateInput(usrIn, {0,1});
    std::cout << std::endl;
    if(usrIn == 0) {
        int dev = 0;
        std::cout << "Select device(s): bit 1 - MDM, bit 0 - EAP";
        std::cout << " (Ex: 3 will select both MDM and EAP): ";
        std::cin >> dev;
        DiagUtils::validateInput(dev, {1,2,3});
        fileMethodCfg.srcType = telux::platform::diag::SourceType::DEVICE;
        fileMethodCfg.srcInfo.device = static_cast<uint8_t>(dev);
    } else {
        std::cout << "Select peripheral(s): bit 2 - All SVMs, bit 1 - Modem_DSP,";
        std::cout << " bit 0 - Integrated_AP (Ex: 2 will select Modem DSP only): ";
        std::cin >> input;
        DiagUtils::validateInput(input, {0,1,2,3,4,5,6,7});
        fileMethodCfg.srcType = telux::platform::diag::SourceType::PERIPHERAL;
        DiagUtils::convertIntToPeripheral(input, fileMethodCfg.srcInfo.peripheral);
    }

    std::string str = "";
#ifdef TELUX_FOR_EXTERNAL_AP
    std::cout << std::endl << "Enter absolute path to EAP log mask file: ";
    std::cin >> str;
    fileMethodCfg.eapLogMaskFile = "";
    fileMethodCfg.eapLogMaskFile = DiagUtils::validateString(str);
#endif
    str = "";
    std::cout << std::endl << "Enter absolute path to MDM log mask file: ";
    std::cin >> str;
    fileMethodCfg.mdmLogMaskFile = DiagUtils::validateString(str);
    std::cout << std::endl
        << "Enter the log mode: 0-Streaming(default), 1-Threshold, 2-Circular Buffer: ";
    std::cin >> input;
    DiagUtils::validateInput(input, {0,1,2,3});
    fileMethodCfg.modeType = DiagUtils::setMode(input);
    std::cout << std::endl << "Select the max size of each log file (0-default size 100 MB):";
    std::cin >> str;
    fileMethodCfg.methodConfig.fileConfig.maxSize = DiagUtils::validateNumber(str);
    std::cout << std::endl
        << "Select the max number of log files that can be created (0-default 100 files): ";
    std::cin >> str;
    fileMethodCfg.methodConfig.fileConfig.maxNumber = DiagUtils::validateNumber(str);
    error = diagLogManager_->setConfig(fileMethodCfg);
    PRINT_OPERATION_ERROR(error);
    std::cout << "\n";
}

void DiagFileMethodMenu::getFileMethodConfig() {
    std::cout << "Get Config " << std::endl;
    DiagUtils::printConfig(diagLogManager_->getConfig());
}

void DiagFileMethodMenu::startDrainingLogs() {
    telux::common::ErrorCode error;
    std::cout << "Start File Method Logs Draining " << std::endl;
    error = diagLogManager_->startDrainingLogs();
    PRINT_OPERATION_ERROR(error);
}

void DiagFileMethodMenu::stopDrainingLogs() {
    telux::common::ErrorCode error;
    std::cout << "Stop File Method Logs Draining " << std::endl;
    error = diagLogManager_->stopDrainingLogs();
    PRINT_OPERATION_ERROR(error);
}

void DiagFileMethodMenu::startLogCollection() {
    telux::common::ErrorCode error;
    std::cout << "start File Method Logs Collection " << std::endl;
    error = diagLogManager_->startLogCollection();
    PRINT_OPERATION_ERROR(error);
}

void DiagFileMethodMenu::stopLogCollection() {
    telux::common::ErrorCode error;
    std::cout << "stop File Method Logs Collection " << std::endl;
    error = diagLogManager_->stopLogCollection();
    PRINT_OPERATION_ERROR(error);
}
