/*
 *  Copyright (c) 2020, The Linux Foundation. All rights reserved.
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
 * @file: Cv2xConfigApp.cpp
 *
 * @brief: Simple application that demonstrates Cv2x configuration relevant operations.
 */

#include <iostream>
#include <future>
#include <string>
#include <mutex>
#include <memory>
#include <fstream>
#include <ctime>
#include <iomanip>
#include <stdlib.h>
#include <atomic>

#include <telux/cv2x/Cv2xRadioTypes.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>
#include "Cv2xConfigApp.hpp"

using std::cout;
using std::endl;
using std::cin;
using std::getline;
using std::promise;
using std::string;
using std::mutex;
using std::make_shared;
using std::shared_ptr;
using std::ifstream;
using std::ofstream;
using std::time;
using std::setw;
using std::lock_guard;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ConfigEventInfo;
using telux::cv2x::ConfigEvent;
using telux::cv2x::Cv2xStatus;
using telux::cv2x::Cv2xStatusType;

static const string CONFIG_FILE("/var/tmp/v2x.xml");
static const string EXPIRY_FILE("/var/tmp/expiry.xml");

class ConfigListener : public ICv2xConfigListener {
public:
    void waitForConfigChangeEvent(ConfigEvent event) {
        //initialize the promise to ignore indications received before calling this API
        promiseSet_ = false;
        configPromise_ = promise<ConfigEvent>();

        while (event != configPromise_.get_future().get()) {
            // the recevied indication is not as expected, wait for the next indication
            configPromise_ = promise<ConfigEvent>();
            promiseSet_ = false;
        }
    }

    void onConfigChanged(const ConfigEventInfo & info) override {
        if (not promiseSet_) {
            promiseSet_ = true;
            configPromise_.set_value(info.event);
        }
    }

private:
    promise<ConfigEvent> configPromise_;
    std::atomic<bool> promiseSet_{false};
};

class Cv2xStatusListener : public ICv2xListener {
public:

    Cv2xStatusListener(Cv2xStatus status) {
        cv2xStatus_ = status;
    };

    Cv2xStatus getCurrentStatus() {
        lock_guard<mutex> lock(cv2xStatusMutex_);
        return cv2xStatus_;
    }

    void waitForCv2xStatus(Cv2xStatusType status) {
        // get the inital status
        Cv2xStatus tmpStatus;
        {
            lock_guard<mutex> lock(cv2xStatusMutex_);
            tmpStatus = cv2xStatus_;
        }

        while (tmpStatus.rxStatus != status or tmpStatus.txStatus != status) {
            // the initial status or the received status is not as expected,
            // wait for the next status change
            statusPromise_ = promise<Cv2xStatus>();
            promiseSet_ = false;
            tmpStatus = statusPromise_.get_future().get();
        }
    }

    void onStatusChanged(Cv2xStatus status) override {
        {
            lock_guard<mutex> lock(cv2xStatusMutex_);

            if (status.rxStatus != cv2xStatus_.rxStatus or
                status.txStatus != cv2xStatus_.txStatus) {
                cout << "Cv2x status updated, rxStatus:" << static_cast<int>(status.rxStatus);
                cout << ", txStatus:" << static_cast<int>(status.txStatus) << endl;
                cv2xStatus_ = status;
            } else {
                // no need set promise if status is not changed
                return;
            }
        }

        if (not promiseSet_) {
            promiseSet_ = true;
            statusPromise_.set_value(status);
        }
    }

private:
    promise<Cv2xStatus> statusPromise_;
    std::atomic<bool> promiseSet_{false};
    mutex cv2xStatusMutex_;
    Cv2xStatus cv2xStatus_;
};

Cv2xConfigApp::Cv2xConfigApp()
    : ConsoleApp("Cv2x Config Menu", "config> ") {
}

