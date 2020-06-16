/*
 *  Copyright (c) 2019-2020, The Linux Foundation. All rights reserved.
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

#ifndef ECALLAPP_HPP
#define ECALLAPP_HPP

#include <memory>
#include <string>
#include <vector>
#include <mutex>

#include "ConsoleApp.hpp"
#include "ECallManager.hpp"

/**
 * ECallApp class provides an user-interactive console to trigger an eCall and answer an incoming
 * call(typically a PSAP callback).
 */
class ECallApp : public ConsoleApp {
public:

    /**
     * Get an instance of ECallApp
     */
    static ECallApp &getInstance();

    /**
     * Initialize the subsystems, console commands and display the menu.
     */
    void init();

    /**
     * Hangs up a triggered eCall and gracefully clears down the subsystems.
     */
    void cleanup();

private:

    ECallApp(std::string appName, std::string cursor);
    ~ECallApp();

    /**
     * Trigger a standard eCall using the emergency number configured in FDN (eg.112)
     */
    void makeECall();

    /**
     * Trigger a Voice eCall to the specified phone number
     */
    void makeCustomNumberECall();

    /**
     * Answer an incoming call
     */
    void answerIncomingCall();

    /**
     * Hangup the ongoing call
     */
    void hangupCall();

    /**
     * Dump the list of calls in progress
     */
    void getCalls();

    /**
     * Request eCall High Level Application Protocol(HLAP) timers status
     */
    void requestECallHlapTimerStatus();

    /**
     * Function to get phoneId from the user-interface
     */
    int getPhoneId();

    /**
     * Function to get eCall category from the user-interface
     */
    int getEcallCategory(telux::tel::ECallCategory &emergencyCategory);

    /**
     * Function to configure MSD transmission at call connect
     */
    telux::common::Status getMsdTransmissionConfig(bool &transmitMsd);

    // Member variable to keep the eCall manager object alive until the application quits.
    std::shared_ptr<ECallManager> eCallMgr_;
};

#endif  // ECALLAPP_HPP
