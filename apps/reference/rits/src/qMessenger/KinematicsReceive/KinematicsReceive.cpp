/*
 *  Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
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
  * @file: KinematicsReceive.cpp
  *
  * @brief: Implementation of the KinematicsReceive.h
  *
  */
#include "KinematicsReceive.h"
#include <chrono>
#include <ctime>

shared_ptr<KinematicsReceive> KinematicsReceive::instance = nullptr;

mutex KinematicsReceive::sync;

void KinematicsReceive::onDetailedLocationUpdate(const shared_ptr<ILocationInfoEx> &locationInfo) {
    lock_guard<mutex> lk(sync);
      this->locationInfo = locationInfo;
      lk.~lock_guard();
   }

void KinematicsReceive::startDetailsCallback(ErrorCode error){
    if (ErrorCode::SUCCESS != error) {
        cout << "Error starting Details Report on Location.\n";
    }
}

KinematicsReceive::KinematicsReceive(){}

shared_ptr<ILocationInfoEx> KinematicsReceive::getLocation(){
    auto start = std::chrono::system_clock::now();
    bool newListener = true;
    if(!KinematicsReceive::instance){
        KinematicsReceive(this->interval);
    }
    while(!KinematicsReceive::instance->locationInfo){
        auto end = std::chrono::system_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        if( newListener && elapsed_seconds.count() > 1){
            cout<<"No location after 1 second. Handling listener.\n";
            KinematicsReceive(this->interval);
            newListener = false;
        }

    }
    lock_guard<mutex> lk(sync);
    auto end = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    //cout<<"Time until first location " << elapsed_seconds.count()<<".\n";
    return KinematicsReceive::instance->locationInfo;
}

KinematicsReceive::KinematicsReceive(uint16_t interval){
    if(!KinematicsReceive::instance){
        KinematicsReceive::instance = make_shared<KinematicsReceive>();
    }
    shared_ptr<ILocationListener> listener = nullptr;
    auto &locationFactory = LocationFactory::getInstance();
    auto locationManager = locationFactory.getLocationManager();
    if (locationManager->onSubsystemReady().get()){
        listener = shared_ptr<ILocationListener>(KinematicsReceive::instance->shared_from_this());
        // Registering a listener to get location fixes
        locationManager->registerListenerEx(listener);
        // Starting the reports for fixes
        auto respCallback = [&](ErrorCode error){
                            startDetailsCallback(error);
                        };
        locationManager->startDetailedReports(interval, respCallback);
    }else{
        cout << "Error on Location Create.\n";
    }
    this->interval = interval;
}

void KinematicsReceive::close(){
   auto &locationFactory = LocationFactory::getInstance();
   auto locationManager = locationFactory.getLocationManager();
   shared_ptr<ILocationListener> listener = nullptr;
   listener = shared_ptr<ILocationListener>(KinematicsReceive::instance->shared_from_this());
   locationManager->deRegisterListenerEx(listener);
   KinematicsReceive::instance->locationInfo = nullptr;
   cout << "Location Listener closed.\n";
}
