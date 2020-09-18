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
 * @file: Cv2xSetTxPowerApp.cpp
 *
 * @brief: Application that set global override of CV2X Tx-power .
 */

#include <iostream>
#include <future>
#include <string>
#include <mutex>
#include <memory>

#include <telux/cv2x/Cv2xFactory.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

using std::cout;
using std::endl;
using std::cin;
using std::promise;

using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;

#define CV2X_TX_POWER_MAX (23)
#define CV2X_TX_POWER_MIN (-40)

int main(int argc, char *argv[]) {
    cout << "Running Sample C-V2X Set Tx-power app" << endl;
    int txPower = CV2X_TX_POWER_MAX;
    telux::common::Status ret;

    cout << "Enter desired global cv2x Tx peak power:";
    cin >> txPower;
    if (txPower > CV2X_TX_POWER_MAX || txPower < CV2X_TX_POWER_MIN) {
        cout << "Illegal tx power value " << txPower << " input, abort!" << endl;
        return EXIT_FAILURE;
    }

    cout << "Desired tx power " << txPower << endl;

    auto & cv2xFactory = Cv2xFactory::getInstance();
    auto cv2xRadioMgr = cv2xFactory.getCv2xRadioManager();
    if (not cv2xRadioMgr) {
        cout << "Error get cv2x radio manger" << endl;
        return EXIT_FAILURE;
    }

    // Wait for radio manager to complete initialization
    if (not cv2xRadioMgr->isReady()) {
        if (!cv2xRadioMgr->onReady().get()) {
            cout << "Error : C-V2X Radio Manager initialization failed" << endl;
            return EXIT_FAILURE;
        }
    }

    promise<ErrorCode> p;
    ret = cv2xRadioMgr->setPeakTxPower(static_cast<int8_t>(txPower), [&p](ErrorCode error) {
        if (ErrorCode::SUCCESS != error) {
            cout << "Set Cv2x Tx Power fail, error code " << static_cast<int>(error) << endl;
        }
        p.set_value(error);
    });
    if (telux::common::Status::SUCCESS == ret && ErrorCode::SUCCESS == p.get_future().get()) {
        cout << "success set_peak_tx_power " << txPower << endl;
    }

    return EXIT_SUCCESS;
}
