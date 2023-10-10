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

#include <ctime>
#include <chrono>
#include <sstream>
#include <iomanip>
#include "DiagCallbackMethodMenu.hpp"

#define print_notification std::cout << std::endl << "\033[1;35mNOTIFICATION: \033[0m" << std::endl

#define PRINT_OPERATION_ERROR(error)                                        \
    if (error == telux::common::ErrorCode::SUCCESS) {                     \
        std::cout << "Operation completed successfully\n" << std::endl;   \
    } else {                                                              \
        std::cout << "Operation returned error "                          \
            << Utils::getErrorCodeAsString(error) << "\n" << std::endl;   \
    }

DiagCallbackMethodMenu::DiagCallbackMethodMenu(std::string appName, std::string cursor)
   : ConsoleApp(appName, cursor) {
}

DiagCallbackMethodMenu::~DiagCallbackMethodMenu() {
    if (diagLogManager_) {
        diagLogManager_ = nullptr;
    }
}

bool DiagCallbackMethodMenu::init() {

    std::vector<std::pair<std::string, std::function<void(std::vector<std::string> &)>>>
        diagCallbackCommandPairList = {
        std::make_pair("get_status",
            std::bind(&DiagCallbackMethodMenu::getStatus, this, std::placeholders::_1)) ,
        std::make_pair("Set_config",
            std::bind(&DiagCallbackMethodMenu::setConfig, this, std::placeholders::_1)) ,
        std::make_pair("get_config",
            std::bind(&DiagCallbackMethodMenu::getCallbackMethodConfig, this))          ,
        std::make_pair("start_draining_logs",
            std::bind(&DiagCallbackMethodMenu::startDrainingLogs, this))                ,
        std::make_pair("stop_draining_logs",
            std::bind(&DiagCallbackMethodMenu::stopDrainingLogs, this))                 ,
        std::make_pair("start_log_collection",
            std::bind(&DiagCallbackMethodMenu::startLogCollection, this))               ,
        std::make_pair("stop_log_collection",
            std::bind(&DiagCallbackMethodMenu::stopLogCollection, this))                ,
    };

    std::vector<std::shared_ptr<ConsoleAppCommand>> diagCallbackMenuCommandList;
    int commandId = 1;
    for(auto& menuItem: diagCallbackCommandPairList) {
        diagCallbackMenuCommandList.push_back(
            std::make_shared<ConsoleAppCommand>(ConsoleAppCommand(std::to_string(commandId),
            menuItem.first, {}, menuItem.second)));
        commandId++;
    }
    addCommands(diagCallbackMenuCommandList);
    if (initDiag()) {
        ConsoleApp::displayMenu();
    }
    return true;
}


bool DiagCallbackMethodMenu::initDiag() {
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

void DiagCallbackMethodMenu::getStatus(std::vector<std::string> userInput) {
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

void DiagCallbackMethodMenu::setConfig(std::vector<std::string> userInput) {
    telux::common::ErrorCode error;
    int input = 0;
    std::string str;
    std::cout << "Set Callback Method Config" << std::endl;
    telux::platform::diag::DiagConfig cbMethodCfg {};
    cbMethodCfg.method = telux::platform::diag::LogMethod::CALLBACK;

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
        cbMethodCfg.srcType = telux::platform::diag::SourceType::DEVICE;
        cbMethodCfg.srcInfo.device = static_cast<uint8_t>(dev);
    } else {
        std::cout << "Select peripheral(s): bit 2 - All SVMs, bit 1 - Modem_DSP,";
        std::cout << " bit 0 - Integrated_AP (Ex: 2 will select Modem DSP only): ";
        std::cin >> input;
        DiagUtils::validateInput(input, {0,1,2,3,4,5,6,7});
        cbMethodCfg.srcType = telux::platform::diag::SourceType::PERIPHERAL;
        DiagUtils::convertIntToPeripheral(input, cbMethodCfg.srcInfo.peripheral);
    }

#ifdef TELUX_FOR_EXTERNAL_AP
    std::cout << std::endl << "Enter absolute path to EAP log mask file: ";
    std::cin >> str;
    cbMethodCfg.eapLogMaskFile = DiagUtils::validateString(str);
#endif
    str = "";
    std::cout << std::endl << "Enter absolute path to MDM log mask file: ";
    std::cin >> str;
    cbMethodCfg.mdmLogMaskFile = DiagUtils::validateString(str);
    std::cout << std::endl << "Enter the log mode: 0-Streaming(default), 1-Threshold, "
    <<"2-Circular Buffer: ";
    std::cin >> input;
    DiagUtils::validateInput(input, {0,1,2,3});
    cbMethodCfg.modeType = DiagUtils::setMode(input);
    error = diagLogManager_->setConfig(cbMethodCfg);
    PRINT_OPERATION_ERROR(error);
    std::cout << "\n";
}

void DiagCallbackMethodMenu::getCallbackMethodConfig() {
    std::cout << "Get Config" << std::endl;
    DiagUtils::printConfig(diagLogManager_->getConfig());
}

void DiagCallbackMethodMenu::startDrainingLogs() {
    telux::common::ErrorCode error;
    std::cout << "Start Callback Method Logs Draining " << std::endl;
    error = diagLogManager_->startDrainingLogs();
    PRINT_OPERATION_ERROR(error);
}

void DiagCallbackMethodMenu::stopDrainingLogs() {
    telux::common::ErrorCode error;
    std::cout << "Stop Callback Method Logs Draining " << std::endl;
    error = diagLogManager_->stopDrainingLogs();
    PRINT_OPERATION_ERROR(error);
}

void DiagCallbackMethodMenu::startLogCollection() {
    telux::common::ErrorCode error;
    std::cout << "start Callback Method Logs Collection " << std::endl;
    std::string logPath {};
    std::cout << "Enter path to save log file (without quotes): ";
    std::cin >> logPath;
    std::cout << std::endl;
    struct stat sb;
    if (stat(logPath.c_str(), &sb) != 0) {
        std::cout << "The Path does not exist!. Please create path\n";
        return;
    }
    if(logPath[logPath.length()-1] != '/') {
         logPath += '/';
    }

    std::tm tmSnapshot;
    std::stringstream ss;
    std::time_t nowTime = std::time(nullptr);
    auto localTime = ::localtime_r(&nowTime, &tmSnapshot);
    if(localTime) {
        ss << std::put_time(localTime, "%b-%d-%Y-%H-%M-%S");
    } else {
        ss << "0-0-0-0-0-0";
    }
    std::cout << "Time stamp is: " << ss.str() << std::endl;

    logPath += "DiagCbLogs-" + ss.str() + ".qmdl";
    diagCbLogFile_.open(logPath.c_str(), std::ios::out | std::ios::app);

    error = diagLogManager_->startLogCollection();
    PRINT_OPERATION_ERROR(error);
}

void DiagCallbackMethodMenu::stopLogCollection() {
    telux::common::ErrorCode error;
    std::cout << "stop Callback Method Logs Collection " << std::endl;
    error = diagLogManager_->stopLogCollection();
    PRINT_OPERATION_ERROR(error);
    diagCbLogFile_.close();

}

void DiagCallbackMethodMenu::onAvailableLogs(uint8_t* ptr, int len) {
    print_notification << "Logs received" << std::endl;
    if (ptr) {
        for ( int i = 0; i < len; i++) {
            diagCbLogFile_ << ptr[i];
        }
        diagCbLogFile_ << std::endl;
    }
}
