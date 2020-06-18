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
 * @file: Cv2xRetrieveConfigurationApp.cpp
 *
 * @brief: Application that  attempts to retrieve configuration file.
 */

#include <iostream>
#include <future>
#include <string>
#include <mutex>
#include <memory>

#include <telux/cv2x/Cv2xConfig.hpp>
#include <telux/cv2x/Cv2xFactory.hpp>

using std::cout;
using std::endl;
using std::cin;
using std::promise;
using std::string;
using std::mutex;
using std::make_shared;
using std::shared_ptr;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;

static promise<ErrorCode> gCallbackPromise;

static void cv2xRetrieveConfigurationCallback(ErrorCode error) {
    if (ErrorCode::SUCCESS != error) {
        cout << "Config file retrieve failed with code: " << static_cast<int>(error) << endl;
    }
    gCallbackPromise.set_value(error);
}

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X Retrieve Configuration app" << endl;

    // Get handle to Cv2xRadioManager
    auto & cv2xFactory = Cv2xFactory::getInstance();

    string configFilePath = "";
    gCallbackPromise = std::promise<ErrorCode>();

    cout << "Enter absolute config file path with filename: ";
    cin >> configFilePath;

    auto cv2xConfig = cv2xFactory.getCv2xConfig();

    // Wait for radio manager to complete initialization
    if (not cv2xConfig->isReady()) {
        if (!cv2xConfig->onReady().get()) {
            cout << "Error : C-V2X Radio Manager initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }

    /* Attempt config file retrieval */
    cout << "Retrieve configuration into file: " << configFilePath << endl;
    if (Status::SUCCESS !=
        cv2xConfig->retrieveConfiguration(configFilePath,
                                          cv2xRetrieveConfigurationCallback)) {
        cout << "Error : Config file retrieval failed." << endl;
        return EXIT_FAILURE;
    }

    auto res = gCallbackPromise.get_future().get();
    if (ErrorCode::SUCCESS != res) {
        cout << "Error : Config file retrieve failed with code: "
            << static_cast<int>(res) << "." << endl;
        return EXIT_FAILURE;
    }

    cout << "Configuration retrieval successful." << endl;
    return EXIT_SUCCESS;
}
