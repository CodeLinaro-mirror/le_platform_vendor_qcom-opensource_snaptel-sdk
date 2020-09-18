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
  * @file: KinematicsReceive.h
  *
  * @brief: Application that abstracts and handles the Kinematics SDK.
  *
  */

#ifndef __KINEMATICS_RECEIVE_H__
#define __KINEMATICS_RECEIVE_H__

#include <iostream>
#include <memory>
#include <unistd.h>
#include <telux/loc/LocationDefines.hpp>
#include <telux/loc/LocationFactory.hpp>
#include <telux/loc/LocationManager.hpp>
#include <telux/loc/LocationListener.hpp>
#include <mutex>


using std::cout;
using std::shared_ptr;
using std::make_shared;
using std::mutex;
using telux::loc::ILocationInfoEx;
using telux::loc::ILocationListener;
using telux::loc::LocationFactory;
using telux::common::ErrorCode;
using std::lock_guard;


class KinematicsReceive : public std::enable_shared_from_this<KinematicsReceive>,
public ILocationListener {
private:
   static shared_ptr<KinematicsReceive> instance;
   static mutex sync;
   uint16_t interval = 100;

   /**
   * Object that holds all location information.
    */
   shared_ptr<ILocationInfoEx> locationInfo = nullptr;

   void onDetailedLocationUpdate(const shared_ptr<ILocationInfoEx> &locationInfo);

   void startDetailsCallback(ErrorCode eventError);


protected:

public:

   /**
    * Default Constructor. Uses default interval of 100 ms. if you use
   * get method.
    */
   KinematicsReceive();


    /**
    * Constructor that creates a RadioReceive Object
    * @param interval - Minimum time interval between two consecutive
    * reports in milliseconds. It can be interval or more.
    */
   KinematicsReceive(uint16_t interval);


   /**
    * Method that gets the most up to date location.
    * @return a shared pointer of IlocationInfoEx structure that holds
    * all location data.
    * @see ILocationInfoEx in Snaptel SDK.
    */
   shared_ptr<ILocationInfoEx> getLocation();


    /**
    * Destructor that closes listener to Location SDK. This method closes
    * the listener for all object singleton owners as well as nulls all 
    * pointer data. If other owners have an instance of this class, then
    * on getLocation(), singleton will handle itself again to get new fixes.
    */
   void close();

};

#endif
