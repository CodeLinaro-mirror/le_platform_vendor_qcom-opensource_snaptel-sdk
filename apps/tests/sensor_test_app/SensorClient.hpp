/*
 *  Copyright (c) 2021, The Linux Foundation. All rights reserved.
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

#ifndef SENSORCLIENT_HPP
#define SENSORCLIENT_HPP

#include <memory>
#include <string>

#include <telux/sensor/Sensor.hpp>
#include <telux/sensor/SensorDefines.hpp>

using namespace telux::sensor;
using namespace telux::common;

class SensorClient : public ISensorEventListener,
                     public std::enable_shared_from_this<SensorClient> {
 public:
    SensorClient(int id, std::shared_ptr<ISensor> sensor, bool verboseNotification);
    ~SensorClient();
    void init();
    void cleanup();
    void printInfo();
    virtual void onEvent(std::shared_ptr<std::vector<SensorEvent>> events) override;
    virtual void onConfigurationUpdate(SensorConfiguration configuration) override;
    void configure(SensorConfiguration config);
    void activate();
    void deactivate();
    void enableLowPowerMode();
    void disableLowPowerMode();
    std::shared_ptr<ISensor> getSensor() const {
        return sensor_;
    }

    const int id_;

 private:
    std::shared_ptr<ISensor> sensor_;
    bool verboseNotification_;
    std::string tag_;
    uint64_t lastBatchReceivedAt_;
};

#endif  // SENSORCLIENT_HPP