Cv2xConfigApp::~Cv2xConfigApp() {
   if(cv2xConfig_ and configListener_) {
      cv2xConfig_->deregisterListener(configListener_);
   }

   if(cv2xRadioManager_ and cv2xStatusListener_) {
      cv2xRadioManager_->deregisterListener(cv2xStatusListener_);
   }
}

int Cv2xConfigApp::initialize() {
    if (EXIT_SUCCESS != cv2xInit()) {
        return EXIT_FAILURE;
    }

    consoleInit();

    return EXIT_SUCCESS;
}

int Cv2xConfigApp::cv2xInit() {
    // get handle of cv2x config
    auto & cv2xFactory = Cv2xFactory::getInstance();
    cv2xConfig_ = cv2xFactory.getCv2xConfig();

    // Wait for cv2x config to complete initialization
    if (not cv2xConfig_->isReady()) {
        if (!cv2xConfig_->onReady().get()) {
            cout << "Error : Cv2x Config initialization failed!" << endl;
            return EXIT_FAILURE;
        }
    }

    // register listener for config change indications
    configListener_ = make_shared<ConfigListener>();
    if (Status::SUCCESS != cv2xConfig_->registerListener(configListener_)) {
        cout << "Error : register Cv2x config listener failed!" << endl;
        return EXIT_FAILURE;
    }

    // get handle of radio manager
    cv2xRadioManager_ = cv2xFactory.getCv2xRadioManager();

    // Wait for radio manager to complete initialization
    if (not cv2xRadioManager_->isReady()) {
        if (!cv2xRadioManager_->onReady().get()) {
            cout << "Error : Cv2x Radio Manager initialization failed!" << endl;
            return EXIT_FAILURE;
        }
    }

    // get initial Cv2x status
    promise<Cv2xStatus> prom;
    auto res = cv2xRadioManager_->requestCv2xStatus([&prom](Cv2xStatus status, ErrorCode code)
                                                    {
                                                        prom.set_value(status);
                                                    });
    if (Status::SUCCESS != res) {
        cout << "Error : Request for Cv2x status failed!" << endl;
        return EXIT_FAILURE;
    };
    auto status = prom.get_future().get();

    // register listener for cv2x status change
    cv2xStatusListener_ = make_shared<Cv2xStatusListener>(status);
    if (Status::SUCCESS != cv2xRadioManager_->registerListener(cv2xStatusListener_)) {
        cout << "Error : register Cv2x status listener failed!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Cv2xConfigApp::consoleInit() {
    shared_ptr<ConsoleAppCommand> retrieveCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "1", "Retrieve_Config", {},
        std::bind(&Cv2xConfigApp::retrieveConfigCommand, this)));

    shared_ptr<ConsoleAppCommand> updateCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "2", "Update_Config", {},
        std::bind(&Cv2xConfigApp::updateConfigCommand, this)));

    shared_ptr<ConsoleAppCommand> enforceExpirationCmd
        = make_shared<ConsoleAppCommand>(ConsoleAppCommand(
        "3", "Enforce_Config_Expiration", {},
        std::bind(&Cv2xConfigApp::enforceConfigExpirationCommand, this)));

    std::vector<shared_ptr<ConsoleAppCommand>> commandsList
        = {retrieveCmd, updateCmd, enforceExpirationCmd};
    ConsoleApp::addCommands(commandsList);
    ConsoleApp::displayMenu();
}

