/*
 *  Copyright (c) 2018-2021, The Linux Foundation. All rights reserved.
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


/*
 *  Changes from Qualcomm Innovation Center are provided under the following license:
 *
 *  Copyright (c) 2022-2023 Qualcomm Innovation Center, Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted (subject to the limitations in the
 * disclaimer below) provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *
 *     * Redistributions in binary form must reproduce the above
 *       copyright notice, this list of conditions and the following
 *       disclaimer in the documentation and/or other materials provided
 *       with the distribution.
 *
 *     * Neither the name of Qualcomm Innovation Center, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
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

 /**
  * @file: RadioInterface.h
  *
  * @brief: Simple application that queries C-V2X Status, interacts with
  *         telSDK and prints information to stdout
  */

#pragma once

#include <iostream>
#include <future>
#include <map>
#include <cassert>
#include <string.h>
#include <stdio.h>
#include <telux/cv2x/Cv2xRadio.hpp>
#include <cstdint>

using std::cout;
using std::cerr;
using std::endl;
using std::shared_ptr;
using std::map;
using std::promise;
using std::string;
using std::thread;
using telux::common::ErrorCode;
using telux::common::Status;
using telux::cv2x::Cv2xFactory;
using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::ICv2xRadio;
using telux::cv2x::Cv2xStatusEx;
using telux::cv2x::Cv2xStatusType;
using telux::cv2x::TrafficCategory;
using telux::cv2x::TrafficIpType;
using telux::cv2x::ICv2xRadioListener;

class Cv2xStatusListener;

typedef void (*v2x_src_l2_addr_update)(uint32_t newAddr);

enum class RadioType {
    TX,
    RX
};

/*
 * Communication related options for sending or receiving.
 */
typedef struct RadioOpt {
    bool enableUdp;
    string ipv4_src;
} RadioOpt_t;

class RadioInterface {

private:

    static map<Cv2xStatusType, string> gCv2xStatusToString;
    //static map<int, string> gCv2xReadyToString;

    /*
     * shared_ptr to the radio listener.
     */
    shared_ptr<ICv2xRadioListener> radioListener_ = nullptr;

    //variable to store cv2x status listener
    std::shared_ptr<Cv2xStatusListener> cv2xStatusListener_;

    /**
     * Method that the SDK uses for callbacks.
     * @param status a Cv2xStatus.
     * @param errorCode a ErrorCode.
     * @see Cv2xStatus
     */
    void cv2xStatusCallback(Cv2xStatusEx status, ErrorCode error);
    void updateSrcL2InfoCallback(ErrorCode error);

protected:

    bool enableCsvLog_ = false;

public:

    /*
    * shared_ptr to the singleton radio manager of the SDK.
    */
    promise<ErrorCode> gCallbackPromise = promise<ErrorCode>();

    /*
     * shared_ptr to the singleton radio manager of the SDK.
     */
    shared_ptr<ICv2xRadioManager> cv2xRadioManager = nullptr;


    /*
     * shared_ptr to the singleton radio of the SDK.
     */
    shared_ptr<ICv2xRadio> cv2xRadio = nullptr;

    /*
    * A Cv2xStatus that holds the radio status information.
    */
    Cv2xStatusEx gCv2xStatus;

    /*
     * Method to register callback for src L2 address updates.
     */
    int registerL2AddrCallback(v2x_src_l2_addr_update cb);

    /*
     * Method to deregister callback for src L2 address updates.
     */
    int deregisterL2AddrCallback(v2x_src_l2_addr_update cb);

    /*
     * Method to get V2X network interface name according to the traffic type.
     */
    int getV2xIfaceName(TrafficIpType type, string& ifName);

    /**
     * Method that clears the value in gCallbackPromise.
     * @see Cv2xStatus.
     */
    void resetCallbackPromise(void);

    /**
    * Non-blocking method that requests and returns TX/RX radio status.
    * @param type a RadioType.
    * @return String with possible values
    * "INACTIVE", "ACTIVE", "SUSPENDED", "UNKNOWN".
    */
    Cv2xStatusType statusCheck(RadioType type);
    /**
    * Blocking method that checks manager, radio and TX/RX status.
    * @param category a TrafficCategory.
    * @param type a RadioType.
    * @return bool
    * @see TrafficCategory
    */
    bool ready(TrafficCategory category, RadioType type);

    /**
    * Set the verbosity for the specific RadioInterface implementation.
    * @param interger value representing level of verbosity
    * @return none
    */
    void set_radio_verbosity(int value);
    int rVerbosity = 0;
    /**
     * @brief Register a listener for cv2x status change and wait if cv2x not active
     * @param type
     * @return true
     * @return false
     */
    void waitForCv2xToActivate(bool &haltRx);
    bool restartFlow;

    telux::cv2x::Cv2xStatus getCurrentStatus();

    /**
    * Method that requests src L2 address update.
    * @return bool
    */
    bool updateSrcL2();

    /**
    * Method to get latest cv2x channel busy ratio value.
    * @return uint8_t, 255 means invalid
    */
    virtual uint8_t getCBRValue();

    /**
    * Method to get latest radio Tx message monotonic time.
    * @return uint64_t, 0 indicate this radio interface has not trasmitted any message yet
    */
    virtual uint64_t latestTxRxTimeMonotonic();

    virtual void enableCsvLog(bool enable);
};

