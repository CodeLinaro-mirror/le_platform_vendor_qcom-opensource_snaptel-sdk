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

#ifndef CV2XCONFIGAPP_HPP
#define CV2XCONFIGAPP_HPP

#include <string>
#include "ConsoleApp.hpp"

#include <telux/cv2x/Cv2xConfig.hpp>
#include <telux/cv2x/Cv2xRadioManager.hpp>

using telux::cv2x::ICv2xConfig;
using telux::cv2x::ICv2xConfigListener;
using telux::cv2x::ICv2xRadioManager;
using telux::cv2x::ICv2xListener;

class Cv2xConfigApp : public ConsoleApp,
    public std::enable_shared_from_this<Cv2xConfigApp> {
public:

    Cv2xConfigApp();

    int initialize();

    void retrieveConfigCommand();

    void updateConfigCommand();

    void enforceConfigExpirationCommand();

    ~Cv2xConfigApp();

private:

    bool stopCv2x_ = false;
    bool deregisterConfigListener_ = false;
    bool deregisterStatusListener_ = false;
    std::shared_ptr<ICv2xConfig> cv2xConfig_ = nullptr;
    std::shared_ptr<ICv2xConfigListener> configListener_ = nullptr;
    std::shared_ptr<ICv2xRadioManager> cv2xRadioManager_ = nullptr;
    std::shared_ptr<ICv2xListener> cv2xStatusListener_ = nullptr;

    int cv2xInit();

    void consoleInit();

    int startCv2xMode();

    int stopCv2xMode();

    int retrieveConfigFile(std::string path);

    int updateConfigFile(std::string path);

    int enforceConfigExpiration();

    int generateExpiryConfigFile(std::string configFilePath, std::string expiryFilePath);
};
#endif  // CV2XCONFIGAPP_HPP
