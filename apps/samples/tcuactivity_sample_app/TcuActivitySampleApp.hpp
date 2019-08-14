/*
 *  Copyright (c) 2019, The Linux Foundation. All rights reserved.
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

#ifndef TCUACTIVITYTESTAPP_HPP
#define TCUACTIVITYTESTAPP_HPP

#include <memory>

#include <telux/power/TcuActivityDefines.hpp>
#include <telux/power/PowerFactory.hpp>
#include <telux/power/TcuActivityManager.hpp>
#include <telux/power/TcuActivityListener.hpp>

#define APP_NAME "telux_tcuactivity_sample_app"
#define PRINT_NOTIFICATION std::cout << APP_NAME << " \033[1;35mNOTIFICATION: \033[0m"

using namespace telux::power;
using namespace telux::common;

class TcuActivityTestApp : public ITcuActivityListener,
                           public IServiceStatusListener,
                           public std::enable_shared_from_this<TcuActivityTestApp> {
public:

    TcuActivityTestApp();
    ~TcuActivityTestApp();

    int start();
    void onTcuActivityStateUpdate(TcuActivityState state) override;
    void onServiceStatusChange(ServiceStatus status) override;

    void registerForUpdates();
    void deregisterForUpdates();
private:

    TcuActivityTestApp(TcuActivityTestApp const &) = delete;
    TcuActivityTestApp &operator=(TcuActivityTestApp const &) = delete;

    // Member variable to keep the manager object alive till application ends.
    std::shared_ptr<telux::power::ITcuActivityManager> tcuActivityStateMgr_;
};

#endif  // TCUACTIVITYTESTAPP_HPP
