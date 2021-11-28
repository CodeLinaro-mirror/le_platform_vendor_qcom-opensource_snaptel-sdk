/*
 *  Copyright (c) 2018-2019, The Linux Foundation. All rights reserved.
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
 *  Copyright (c) 2021 Qualcomm Innovation Center, Inc. All rights reserved.
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
 * This is a sample program to register and receive location updates
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "telux/loc/LocationDefines.hpp"
#include "telux/loc/LocationFactory.hpp"
#include "telux/loc/LocationManager.hpp"
#include "telux/loc/LocationListener.hpp"

#define PRINT_NOTIFICATION std::cout << "\033[1;35mNOTIFICATION: \033[0m"

using namespace telux::loc;
using namespace telux::common;

class MyLocationListener : public telux::loc::ILocationListener {
public:
   void onDetailedLocationUpdate(const std::shared_ptr<telux::loc::ILocationInfoEx> &locationInfo) {
      std::cout << std::endl;
      std::cout << "*********************** Detailed Location Report *********************" << std::endl;
      time_t realtime;
      realtime = (time_t)(locationInfo->getTimeStamp());
      PRINT_NOTIFICATION << "Timestamp : " << ctime(&realtime) << std::endl;
      PRINT_NOTIFICATION << "Latitude : " << locationInfo->getLatitude() << std::endl;
      PRINT_NOTIFICATION << "Longitude : " << locationInfo->getLongitude() << std::endl;
      PRINT_NOTIFICATION << "Altitude : " << locationInfo->getAltitude() << std::endl;
   }
};

/**
 * Main routine
 */
int main(int argc, char ** argv) {
   std::shared_ptr<telux::loc::ILocationListener> myLocationListener
      = std::make_shared<MyLocationListener>();
   std::shared_ptr<telux::loc::ILocationManager> locationManager;
   // Get location manager object
    std::promise<ServiceStatus> prom = std::promise<ServiceStatus>();
    auto &locationFactory = LocationFactory::getInstance();
    locationManager = locationFactory.getLocationManager([&](ServiceStatus status) {
        prom.set_value(status);
    });
    std::chrono::time_point<std::chrono::system_clock> startTime, endTime;
    startTime = std::chrono::system_clock::now();
    ServiceStatus locMgrStatus = locationManager->getServiceStatus();
    if (locMgrStatus != ServiceStatus::SERVICE_AVAILABLE) {
         std::cout << "Location subsystem is not ready, Please wait" << std::endl;
    }
    locMgrStatus = prom.get_future().get();
    if (locMgrStatus == ServiceStatus::SERVICE_AVAILABLE) {
          endTime = std::chrono::system_clock::now();
          std::chrono::duration<double> elapsedTime = endTime - startTime;
          std::cout << "Elapsed Time for Subsystems to ready : " << elapsedTime.count()
              << "s\n" << std::endl;
    } else {
          std::cout << "ERROR - Unable to initialize Location subsystem" << std::endl;
          return -1;
    }
   // Registering a listener to get location fixes
   locationManager->registerListenerEx(myLocationListener);
   // Starting the reports for fixes
   locationManager->startDetailedReports(1000,NULL);
   // Exit logic is specific to an application
   std::cout << "Press enter to exit" << std::endl;
   std::string input;
   std::getline(std::cin, input);
   locationManager->deRegisterListenerEx(myLocationListener);
   std::cout << "Exiting application..." << std::endl;
   return 0;
}
