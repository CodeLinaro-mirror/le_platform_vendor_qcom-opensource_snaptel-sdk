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

#ifndef CV2XDAEMON_HPP
#define CV2XDAEMON_HPP

#include <iostream>
#include <getopt.h>

#include <Cv2xTelux.hpp>
#include <Cv2xUtils.hpp>
#include <Cv2xLog.hpp>
#include <TcuActivityManager.hpp>

using namespace telux::power;

/**
 * @file       Cv2xDaemon.hpp
 *
 * @brief      This is Sample Reference app which uses Telematics SDK API to Start/Stop V2X Mode,
 *             Start/Stop data call, Register and Handle for SSR( Sub-System Restart) and handles
 *             state transition.
 *
 *             It allows one to interactively invoke most of the public APIs in the Telematics SDK.
 *             Works in Foreground and Background Mode.
 *
 */

class Cv2xDaemon{

    public:
        ~Cv2xDaemon();

        std::mutex mutex_;
        std::condition_variable cv_;

        /**
         * static function to get Cv2xDaemon Instance
         */
        static Cv2xDaemon & getInstance();

        /**
         * Display the usage of program
         */
        void printUsage(std::string appName);

        /**
         * Process input flags from user
         */
        Status parseArguments(int argc, char **argv);

        /**
         * Perform actions bases on user input flags
         */
        Status handleArguments(bool &isRunningDaemonMode);


        /**
         * Register Signal handle for proper cleanup of program.
         */
        void setupSignalHandler();

        /**
         * Initialize V2X subsystem
         */
        Status initV2X();

        /**
         * De-Initialize V2X and Data subsystem
         */
        Status deInit();

        /**
         * Switch the CV2X mode to start
         */
        Status startV2xMode();

        /**
         * Switch the CV2X mode to stop
         */
        Status stopV2xMode();

        /**
         * Daemonizes the app, start modem data calll and bring up IP/NON-IP V2X interface
         * register for various listener,like v2x radio status, data connection change, SSR
         */
        Status runAsDaemon();

        /**
         * Handle system power notifications from powr manager daemon
         */
        Status handleSystemStateChange();

        /**
         * Register system power notifications from powr manager daemon
         */
        Status enableSysPowerNotification();

        /**
         * set latest system state and cache to Cv2xDaemon
         */
        void setSystemState(TcuActivityState newState);

        /**
         * get latest cached system state
         */
        TcuActivityState getSystemState();

    private:
        Cv2xDaemon();

        /**
         * Flags to run command line options
         */
        int daemonMode_;
        int startV2x_;
        int stopV2x_;
        TcuActivityState systemState_;

        /**
         * Wrapper class to perform Telsdk Operations
         */
        std::shared_ptr<Cv2xTelux> cv2xTelux_;
        std::shared_ptr<telux::power::ITcuActivityManager> sysStateMgr_;
        std::shared_ptr<telux::power::ITcuActivityListener> sysStateListener_;
};
#endif