int Cv2xConfigApp::startCv2xMode() {
    auto sp = std::dynamic_pointer_cast<Cv2xStatusListener>(cv2xStatusListener_);
    if (!sp) {
        cout << "Error: Get cv2x status listener failed!" << endl;
        return EXIT_FAILURE;
    }

    // check if Cv2x is already started before running the test
    auto status = sp->getCurrentStatus();
    if (Cv2xStatusType::UNKNOWN == status.rxStatus or
        Cv2xStatusType::UNKNOWN == status.txStatus) {
        cout << "Error : Cv2x status is unknown!" << endl;
        return EXIT_FAILURE;
    }

    if (Cv2xStatusType::INACTIVE == status.rxStatus and
        Cv2xStatusType::INACTIVE == status.txStatus) {
        // need start cv2x later
        cout << "Cv2x is not started." << endl;
    } else if (Cv2xStatusType::ACTIVE == status.rxStatus and
        Cv2xStatusType::ACTIVE == status.txStatus) {
        cout << "Cv2x is started and active." << endl;
        return EXIT_SUCCESS;
    } else {
        // cv2x is started before running this tool but not ready
        cout << "Error: Cv2x is started already but status is not active!" << endl;
        return EXIT_FAILURE;
    }

    // start cv2x mode
    cout << "Starting Cv2x..." << endl;

    promise<ErrorCode> prom;
    if (Status::SUCCESS != cv2xRadioManager_->startCv2x(
        [&prom](ErrorCode error) { prom.set_value(error); })) {
        cout << "Error : Unable to start Cv2x mode!" << endl;
        return EXIT_FAILURE;
    }

    auto res = prom.get_future().get();
    if (ErrorCode::SUCCESS != res) {
        cout << "Error : Start Cv2x mode failed with error code: "
            << static_cast<int>(res) << "!" << endl;
        return EXIT_FAILURE;
    }
    stopCv2x_ = true; //need stop cv2x when exit

    // wait until status changes to active
    sp->waitForCv2xStatus(Cv2xStatusType::ACTIVE);

    return EXIT_SUCCESS;
}

int Cv2xConfigApp::stopCv2xMode() {
    cout << "Stopping Cv2x..." << endl;

    promise<ErrorCode> prom;
    if (Status::SUCCESS != cv2xRadioManager_->stopCv2x(
        [&prom](ErrorCode error) { prom.set_value(error); })) {
        cout << "Error : Unable to stop Cv2x mode!" << endl;
        return EXIT_FAILURE;
    }

    auto res = prom.get_future().get();
    if (ErrorCode::SUCCESS != res) {
        cout << "Error : Stop Cv2x mode failed with error: "
            << static_cast<int>(res) << "!" << endl;
        return EXIT_FAILURE;
    }

    auto sp = std::dynamic_pointer_cast<Cv2xStatusListener>(cv2xStatusListener_);
    if (sp) {
        // wait until status changes to inactive
        sp->waitForCv2xStatus(Cv2xStatusType::INACTIVE);
    }

    return EXIT_SUCCESS;
}

int Cv2xConfigApp::retrieveConfigFile(string path) {
    cout << "Retrieving config file..." << endl;

    promise<ErrorCode> prom;
    if (Status::SUCCESS != cv2xConfig_->retrieveConfiguration(path,
                                                              [&prom](ErrorCode code)
                                                              {
                                                                  prom.set_value(code);
                                                              })) {
        cout << "Error : Retrieve config file failed!" << endl;
        return EXIT_FAILURE;
    }

    auto res = prom.get_future().get();
    if (ErrorCode::SUCCESS != res) {
        cout << "Error : Retrieve config file failed with code: "
            << static_cast<int>(res) << "!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Cv2xConfigApp::retrieveConfigCommand() {
    string configFilePath;
    cout << "Enter absolute config file path with file name: ";
    getline(cin, configFilePath);

    if (EXIT_SUCCESS == retrieveConfigFile(configFilePath)) {
        cout << "Retrieve config file successfully." <<  endl;
    }

    displayMenu();
}

int Cv2xConfigApp::updateConfigFile(string path) {
    cout << "Updating config file..." << endl;

    promise<ErrorCode> prom;
    if (Status::SUCCESS != cv2xConfig_->updateConfiguration(path,
                                                            [&prom](ErrorCode code)
                                                            {
                                                                prom.set_value(code);
                                                            })) {
        cout << "Error : Update config file failed!" << endl;
        return EXIT_FAILURE;
    }

    auto res = prom.get_future().get();
    if (ErrorCode::SUCCESS != res) {
        cout << "Error : Update config file failed with code: "
            << static_cast<int>(res) << "!" << endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

void Cv2xConfigApp::updateConfigCommand() {
    string configFilePath;
    cout << "Enter absolute config file path and file name: ";
    getline(cin, configFilePath);

    if (EXIT_SUCCESS == updateConfigFile(configFilePath)) {
        cout << "Update config file successfully." <<  endl;
    }

    displayMenu();
}

int Cv2xConfigApp::generateExpiryConfigFile(string configFilePath, string expiryFilePath) {
    cout << "Generating expiry config file..." << endl;

    // print current timestamp
    std::cout << "Current timestamp:" << time(NULL) << endl;

    // add expiry item to config file
    string timestamp;
    cout << "Enter config expiry timestamp: ";
    getline(cin, timestamp);

    ifstream input(configFilePath);
    if (!input) {
        cout << "Error : open original config file failed!" << endl;
        return EXIT_FAILURE;
    }

    ofstream output(expiryFilePath);
    if (!output) {
        cout << "Error : open expiry config file failed!" << endl;
        return EXIT_FAILURE;
    }

    string line;
    input.unsetf(ifstream::skipws);

    while(!input.eof()) {
        getline(input, line);

        // not copy the line including Expiration tag
        if (string::npos != line.find("<Expiration>")) {
            continue;
        }
        output << line << '\n';

        if (string::npos != line.find("<V2XoverPC5>")) {
            // insert expiry item to the next line
            int len = sizeof("<Expiration>") + 3; // add whitespaces
            output << setw(len) << "<Expiration>" << timestamp << "</Expiration>" << '\n';
        }
    }

    cout << "Current timestamp:" << time(NULL) << endl;

    return EXIT_SUCCESS;
}

int Cv2xConfigApp::enforceConfigExpiration() {
    int ret = EXIT_SUCCESS;

    // cv2x must be active for the listening of config change indcations
    if (EXIT_SUCCESS != startCv2xMode()) {
        return EXIT_FAILURE;
    }

    // generate expiry config file based on the retrieved config file
    // and then update the exipry config file
    if (EXIT_SUCCESS == retrieveConfigFile(CONFIG_FILE) and
        EXIT_SUCCESS == generateExpiryConfigFile(CONFIG_FILE, EXPIRY_FILE) and
        EXIT_SUCCESS == updateConfigFile(EXPIRY_FILE)) {
        auto sp = std::dynamic_pointer_cast<ConfigListener>(configListener_);
        if (sp) {
            // wait until receiving config expiry indication
            cout << "Waiting for config expiry indication..." << endl;
            sp->waitForConfigChangeEvent(ConfigEvent::EXPIRED);

            // wait until receiving config changed indication
            cout << "Waiting for config changed indication..." << endl;
            sp->waitForConfigChangeEvent(ConfigEvent::CHANGED);
        } else {
            ret = EXIT_FAILURE;
        }
    } else {
        ret = EXIT_FAILURE;
    }

    // stop cv2x if not started originally
    if (stopCv2x_) {
        stopCv2x_ = false;
        if (EXIT_SUCCESS != stopCv2xMode()) {
            return EXIT_FAILURE;
        }
    }

    return ret;
}

void Cv2xConfigApp::enforceConfigExpirationCommand() {
    if (EXIT_SUCCESS == enforceConfigExpiration()) {
        cout << "Enforce expiration of Cv2x config successfully." << endl;
    }

    displayMenu();
}

int main(int argc, char *argv[]) {
    shared_ptr<Cv2xConfigApp> cv2xConfig = make_shared<Cv2xConfigApp>();
    if (EXIT_SUCCESS != cv2xConfig->initialize()){
        cout << "Error: Initialization failed!" << endl;
        return EXIT_FAILURE;
    }

    // continuously read and execute commands
    return cv2xConfig->mainLoop();
}
